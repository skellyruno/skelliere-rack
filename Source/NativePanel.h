#pragma once
#include "PluginProcessor.h"
namespace skl::ui {
// Compact, real host-parameter editor. Every button reads the current slot
// state and writes the same dedicated parameter consumed by the audio thread.
class NativePanel final:public juce::Component,private juce::Timer {
public:
    NativePanel(SkellierProcessor& owner,int slot,int effect):p(owner),s(slot),type(effect){
        const int idsDelay[]={nDelayModel,nDelayBanks,nDelayTrim};
        const int idsGlitch[]={nGlitchStyle,nGlitchCount,nGlitchRate,nGlitchChaos,nGlitchSwing,nGlitchSeed,nGlitchLock,nGlitchSource,nGlitchTrim};
        if(type==DelayNative){for(int id:idsDelay)addControl(id);setSize(500,240);}
        else {for(int id:idsGlitch)addControl(id);setSize(680,500);
            for(int j=0;j<32;++j){auto& b=steps[j];addAndMakeVisible(b);b.setComponentID("native.step."+juce::String(j));b.setTooltip("Select step "+juce::String(j+1)+" and cycle Off, On, Accent, Reverse");b.onClick=[this,j]{if(!valid()||j>=stepCount())return;selected=j;const int next=(int(real(nGlitchState+j))+1)%4;p.editNativeControl(s,type,nGlitchState+j,next/3.f);refresh();};}
            addAndMakeVisible(stepSelect);stepSelect.setComponentID("native.step.select");styleCombo(stepSelect);for(int j=0;j<32;++j)stepSelect.addItem("Step "+juce::String(j+1),j+1);stepSelect.onChange=[this]{selected=std::clamp(stepSelect.getSelectedId()-1,0,31);refresh();};
            addAndMakeVisible(probability);probability.setComponentID("native.probability");probability.setRange(0,100,1);probability.setSliderStyle(juce::Slider::LinearHorizontal);probability.setTextBoxStyle(juce::Slider::TextBoxRight,false,65,25);probability.setTextValueSuffix(" %");probability.setChangeNotificationOnlyOnRelease(true);styleSlider(probability);
            probability.onValueChange=[this]{if(valid()&&selected<stepCount())p.editNativeControl(s,type,nGlitchProbability+selected,float(probability.getValue()/100.));refresh();};
            addAndMakeVisible(trigger);trigger.setComponentID("native.trigger");trigger.setButtonText("TRIGGER");trigger.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff445d38));trigger.onClick=[this]{if(valid()&&int(real(nGlitchSource))==2)p.triggerNativeGlitch(s);};
        }
        refresh();startTimerHz(15);
    }
    ~NativePanel() override{stopTimer();}
    void resized() override{
        const int cols=type==DelayNative?1:2,w=(getWidth()-44)/cols;
        for(size_t j=0;j<controls.size();++j){auto& c=*controls[j];int x=22+int(j%cols)*w,y=60+int(j/cols)*32;c.label.setBounds(x,y,114,27);if(c.combo.isVisible())c.combo.setBounds(x+117,y,w-126,27);else c.slider.setBounds(x+117,y,w-126,27);}
        if(type!=GlitchNative)return;
        for(int j=0;j<32;++j)steps[j].setBounds(22+(j%8)*79,253+(j/8)*37,74,31);
        stepSelect.setBounds(22,416,120,27);probability.setBounds(152,416,330,27);trigger.setBounds(510,416,144,27);
    }
    void paint(juce::Graphics& g) override{
        g.fillAll(juce::Colour(0xff181b19));g.setColour(juce::Colour(0xff82745a));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),6,1);
        g.setColour(juce::Colour(0xffe6dac3));g.setFont(juce::Font(juce::FontOptions(22.f,juce::Font::bold)));g.drawText(type==DelayNative?"SKELL DELAY / ENGINE":"SKELL GLITCH / SEQUENCER",22,12,getWidth()-44,30,juce::Justification::centredLeft);
        g.setFont(juce::Font(juce::FontOptions(12.f)));g.setColour(juce::Colour(0xffb9b7a5));
        if(type==DelayNative){const int model=p.nativeDelayModel(s),want=int(real(nDelayModel));const bool blocked=p.nativeDelayBlocked(s);juce::String msg=blocked?"Memory limit — previous engine or dry retained":model==want?"Active: "+juce::StringArray{"Classic","Modern","Bank"}[model]:"Preparing selected engine — playback may remain dry";g.setColour(juce::Colour(blocked?0xffffba87:0xffc5f694));g.drawFittedText(msg,22,166,getWidth()-44,29,juce::Justification::centredLeft,2);g.setColour(juce::Colour(0xffb9b7a5));g.drawFittedText("Bank: parallel, staggered native taps. Wet trim is explicit; no limiter.",22,195,getWidth()-44,30,juce::Justification::centredLeft,2);}
        else {g.drawText("0 OFF    1 ON    A ACCENT    R REVERSE   |   Click a step to cycle its state",22,226,getWidth()-44,20,juce::Justification::centredLeft);g.drawText("STEP PROBABILITY",152,397,220,17,juce::Justification::centredLeft);g.drawFittedText("Automatic, MIDI note-on or manual trigger. Full-band, zero added PDC. Wet trim is not a limiter.",22,461,getWidth()-44,27,juce::Justification::centredLeft,2);}
    }
private:
    struct Control {int id=0;juce::Label label;juce::ComboBox combo;juce::Slider slider;};
    bool valid()const noexcept{return p.slotType(s)==type;}
    float norm(int id)const noexcept{return p.apvts.getRawParameterValue(SkellierProcessor::nativeId(s,id))->load(std::memory_order_relaxed);}
    float real(int id)const noexcept{return nativeReal(id,norm(id));}
    int stepCount()const noexcept{return int(real(nGlitchCount));}
    static void styleSlider(juce::Slider& v){v.setColour(juce::Slider::trackColourId,juce::Colour(0xff556449));v.setColour(juce::Slider::thumbColourId,juce::Colour(0xffb6d29b));v.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffeadfc6));v.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff101411));v.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff716c58));}
    static void styleCombo(juce::ComboBox& v){v.setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff222a22));v.setColour(juce::ComboBox::textColourId,juce::Colour(0xffe8dfc6));v.setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff81775f));v.setColour(juce::ComboBox::arrowColourId,juce::Colour(0xffb6d29b));}
    void addControl(int id){auto c=std::make_unique<Control>();c->id=id;auto spec=nativeSpec(id);c->label.setText(spec.label,juce::dontSendNotification);c->label.setColour(juce::Label::textColourId,juce::Colour(0xffe3d8bf));addAndMakeVisible(c->label);
        if(spec.names){auto names=juce::StringArray::fromTokens(spec.names,",","");for(int j=0;j<names.size();++j)c->combo.addItem(names[j],j+1);addAndMakeVisible(c->combo);c->combo.setComponentID("native.control."+juce::String(id));styleCombo(c->combo);auto* control=c.get();c->combo.onChange=[this,control,spec]{if(valid()&&control->combo.getSelectedId()>0)p.editNativeControl(s,type,control->id,float(control->combo.getSelectedId()-1)/float(spec.steps-1));refresh();};}
        else {c->slider.setRange(spec.lo,spec.hi,spec.steps>1?1.:spec.unit==Unit::Db?.1:spec.unit==Unit::Pct?.01:0.);c->slider.setSliderStyle(juce::Slider::LinearHorizontal);c->slider.setTextBoxStyle(juce::Slider::TextBoxRight,false,66,25);c->slider.setChangeNotificationOnlyOnRelease(true);if(spec.unit==Unit::Pct){c->slider.textFromValueFunction=[](double x){return juce::String(x*100.,0)+" %";};c->slider.valueFromTextFunction=[](const juce::String& text){return text.getDoubleValue()*.01;};c->slider.updateText();}if(spec.unit==Unit::Db)c->slider.setTextValueSuffix(" dB");auto* control=c.get();c->slider.onValueChange=[this,control,spec]{if(valid())p.editNativeControl(s,type,control->id,float((control->slider.getValue()-spec.lo)/(spec.hi-spec.lo)));};addAndMakeVisible(c->slider);c->slider.setComponentID("native.control."+juce::String(id));styleSlider(c->slider);}
        controls.push_back(std::move(c));}
    void refresh(){if(!valid()){setEnabled(false);return;}for(auto& c:controls){if(c->combo.isVisible())c->combo.setSelectedId(int(real(c->id))+1,juce::dontSendNotification);else if(!c->slider.isMouseButtonDown()){c->slider.setValue(real(c->id),juce::dontSendNotification);c->slider.updateText();}if(c->id==nDelayBanks)c->slider.setEnabled(int(real(nDelayModel))==2);}
        if(type==GlitchNative){selected=std::clamp(selected,0,stepCount()-1);const char* names[]={"0","1","A","R"};const unsigned colours[]={0xff30332e,0xff496d42,0xff826437,0xff5d4b76};for(int j=0;j<32;++j){const int st=int(real(nGlitchState+j));auto& b=steps[j];b.setButtonText(juce::String(j+1).paddedLeft('0',2)+" "+names[st]+" "+juce::String(real(nGlitchProbability+j)*100,0)+"%");b.setColour(juce::TextButton::buttonColourId,juce::Colour(colours[st]).brighter(j==selected?.22f:0.f));b.setEnabled(j<stepCount());}stepSelect.setSelectedId(selected+1,juce::dontSendNotification);if(!probability.isMouseButtonDown())probability.setValue(real(nGlitchProbability+selected)*100,juce::dontSendNotification);trigger.setEnabled(int(real(nGlitchSource))==2);}
        repaint();}
    void timerCallback() override{refresh();}
    SkellierProcessor& p;int s,type,selected=0;std::vector<std::unique_ptr<Control>> controls;std::array<juce::TextButton,32> steps;juce::ComboBox stepSelect;juce::Slider probability;juce::TextButton trigger;
};
}
