import numpy as np
from scipy.signal import resample_poly
N=65536; sr=48000
def db(x): return 20*np.log10(max(x,1e-12))
def logcosh(z):
    a=np.abs(z); return a+np.log1p(np.exp(-2*a))-np.log(2)
def clip_base(x,g,c):  # production: base-rate tanh
    return c*np.tanh(np.clip(x*g/c,-32,32))
def clip_adaa1(x,g,c):
    z=np.clip(x*g/c,-32,32); zp=np.concatenate(([0],z[:-1]))
    dz=z-zp; small=np.abs(dz)<1e-5
    y=np.where(small,np.tanh(0.5*(z+zp)),(logcosh(z)-logcosh(zp))/np.where(small,1,dz))
    return c*y
def ref(x_fn,f,amp,g,c,os_=8):
    n=np.arange(N*os_); x=amp*np.sin(2*np.pi*f*n/(sr*os_))
    return resample_poly(c*np.tanh(np.clip(x*g/c,-32,32)),1,os_,window=('kaiser',12.0))
w=np.blackman(N); norm=w.sum()/2
def alias_db(a,r,fund_bin):
    d=np.abs(np.fft.rfft((a-r)*w))/norm
    A=np.abs(np.fft.rfft(a*w))/norm; fund=A[fund_bin-3:fund_bin+4].max()
    return db(np.sqrt(np.sum(d[10:]**2))/fund)
def report_v1():
    print("== alias vs 8x reference (dB rel fundamental, lower = better) ==")
    print(f"{'tone':>6} {'amp':>4} {'drv':>4} {'ceil':>4} | {'base-rate':>10} {'ADAA1':>8}")
    for b in (3413,5461,6826,9557):
        f=b*sr/N
        for amp,drv,ce in ((0.3,12,-1),(0.5,24,-1),(0.5,24,-6)):
            g=10**(drv/20); c=10**(ce/20)
            x=amp*np.sin(2*np.pi*f*np.arange(N)/sr); r=ref(None,f,amp,g,c)
            print(f"{f:6.0f} {amp:4.1f} {drv:4d} {ce:4d} | {alias_db(clip_base(x,g,c),r,b):10.2f} {alias_db(clip_adaa1(x,g,c),r,b):8.2f}")
    print("\n== linear-region (unity gain, tiny signal) HF response: ADAA1 vs base-rate ==")
    g=1.0;c=1.0
    for f in (1000,5000,10000,15000,20000):
        b=int(round(f*N/sr)); f=b*sr/N
        x=0.01*np.sin(2*np.pi*f*np.arange(N)/sr)
        A=lambda y: np.abs(np.fft.rfft(y*w))[b-2:b+3].max()
        print(f"{f:7.0f} Hz  base {db(A(clip_base(x,g,c))/A(x)):6.2f} dB   ADAA1 {db(A(clip_adaa1(x,g,c))/A(x)):6.2f} dB")
    print("\n== ceiling (max |y|, ceiling=-6 dBFS => 0.5012) ==")
    x=0.8*np.sin(2*np.pi*(6826*sr/N)*np.arange(N)/sr)
    print("base",np.abs(clip_base(x,10**(24/20),10**(-6/20))).max(),"ADAA1",np.abs(clip_adaa1(x,10**(24/20),10**(-6/20))).max())

if __name__=="__main__": report_v1()
