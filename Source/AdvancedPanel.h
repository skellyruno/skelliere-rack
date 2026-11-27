#pragma once
#include "PluginProcessor.h"
#include "Look.h"
namespace skl::ui {
// The same real APVTS-backed component is used in the rack callout and the
// editor verification harness. The frequency view is detector coverage / GR,
// not a simulated audio spectrum or a fabricated transfer curve.
class AdvancedPanel final:public juce::Component,private juce::Timer {
public:
    AdvancedPanel(SkellierProcessor& owner,int slot,int effect):p(owner),s(slot),type(effect){
        setName("Advanced "+juce::String(info(type).name));setSize(680,type==Multiband?536:type==Doubler?342:218);
        addAndMakeVisible(mode);mode.setButtonText("Advanced engine");mode.setColour(juce::ToggleButton::textColourId,juce::Colour(0xffe7d9bc));
        modeAttachment=std::make_unique<juce::ButtonParameterAttachment>(*parameter(advMode),mode);
        if(type==Multiband||type==Doubler){addAndMakeVisible(selector);for(int j=0;j<(type==Multiband?8:2);++j)selector.addItem(type==Doubler?"Voice "+juce::String(j+1):j<6?"Band "+juce::String(j+1):"Floating "+juce::String(j-5),j+1);selector.setSelectedId(1,juce::dontSendNotification);selector.onChange=[this]{selected=selector.getSelectedId()-1;bind();repaint();};}
        if(type==Multiband){addAndMakeVisible(solo);solo.setButtonText("Audition band");solo.setClickingTogglesState(true);solo.onClick=[this]{if(!enabled())solo.setToggleState(false,juce::dontSendNotification);p.bandSolo[size_t(s)].store(solo.getToggleState()?selected:-1);repaint();};}
        mode.onStateChange=[this]{if(!enabled()){p.bandSolo[size_t(s)].store(-1);solo.setToggleState(false,juce::dontSendNotification);}repaint();};
        for(int j=0;j<9;++j){auto& label=labels[j];label.setColour(juce::Label::textColourId,juce::Colour(0xffc9c1af));label.setFont(font(12.f,true));addChildComponent(label);auto& a=sliders[j];a.setSliderStyle(juce::Slider::LinearHorizontal);a.setTextBoxStyle(juce::Slider::TextBoxRight,false,85,24);a.setColour(juce::Slider::trackColourId,juce::Colour(0xffbb9b66));a.setColour(juce::Slider::thumbColourId,juce::Colour(0xffe6d0a6));a.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffe7d9bc));a.setColour(juce::Slider::textBoxOutlineColourId,juce::Colour(0xff49443a));addChildComponent(a);}
        bind();startTimerHz(25);
    }
    ~AdvancedPanel() override {stopTimer();endDrag();p.bandSolo[size_t(s)].store(-1);}
    void resized() override {
        mode.setBounds(20,43,200,26);selector.setBounds(22,type==Multiband?296:86,170,27);solo.setBounds(204,296,145,27);
        const int top=type==Multiband?337:132;
        for(int j=0;j<9;++j){const int col=j%2,row=j/2,x=22+col*318,y=top+row*35;labels[j].setBounds(x,y,84,27);sliders[j].setBounds(x+88,y,222,27);}
    }
    void paint(juce::Graphics& g) override {
        const auto area=getLocalBounds().toFloat();g.fillAll(juce::Colour(0xff161918));g.setColour(juce::Colour(0xff766449));g.drawRoundedRectangle(area.reduced(1),6,1);g.setColour(juce::Colour(0xffe5d4b6));g.setFont(font(21.f,true));g.drawText("SKELL  /  "+juce::String(info(type).name).toUpperCase(),22,9,getWidth()-44,30,juce::Justification::centredLeft);
        g.setColour(enabled()?juce::Colour(0xffd6ba7d):juce::Colour(0xff9d9e94));g.setFont(font(12.f,true));g.drawText(enabled()?"ADVANCED ACTIVE":"LEGACY ACTIVE",getWidth()-214,43,192,26,juce::Justification::centredRight);
        if(type==Multiband){paintGraph(g);g.setColour(juce::Colour(0xff9d9e94));g.setFont(font(11.f));g.drawText("Drag crossover lines or floating handles. Click a band to edit.",22,273,635,18,juce::Justification::centredLeft);g.drawText("Main panel knobs remain global threshold / ratio / range / timing macros.",22,514,635,16,juce::Justification::centredLeft);}
        else if(type==Doubler){g.setColour(juce::Colour(0xffc9c1af));g.setFont(font(13.f));g.drawFittedText("Independent wet voices: delay, signed cents, pan and level. Main panel delay, detune, width and sweep rate remain global offsets.",juce::Rectangle<int>(22,237,636,67),juce::Justification::centredLeft,3);}
        else{g.setColour(juce::Colour(0xffc9c1af));g.setFont(font(14.f));g.drawFittedText(type==Resonance?"13 narrow detectors, adaptive peak prominence and a shared reduction budget. Existing panel controls remain fully functional.":"Antiderivative anti-aliasing on the harmonic residual. Existing panel controls remain fully functional. Zero additional host latency.",juce::Rectangle<int>(22,82,636,100),juce::Justification::centredLeft,4);}
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(type!=Multiband||!graph().contains(e.position))return;
        const auto hz=crossovers();float best=12;drag=-1;
        for(int j=0;j<5;++j){const float d=std::abs(e.position.x-xOf(hz[j]));if(d<best){best=d;drag=advCross+j;}}
        for(int j=0;j<2;++j){const float x=xOf(std::min(p.advancedGraphTop(),value(floatingControl(j,0)))),d=std::abs(e.position.x-x);if(d<13&&std::abs(e.position.y-(graph().getY()+25+j*18))<11){drag=floatingControl(j,0);select(j+6);break;}}
        if(drag>=0){parameter(drag)->beginChangeGesture();mouseDrag(e);return;}
        int band=0;while(band<5&&e.position.x>xOf(hz[band]))++band;select(band);
    }
    void mouseDrag(const juce::MouseEvent& e) override {if(drag<0||!valid())return;float hz=hzAt(e.position.x);if(drag>=advCross&&drag<advBand){auto x=crossovers();const int j=drag-advCross;const float upper=j<4?x[j+1]/1.2f:p.advancedGraphTop();hz=std::clamp(hz,std::min(upper,j?x[j-1]*1.2f:30.f),upper);}parameter(drag)->setValueNotifyingHost(toNorm(advancedSpec(drag),std::min(hz,p.advancedGraphTop())));repaint();}
    void mouseUp(const juce::MouseEvent&) override {endDrag();}
    void select(int i){selector.setSelectedId(i+1,juce::sendNotificationSync);}
    bool exerciseBindings(){if(type!=Multiband&&type!=Doubler)return true;bool ok=true;for(int i=0;i<(type==Multiband?8:2);++i){select(i);const int n=type==Doubler?4:i<6?7:9;for(int j=0;j<n;++j){const auto id=bound[j];auto* a=parameter(id);const float previous=a->getValue();sliders[j].setValue(previous<.5?1.:0.,juce::sendNotificationSync);ok=ok&&std::abs(a->getValue()-(previous<.5?1.f:0.f))<1e-5f;const auto k=advancedSpec(id);const auto label=sliders[j].getTextFromValue(sliders[j].getValue());ok=ok&&label.isNotEmpty()&&(k.unit!=Unit::Hz||label.containsIgnoreCase("hz"))&&(k.unit!=Unit::Db||label.containsIgnoreCase("db"))&&(k.unit!=Unit::Ms||label.containsIgnoreCase("ms"))&&(k.unit!=Unit::Pct||label.contains("%"));a->setValueNotifyingHost(previous);}}select(0);return ok;}
private:
    bool valid() const{return p.slotType(s)==type;}
    bool enabled() const{return valid()&&parameter(advMode)->getValue()>.5f;}
    juce::RangedAudioParameter* parameter(int i) const{return p.apvts.getParameter(SkellierProcessor::advancedId(s,i));}
    float value(int i) const{return advancedReal(i,parameter(i)->getValue());}
    juce::Rectangle<float> graph()const{return {22,82,636,184};}
    float xOf(float hz)const{return graph().getX()+graph().getWidth()*std::clamp(std::log(std::max(30.f,hz)/30.f)/std::log(16000.f/30.f),0.f,1.f);}
    float hzAt(float x)const{return 30.f*std::pow(16000.f/30.f,std::clamp((x-graph().getX())/graph().getWidth(),0.f,1.f));}
    std::array<float,5> crossovers()const{std::array<float,5>a{};float prev=25;for(int j=0;j<5;++j){const float upper=p.advancedGraphTop()/std::pow(1.2f,float(4-j));a[j]=std::clamp(value(advCross+j),std::min(upper,prev*1.2f),upper);prev=a[j];}return a;}
    void bind(){for(auto& a:attachments)a.reset();int n=0;if(type==Multiband){for(int j=0;j<7;++j)bound[n++]=bandControl(selected,j);if(selected>=6){bound[n++]=floatingControl(selected-6,0);bound[n++]=floatingControl(selected-6,1);}}else if(type==Doubler)for(int j=0;j<4;++j)bound[n++]=voiceControl(selected,j);for(int j=0;j<9;++j){labels[j].setVisible(j<n);sliders[j].setVisible(j<n);if(j>=n)continue;const auto k=advancedSpec(bound[j]);labels[j].setText(k.label,juce::dontSendNotification);attachments[j]=std::make_unique<juce::SliderParameterAttachment>(*parameter(bound[j]),sliders[j]);sliders[j].textFromValueFunction=[k,type=type,id=bound[j]](double v){return type==Doubler&&(id-advVoice)%4==1?juce::String(toReal(k,float(v)),1)+" ct":knobText(k,float(v));};sliders[j].valueFromTextFunction=[k](const juce::String& text){auto t=text.trim().toLowerCase();float v=t.getFloatValue();if(k.unit==Unit::Hz&&t.contains("khz"))v*=1000;if(k.unit==Unit::Pct)v*=.01f;if(k.unit==Unit::Pan){if(t.startsWith("l"))v=-t.substring(1).getFloatValue()*.01f;else if(t.startsWith("r"))v=t.substring(1).getFloatValue()*.01f;else if(t=="center")v=0;}if(k.unit==Unit::Steps){if(t=="on")v=1;else if(t=="off")v=0;}return toNorm(k,v);};sliders[j].setRange(0,1,k.steps>1?1:0);sliders[j].updateText();}
        if(type==Multiband&&solo.getToggleState())p.bandSolo[size_t(s)].store(enabled()?selected:-1);}
    void endDrag(){if(drag>=0){parameter(drag)->endChangeGesture();drag=-1;}}
    void timerCallback() override {if(!valid()){setEnabled(false);p.bandSolo[size_t(s)].store(-1);}else if(type==Multiband){if(!enabled()){p.bandSolo[size_t(s)].store(-1);solo.setToggleState(false,juce::dontSendNotification);}repaint(graph().toNearestInt());}}
    void paintGraph(juce::Graphics& g){auto r=graph();g.setColour(juce::Colour(0xff0a0e0d));g.fillRoundedRectangle(r,5);g.setColour(juce::Colour(0xff514a3e));g.drawRoundedRectangle(r,5,1);auto hz=crossovers();float left=r.getX();const float baseline=r.getY()+43;const float bottom=r.getBottom()-24;
        for(int j=0;j<6;++j){const float right=j<5?xOf(hz[j]):r.getRight();const auto rect=juce::Rectangle<float>(left,baseline,right-left,bottom-baseline);g.setColour(juce::Colour(j==selected?0xff293c35:0xff16201c));g.fillRect(rect.reduced(1,0));const float gr=enabled()?std::clamp(p.advancedBandReduction(s,j),0.f,24.f):0;g.setColour(juce::Colour(0xffba9161).withAlpha(.76f));g.fillRect(rect.reduced(2,0).withHeight(rect.getHeight()*gr/24));g.setColour(juce::Colour(0xffc5c5b9));g.setFont(font(10.f,true));g.drawFittedText("B"+juce::String(j+1)+"  "+juce::String(gr,1),rect.withHeight(20).toNearestInt(),juce::Justification::centred,1);if(j<5){g.setColour(juce::Colour(0xffb1a380));g.drawVerticalLine(int(right),r.getY()+7,bottom);g.fillRoundedRectangle(right-4,r.getY()+11,8,24,3);g.setFont(font(10.f));g.drawFittedText(hz[j]<1000?juce::String(int(hz[j])):juce::String(hz[j]/1000,1)+"k",juce::Rectangle<int>(int(right)-24,int(r.getBottom())-20,48,18),juce::Justification::centred,1);}left=right;}
        for(int j=0;j<2;++j){float x=xOf(std::min(p.advancedGraphTop(),value(floatingControl(j,0))));const bool on=value(bandControl(j+6,6))>.5f;const float gr=enabled()?p.advancedBandReduction(s,j+6):0;g.setColour(on?juce::Colour(0xff6aaf95):juce::Colour(0xff5b625e));g.drawVerticalLine(int(x),r.getY()+14,bottom);g.fillEllipse(x-8,r.getY()+17+j*18,16,16);g.setColour(juce::Colour(0xffe4dbc6));g.setFont(font(10.f,true));g.drawText(juce::String(j+1),juce::Rectangle<float>(x-8,r.getY()+17+j*18,16,16),juce::Justification::centred);if(on){g.setFont(font(10.f));g.drawText(juce::String(gr,1)+" dB",juce::Rectangle<float>(x-27,bottom-14,54,14),juce::Justification::centred);}}
    }
    SkellierProcessor& p;int s,type,selected=0,drag=-1;juce::ToggleButton mode;juce::ComboBox selector;juce::TextButton solo;std::unique_ptr<juce::ButtonParameterAttachment>modeAttachment;std::array<juce::Slider,9> sliders;std::array<juce::Label,9>labels;std::array<int,9>bound{};std::array<std::unique_ptr<juce::SliderParameterAttachment>,9>attachments;
};
}
