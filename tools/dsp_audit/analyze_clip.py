import numpy as np, subprocess, os
from scipy.signal import resample_poly
N=65536; sr=48000
def db(x): return 20*np.log10(max(x,1e-12))
def run(bin_,amp,drive,ceil):
    subprocess.run([os.environ.get("DUMP","./dump_clip"),str(bin_),str(amp),str(drive),str(ceil),os.environ.get("OUTBIN","o.bin")],check=True)
    return np.fromfile(os.environ.get("OUTBIN","o.bin"),dtype=np.float32).astype(np.float64)
def ref(bin_,amp,drive,ceil,os_=8):
    f=bin_*sr/N; n=np.arange(N*os_); x=amp*np.sin(2*np.pi*f*n/(sr*os_))
    g=10**(drive/20); c=10**(ceil/20)
    y=c*np.tanh(np.clip(x*g/c,-32,32))
    return resample_poly(y,1,os_,window=('kaiser',12.0))   # band-limited decimation
w=np.blackman(N)
def spec(x): return np.abs(np.fft.rfft(x*w))/ (w.sum()/2)
print(f"{'tone':>8} {'amp':>5} {'drive':>5} {'ceil':>5} | {'fund dBFS':>9} {'alias(total) dB rel fund':>25} {'worst alias bin dB':>18} {'THD+alias dB':>12}")
for bin_ in (1365,3413,5461,6826,8192,9557):   # ~1k,2.5k,4k,5k,6k,7k Hz
    for amp,drive,ceil in ((0.3,12,-1),(0.5,24,-1),(0.5,24,-6)):
        a=run(bin_,amp,drive,ceil); r=ref(bin_,amp,drive,ceil)
        A=spec(a); R=spec(r)
        fund=A[bin_-3:bin_+4].max()
        # alias = what the real module has that the band-limited reference does not
        d=np.abs(np.fft.rfft((a-r)*w))/(w.sum()/2)
        d[:0]=0
        alias_rms=np.sqrt(np.sum(d[10:]**2))
        print(f"{bin_*sr/N:8.0f} {amp:5.2f} {drive:5.0f} {ceil:5.0f} | {db(fund):9.2f} {db(alias_rms/fund):25.2f} {db(d[10:].max()/fund):18.2f} {db(np.sqrt(np.sum(A[10:]**2)-fund**2)/fund):12.2f}")
