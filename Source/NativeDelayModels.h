#pragma once
#include "../research/NativeDelayExchange.h"
#include <native/codec.h>
#include <atomic>
#include <memory>
namespace skl::native_delay {
// Reservations cover prepared, pending, fading and retired engines. This is a
// delay-storage budget, not a guarantee of total process RSS.
class RackBudget {
public:
    explicit RackBudget(std::size_t limit=384u*MiB):limit_(limit){}
    bool acquire(std::size_t n) noexcept {auto old=used_.load();do{if(n>limit_||old>limit_-n)return false;}while(!used_.compare_exchange_weak(old,old+n));return true;}
    void release(std::size_t n) noexcept {used_.fetch_sub(n);}
    std::size_t used() const noexcept{return used_.load();}
private:std::atomic<std::size_t> used_{0};std::size_t limit_;
};
struct Selection {int model=0,banks=2,route=0; bool operator==(const Selection& b)const noexcept{return model==b.model&&banks==b.banks&&route==b.route;}};
inline Selection selection(int model,int banks,int route) noexcept {return {std::clamp(model,0,2),std::clamp(banks,1,8),std::clamp(route,0,2)};}
// The rack macros use the existing raw Classic mapping. Modern and Bank have
// their own raw control ranges. Bank taps are parallel, normalized and staggered.
inline skellay::LiveControls macroControls(Selection s,const std::array<int,8>& classic) noexcept {
    using namespace skellay;LiveControls c;c.model=Model(s.model+2);
    if(s.model==0){std::copy(classic.begin(),classic.end(),c.values.begin());return c;}
    if(s.model==1){std::copy(SKELLdelay::delay3Default.begin(),SKELLdelay::delay3Default.end(),c.values.begin());auto& v=c.values;v[1]=s.route==2?2:1;v[4]=classic[4];v[5]=3000+int(std::lround(classic[5]*3000./512.));v[14]=int(std::lround(classic[3]*6000./128.));v[16]=classic[7]*6000/128;v[23]=6000;v[25]=0;return c;}
    auto bank=skelly::fldbank::defaultState();bank.p[0]=0;std::copy_n(bank.p.begin(),131,c.values.begin());
    for(int b=0;b<8;++b){int i=4+b*16;c.values[i+6]=classic[3];c.values[i+8]=std::clamp(classic[4]*10*(b+1),1,7680);c.values[i+9]=classic[5];c.values[i+10]=classic[7]*512;c.values[i+14]=128/s.banks;}
    return c;
}
inline skellay::Configuration configuration(Selection s,const std::array<int,8>& classic) {
    const auto c=macroControls(s,classic);
    if(s.model!=2){skelly::delay_research::RawState raw{int(c.model),{}};raw.controls.assign(c.values.begin(),c.values.begin()+skellay::liveControlCount(c.model));auto encoded=skelly::delay_research::encode(raw);return {c.model,encoded?std::move(*encoded):std::vector<std::uint8_t>{}};}
    auto bank=skelly::fldbank::defaultState();std::copy_n(c.values.begin(),131,bank.p.begin());
    for(int b=0;b<8;++b){bank.bank[b].enabled=b<s.banks;bank.bank[b].mode=s.route+1;}
    return {c.model,skelly::fldbank::serialise(bank)};
}
// One producer/one consumer; the published engine owns its reservation. Never
// destroy engines on the audio thread. A model generation prevents old tails
// reappearing after a slot reset (remove/reinsert).
class PreparedSlot {
    struct Owned final:skellay::Engine {
        std::unique_ptr<skellay::Engine> inner;std::shared_ptr<RackBudget> budget;std::size_t bytes;
        std::atomic<unsigned>& generation;std::atomic<int>& audible;unsigned ownGeneration;Selection model;
        Owned(std::unique_ptr<skellay::Engine> e,std::shared_ptr<RackBudget> b,std::size_t n,std::atomic<unsigned>& g,std::atomic<int>& a,unsigned gen,Selection s):inner(std::move(e)),budget(std::move(b)),bytes(n),generation(g),audible(a),ownGeneration(gen),model(s){}
        ~Owned() override{inner.reset();budget->release(bytes);}
        void updateTempo(double v) noexcept override{if(inner&&generation.load()==ownGeneration)inner->updateTempo(v);}
        bool updateRealtimeControls(const skellay::LiveControls& c) noexcept override{if(generation.load()!=ownGeneration)return false;audible.store(model.model,std::memory_order_relaxed);if(!inner)return true;std::array<int,8> raw{};std::copy_n(c.values.begin(),8,raw.begin());return inner->updateRealtimeControls(macroControls(model,raw));}
        bool process(float* l,float* r,int n) noexcept override{if(generation.load()!=ownGeneration){std::fill_n(l,n,0.f);std::fill_n(r,n,0.f);return true;}if(!inner){std::fill_n(l,n,0.f);std::fill_n(r,n,0.f);return true;}return inner->process(l,r,n);}
    };
public:
    // prepare/service/collect are control-thread only; no concurrent initialise.
    void prepare(int sr,int block,std::shared_ptr<RackBudget> b){exchange.reset();budget=std::move(b);rate=sr;capacity=std::clamp(block,32,4096);generation.store(1);audible.store(-1);lastGeneration=0;exchange.initialise({},capacity,int(sr*.020));}
    void invalidate() noexcept {generation.fetch_add(1);audible.store(-1);}
    bool service(Selection s,const std::array<int,8>& classic,double bpm){
        exchange.collect();const auto gen=generation.load();if(lastGeneration==gen&&s==last)return true;
        if(rate<32000||rate>192000||!budget)return false;
        if(s.model==0){exchange.publish(std::make_unique<Owned>(nullptr,budget,0,generation,audible,gen,s));last=s;lastGeneration=gen;return true;}
        auto c=configuration(s,classic);const auto bytes=delayStorageEstimate(c,rate,30.,capacity);
        // The estimate includes the temporary Bank construction ring. Reserve
        // that peak against the rack budget, but enforce the per-engine cap
        // on actual persistent storage after construction.
        if(bytes>256u*MiB||!budget->acquire(bytes))return false;
        std::unique_ptr<skellay::Engine> e;
        try{e=skellay::prepare(c,rate,std::clamp(bpm,30.,300.),capacity,30.);}catch(...){budget->release(bytes);return false;}
        if(!e){budget->release(bytes);return false;}
        const auto measured=e->delayStorageBytes();
        // Only Bank exposes its persistent ring capacity; these rack banks
        // have no pattern tables. Keep Modern's conservative estimate.
        const auto resident=s.model==2&&measured>0?measured:bytes;
        if(resident>128u*MiB||resident>bytes){e.reset();budget->release(bytes);return false;}
        budget->release(bytes-resident);
        std::unique_ptr<Owned> owned;try{owned=std::make_unique<Owned>(std::move(e),budget,resident,generation,audible,gen,s);}catch(...){e.reset();budget->release(resident);return false;}
        exchange.publish(std::move(owned));last=s;lastGeneration=gen;return true;
    }
    void collect(){exchange.collect();}
    bool process(float* l,float* r,int n,double bpm,const skellay::LiveControls& c) noexcept {
        // Before first publication the empty exchange is dry-in/dry-out. The
        // caller treats that as unprepared, never as the requested wet model.
        audible.store(-1,std::memory_order_relaxed);const bool ok=exchange.process(l,r,n,bpm,&c);return ok&&audible.load(std::memory_order_relaxed)>=0;
    }
    int model() const noexcept{return audible.load(std::memory_order_relaxed);}
    int maxBlock() const noexcept{return capacity;}
private:
    // Exchange must destruct before the atomics and shared budget it references.
    std::shared_ptr<RackBudget> budget;std::atomic<unsigned> generation{1};std::atomic<int> audible{-1};Selection last{};unsigned lastGeneration=0;int rate=0,capacity=32;skellay::EngineExchange exchange;
};
}
