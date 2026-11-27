"""Private source-known Rack tape-kernel benchmark. Never commit manifest/audio/rows."""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import statistics
import subprocess
import tempfile
import time
import numpy as np
import soundfile as sf

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--manifest',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    rows=[];seen=set()
    with tempfile.TemporaryDirectory(prefix='skell-tape-') as d:
        path=Path(d)/'tape.so'
        subprocess.run(['g++','-std=c++17','-O3','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'tests/tape_vocal_adapter.cpp'),'-o',str(path)],check=True)
        lib=C.CDLL(str(path));fn=lib.tape_render
        ptr=np.ctypeslib.ndpointer(dtype=np.float32,flags='C_CONTIGUOUS')
        fn.argtypes=[C.c_int,ptr,ptr,C.c_uint,C.c_uint,C.c_float,C.c_float,C.c_float];fn.restype=C.c_int
        for item in json.loads(a.manifest.read_text())['inputs']:
            src=Path(item['path']);digest=hashlib.sha256(src.read_bytes()).hexdigest()
            if digest!=item['sha256'] or digest in seen:raise ValueError('Fingerprint mismatch or duplicate')
            seen.add(digest);x,fs=sf.read(src,dtype='float32',always_2d=True);x=np.ascontiguousarray(x)
            if x.shape[1] not in (1,2) or not len(x) or not np.isfinite(x).all():raise ValueError('Invalid audio')
            base=np.empty((len(x),x.shape[1],2),dtype=np.float32);fast=np.empty_like(base);cases=[]
            for gain,amount,bias in ((1.,0.,.5),(1.,.5,.5),(2.,1.,0.)):
                ts=[[],[]]
                for rep in range(3):
                    for k in ((0,1) if rep%2==0 else (1,0)):
                        out=fast if k else base;t=time.perf_counter_ns();ok=fn(k,x,out,len(x),x.shape[1],gain,amount,bias);ts[k].append((time.perf_counter_ns()-t)/1e6)
                        if ok!=1 or not np.isfinite(out).all():raise RuntimeError('Render failed')
                    if not np.array_equal(base,fast):raise RuntimeError('Kernel sample parity failed')
                expected=np.pad(x,((12,0),(0,0)))[:len(x)]
                if not np.array_equal(fast[:,:,0],expected):raise RuntimeError('Dry latency failed')
                cases.append({'input_gain':gain,'amount':amount,'bias_control':bias,'exact':True,'raw_ms':ts,'median_ms_per_audio_second':[statistics.median(t)/(len(x)/fs) for t in ts]})
            rows.append({'id':item['id'],'stage':item['stage'],'sha256':digest,'fs':fs,'frames':len(x),'channels':x.shape[1],'cases':cases})
            print(item['id'],flush=True)
    a.output.write_text(json.dumps({'schema':1,'rows':rows,'limits':'Original Rack tape interpolation/residual kernel only, not full module or host; native timing includes prepare, no private inputs in CI.'},indent=2)+'\n')

if __name__=='__main__':main()
