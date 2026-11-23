"""Independent offline numerical test of the publicly visible *Skelliere* Warm Drive math.

Not a TVB/VMR plugin render, not a low-latency implementation or evidence of vendor DSP.
"""
import json
import numpy as np
from scipy.signal import resample_poly,lfilter
from pathlib import Path

OUT=Path('/mnt/data/vmr_dsp_workspace')
fs=48000
n=fs*2
x=.45*np.sin(2*np.pi*17000*np.arange(n)/fs)
drive_db=12.0
tone_hz=12000
pre=10**(drive_db/20)

def waveshape_and_tone(x,fs):
    y=(np.tanh(2.0*x*pre+.2)-np.tanh(.2))/.97
    y=lfilter([1.,-1.],[1.,-.995],y)
    a=np.exp(-2*np.pi*tone_hz/fs)
    y=lfilter([1-a],[1.,-a],y)
    return y*(.5/np.sqrt(pre))

legacy=waveshape_and_tone(x,fs)
# Reference-only: polyphase interpolation + decimation, not a current Skelliere algorithm.
x4=resample_poly(x,4,1,window=('kaiser',12))
ref4=resample_poly(waveshape_and_tone(x4,fs*4),1,4,window=('kaiser',12))[:len(x)]

def projection(signal,freq):
    chunk=signal[fs//2:fs*3//2] # 1 sec with integer bin frequency
    t=np.arange(len(chunk))/fs
    coeff=2.0*np.dot(chunk,np.exp(-2j*np.pi*freq*t))/len(chunk)
    return 20*np.log10(max(abs(coeff),1e-15))
res={
 'condition': {'sample_rate':fs,'frequency_hz':17000,'input_amplitude':.45,'drive_db':drive_db,'tone_hz':tone_hz,'mix':1},
 'comparison_note':'Offline math-only Skelliere saturation simulation versus oversampled mathematical reference. VMR/TVB plugin binaries not executed.',
 'legacy_alias_harmonic_dbfs':{'3k':float(projection(legacy,3000)),'14k':float(projection(legacy,14000))},
 'four_x_reference_alias_dbfs':{'3k':float(projection(ref4,3000)),'14k':float(projection(ref4,14000))},
 'legacy_fundamental_dbfs':float(projection(legacy,17000)),
 'four_x_reference_fundamental_dbfs':float(projection(ref4,17000)),
}
res['reduction_db']={key:res['legacy_alias_harmonic_dbfs'][key]-res['four_x_reference_alias_dbfs'][key] for key in ('3k','14k')}
(OUT/'skelliere_saturation_alias_probe.json').write_text(json.dumps(res,indent=2))
print(json.dumps(res,indent=2))
