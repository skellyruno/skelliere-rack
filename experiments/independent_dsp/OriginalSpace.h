#pragma once
// Independent FDN design, not a commercial effect reproduction.
#include "OriginalDynamics.h"
namespace skell_lab {
class OriginalSpace {
    struct Ring {
        std::vector<double> v;
        std::size_t pos=0;
        void prepare(std::size_t n){v.assign(std::max<std::size_t>(1,n),0.);pos=0;}
        double read() const {return v[pos];}
        void write(double x){v[pos]=x;if(++pos==v.size())pos=0;}
        void reset(){std::fill(v.begin(),v.end(),0.);pos=0;}
    };
    std::array<Ring,8> lines{};
    std::array<std::array<Ring,2>,2> diffuse{};
    std::array<double,8> damp{},gain{};
    double pole,attack,release,minimumWet,env=0.,lastDuck=1.;
    static constexpr double norm=0.3535533905932737622;
    static constexpr double mix=.20;
    static void hadamard(std::array<double,8>& a){
        for(std::size_t step=1;step<8;step*=2)
            for(std::size_t i=0;i<8;i+=2*step)
                for(std::size_t j=0;j<step;++j){double u=a[i+j],v=a[i+j+step];a[i+j]=u+v;a[i+j+step]=u-v;}
        for(auto& x:a)x*=norm;
    }
public:
    OriginalSpace(double fs,double rt60=1.8,double dampingHz=4200.,double duckDb=9.){
        validRate(fs);
        if(!std::isfinite(rt60)||rt60<.15||rt60>12||!std::isfinite(dampingHz)||dampingHz<100||dampingHz>fs*.45||!std::isfinite(duckDb)||duckDb<0||duckDb>18)throw std::invalid_argument("space settings");
        pole=std::exp(-2*3.14159265358979323846*dampingHz/fs);attack=decay(fs,14);release=decay(fs,260);minimumWet=db(-duckDb);
        constexpr std::array<double,8> ms{29.3,34.1,39.7,43.9,49.3,53.7,59.3,67.1};
        for(std::size_t i=0;i<8;++i){auto n=std::size_t(std::llround(fs*ms[i]*.001));lines[i].prepare(n);gain[i]=std::pow(10.,-3*double(n)/(fs*rt60));}
        for(std::size_t c=0;c<2;++c)for(std::size_t i=0;i<2;++i)diffuse[c][i].prepare(std::size_t(std::llround(fs*(.0047+.0031*i+.0013*c))));
    }
    void reset(){for(auto& l:lines)l.reset();for(auto& c:diffuse)for(auto& l:c)l.reset();damp.fill(0.);env=0;lastDuck=1.;}
    double wetGain()const{return lastDuck;}
    // Wet-only output; all storage allocated in constructor; no host latency.
    Frame wet(Frame dry){
        for(auto& x:dry)x=clean(x);
        const double p=peak(dry),a=p>env?attack:release;env=a*env+(1-a)*p;
        // Smooth bounded rational ducking, no sample-rate logarithm or pow.
        const double amount=env/(env+.10);lastDuck=1.-(1.-minimumWet)*amount;
        for(std::size_t c=0;c<2;++c)for(auto& l:diffuse[c]){const double d=l.read(),x=dry[c],y=d-.55*x;l.write(x+.55*y);dry[c]=y;}
        std::array<double,8> feedback{};Frame out{};
        for(std::size_t i=0;i<8;++i){const double y=lines[i].read();damp[i]=pole*damp[i]+(1.-pole)*y;if(std::abs(damp[i])<1e-25)damp[i]=0.;feedback[i]=gain[i]*damp[i];out[0]+=y*norm;out[1]+=y*norm*((i&2)?-1.:1.);}
        hadamard(feedback);
        for(std::size_t i=0;i<8;++i)lines[i].write(feedback[i]+mix*norm*(dry[0]+((i&1)?-dry[1]:dry[1])));
        for(auto& x:out)x*=lastDuck;
        return out;
    }
};
}
