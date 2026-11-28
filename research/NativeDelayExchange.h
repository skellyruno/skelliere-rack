#pragma once
// Owner-source-only Skellay integration candidate. The native delay engines
// and their exchange implementation remain external; no DSP substitute.
#include <native/EngineExchange.h>
#include <native/FruityDelayBank.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace skl::native_delay {
// Bound delay-line memory before constructing an engine. Includes the original
// line + replacement line during native Bank live-timing preparation, and the
// original pattern table. This is a delay-storage estimate, not process RSS.
inline constexpr std::size_t MiB = 1024u * 1024u;
inline std::size_t delayStorageEstimate(const skellay::Configuration& c,
                                       int rate,double floor,int block) {
    using skellay::Model;
    if(rate<32000 || rate>192000 || block<32 || block>4096 ||
       !std::isfinite(floor)||floor<20||floor>400||!skellay::validate(c))
        return std::numeric_limits<std::size_t>::max();
    if(c.model==Model::modern) {
        // Four native power-of-two ring buffers; extra diffusion storage is
        // not included in this estimate.
        const std::size_t frames=rate>126976?0x400000:rate>61440?0x200000:0x100000;
        return 4u*frames*sizeof(float);
    }
    if(c.model==Model::classic)
        return 2u*std::size_t(std::ceil((240./20.+.5)*rate+block+4))*sizeof(float)*2u;
    const auto s=skelly::fldbank::parse(c.bytes);
    if(!s) return std::numeric_limits<std::size_t>::max();
    const int os=1<<s->p[132];
    std::size_t bytes=0;
    for(const auto& b:s->bank) if(b.enabled) {
        const double seconds=b.sync?240./floor:2.;
        const auto frames=std::size_t(std::ceil(seconds*rate*os+block*os+.5*rate*os+4));
        // Old + new stereo ring, plus conservative pattern-table allowance.
        bytes+=frames*((b.mode!=0?4u:0u)+(b.steps>0?1u:0u))*sizeof(float);
    }
    return bytes;
}

class Exchange {
public:
    // Call initialise only when the audio thread has stopped. Bank timing is
    // explicitly bounded to floor; the rack must request a new prepared model
    // before lowering this floor. Never silently substitute a different model.
    bool initialise(const skellay::Configuration& c,int sr,double bpm,int block,
                    double floor=30.,std::size_t maxStorage=128u*MiB) {
        if(!validTempo(bpm,floor)||delayStorageEstimate(c,sr,floor,block)>maxStorage)return false;
        auto engine=skellay::prepare(c,sr,bpm,block,floor);
        if(!engine)return false;
        exchange.initialise(std::move(engine),block,int(std::ceil(sr*.020)));
        rate_=sr;block_=block;floor_=floor;budget_=maxStorage;initialised_=true;
        return true;
    }
    // Single producer only. Prepare and release all large buffers outside the
    // callback. Keep the last good engine if validation or preparation fails.
    bool publish(const skellay::Configuration& c,double bpm) {
        if(!initialised_||!validTempo(bpm,floor_)||
           delayStorageEstimate(c,rate_,floor_,block_)>budget_)return false;
        exchange.collect();
        auto e=skellay::prepare(c,rate_,bpm,block_,floor_);
        if(!e)return false;
        exchange.publish(std::move(e));return true;
    }
    void collect() { exchange.collect(); }
    // Audio thread: no allocation, no structural prepare, no I/O. This first
    // exchange uses a 20 ms crossfade; preservation of long old echo tails is
    // intentionally not asserted. Below-floor tempos fail explicitly.
    bool process(float* l,float* r,int n,double bpm,const skellay::LiveControls* controls=nullptr) noexcept {
        if(!initialised_||!validTempo(bpm,floor_))return false;
        return exchange.process(l,r,n,bpm,controls);
    }
private:
    static bool validTempo(double bpm,double floor) noexcept {
        return std::isfinite(bpm)&&std::isfinite(floor)&&floor>=20&&floor<=bpm&&bpm<=400;
    }
    skellay::EngineExchange exchange;
    int rate_=0,block_=0;double floor_=30.;std::size_t budget_=0;bool initialised_=false;
};
}
