#pragma once
#include <memory>
namespace skelly::voiceshift {
enum Param:int { pitch,formant,link,mode,drive,mix };
enum class Mode { transpose,quantize,robot };
constexpr double robotNote=60.,minHz=70.,maxHz=1000.;
class VoiceShiftCore {
public:
 virtual ~VoiceShiftCore()=default;
 virtual void prepare(double) noexcept=0;
 virtual void reset() noexcept=0;
 virtual void process(float*,float*,int,const float*) noexcept=0;
 virtual int latencySamples() const noexcept=0;
};
std::unique_ptr<VoiceShiftCore> make();
int latencyFor(double) noexcept;
}
