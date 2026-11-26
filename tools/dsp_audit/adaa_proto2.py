import numpy as np
from adaa_proto import clip_base, clip_adaa1, ref, logcosh, N, sr, db, w, norm
def alias_mag_db(a,r,b):
    A=np.abs(np.fft.rfft(a*w))/norm; R=np.abs(np.fft.rfft(r*w))/norm
    fund=A[b-3:b+4].max(); e=np.maximum(0,A-R)[10:]      # excess energy over band-limited truth; delay-invariant
    return db(np.sqrt(np.sum(e**2))/fund)
def clip_hybrid(x,g,c,lo=0.3,hi=1.0):
    z=np.clip(x*g/c,-32,32); zp=np.concatenate(([0],z[:-1]))
    m=np.maximum(np.abs(z),np.abs(zp)); t=np.clip((m-lo)/(hi-lo),0,1); wgt=t*t*(3-2*t)
    return (1-wgt)*clip_base(x,g,c)+wgt*clip_adaa1(x,g,c)
print(f"{'tone':>6} {'amp':>4} {'drv':>4} {'ceil':>4} | {'base':>8} {'ADAA1':>8} {'hybrid':>8}   (alias dB rel fundamental, delay-invariant)")
for b in (3413,5461,6826,9557):
    f=b*sr/N
    for amp,drv,ce in ((0.3,12,-1),(0.5,24,-1),(0.5,24,-6)):
        g=10**(drv/20); c=10**(ce/20)
        x=amp*np.sin(2*np.pi*f*np.arange(N)/sr); r=ref(None,f,amp,g,c)
        print(f"{f:6.0f} {amp:4.1f} {drv:4d} {ce:4d} | {alias_mag_db(clip_base(x,g,c),r,b):8.2f} {alias_mag_db(clip_adaa1(x,g,c),r,b):8.2f} {alias_mag_db(clip_hybrid(x,g,c),r,b):8.2f}")
print("\nlinear-region HF response, tiny signal at unity gain (dB re input; base-rate is 0.00 by construction):")
for f in (5000,10000,15000):
    b=int(round(f*N/sr)); f=b*sr/N; x=0.01*np.sin(2*np.pi*f*np.arange(N)/sr)
    A=lambda y: np.abs(np.fft.rfft(y*w))[b-2:b+3].max()
    print(f"{f:7.0f} Hz  ADAA1 {db(A(clip_adaa1(x,1.0,1.0))/A(x)):6.2f} dB   hybrid {db(A(clip_hybrid(x,1.0,1.0))/A(x)):6.2f} dB")
