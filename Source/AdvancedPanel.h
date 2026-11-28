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
        setName("Advanced "+juce::String(info(type).name));setSize(680,type==Multiband?536:type==Doubler?342:type==Exciter?330:376);
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
        const int top=type==Multiband?337:type==Doubler?132:250;
        for(int j=0;j<9;++j){const int col=j%2,row=j/2,x=22+col*318,y=top+row*35;labels[j].setBounds(x,y,84,27);sliders[j].setBounds(x+88,y,222,27);}
    }
    void paint(juce::Graphics& g) override {
        const auto area=getLocalBounds().toFloat();g.fillAll(juce::Colour(0xff161918));g.setColour(juce::Colour(0xff766449));g.drawRoundedRectangle(area.reduced(1),6,1);g.setColour(juce::Colour(0xffe5d4b6));g.setFont(font(21.f,true));g.drawText("SKELL  /  "+juce::String(info(type).name).toUpperCase(),22,9,getWidth()-44,30,juce::Justification::centredLeft);
        g.setColour(enabled()?juce::Colour(0xffd6ba7d):juce::Colour(0xff9d9e94));g.setFont(font(12.f,true));g.drawText(enabled()?"ADVANCED ACTIVE":"LEGACY ACTIVE",getWidth()-214,43,192,26,juce::Justification::centredRight);
        if(type==Multiband){paintGraph(g);g.setColour(juce::Colour(0xff9d9e94));g.setFont(font(11.f));g.drawText("Drag split lines / floating nodes, or enter exact Hz below.",22,273,435,18,juce::Justification::centredLeft);g.drawText("GAIN REDUCTION  /  0-24 dB",455,273,203,18,juce::Justification::centredRight);g.setColour(juce::Colour(0xffd6ba7d));g.setFont(font(12.f,true));const float gr=enabled()?std::clamp(p.advancedBandReduction(s,selected),0.f,24.f):0;g.drawText((selected<6?"BAND "+juce::String(selected+1):"FLOATING "+juce::String(selected-5))+"   /   "+juce::String(gr,1)+" dB GR",357,296,301,27,juce::Justification::centredRight);g.drawText("Main panel knobs remain global threshold / ratio / range / timing macros.",22,514,635,16,juce::Justification::centredLeft);}
        else if(type==Doubler){g.setColour(juce::Colour(0xffc9c1af));g.setFont(font(13.f));g.drawFittedText("Independent wet voices: delay, signed cents, pan and level. Main panel delay, detune, width and sweep rate remain global offsets.",juce::Rectangle<int>(22,237,636,67),juce::Justification::centredLeft,3);}
        else paintToneHardware(g);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if(type!=Multiband||!graph().contains(e.position))return;
        const auto hz=crossovers();float best=12;drag=-1;
        for(int j=0;j<5;++j){const float d=std::abs(e.position.x-xOf(hz[j]));if(d<best){best=d;drag=advCross+j;}}
        for(int j=0;j<2;++j){const float x=xOf(std::min(p.advancedGraphTop(),value(floatingControl(j,0)))),d=std::abs(e.position.x-x);if(d<13&&std::abs(e.position.y-(graph().getY()+25+j*18))<11){drag=floatingControl(j,0);select(j+6);break;}}
        if(drag>=0){parameter(drag)->beginChangeGesture();mouseDrag(e);return;}
        int band=0;while(band<5&&e.position.x>xOf(hz[band]))++band;select(band);
    }
    void mouseDrag(const juce::MouseEvent& e) override {if(drag<0||!valid())return;const float n=constrainFrequency(drag,toNorm(advancedSpec(drag),hzAt(e.position.x)));parameter(drag)->setValueNotifyingHost(n);repaint();}
    void mouseUp(const juce::MouseEvent&) override {endDrag();}
    void select(int i){selector.setSelectedId(i+1,juce::sendNotificationSync);}
    bool exerciseBindings(){if(type==Resonance||type==Exciter){bool ok=true;for(int j=0;j<info(type).numKnobs;++j){auto* a=macroParameter(j);const float old=a->getValue(),next=old<.5f?.75f:.25f;sliders[j].setValue(next,juce::sendNotificationSync);ok&=std::abs(a->getValue()-next)<1e-5f;a->setValueNotifyingHost(old);}return ok;}bool ok=true;for(int i=0;i<(type==Multiband?8:2);++i){select(i);const int n=boundCount;for(int j=0;j<n;++j){const auto id=bound[j];auto* a=parameter(id);const float previous=a->getValue();sliders[j].setValue(previous<.5?1.:0.,juce::sendNotificationSync);ok=ok&&std::abs(a->getValue()-(previous<.5?1.f:0.f))<1e-5f;const auto k=advancedSpec(id);const auto label=sliders[j].getTextFromValue(sliders[j].getValue());ok=ok&&label.isNotEmpty()&&(k.unit!=Unit::Hz||label.containsIgnoreCase("hz"))&&(k.unit!=Unit::Db||label.containsIgnoreCase("db"))&&(k.unit!=Unit::Ms||label.containsIgnoreCase("ms"))&&(k.unit!=Unit::Pct||label.contains("%"));a->setValueNotifyingHost(previous);}}select(0);return ok;}
private:
    class FrequencySlider final:public juce::Slider {
    public:
        std::function<double(double)> constrain;
        double snapValue(double attempt,DragMode) override {return constrain?constrain(attempt):attempt;}
    };
    float constrainFrequency(int id,float normalized)const {
        float hz=advancedReal(id,normalized);if(id>=advCross&&id<advBand){const auto x=crossovers();const int j=id-advCross;const float upper=j<4?x[j+1]/1.2f:p.advancedGraphTop();hz=std::clamp(hz,std::min(upper,j?x[j-1]*1.2f:30.f),upper);}return toNorm(advancedSpec(id),std::min(hz,p.advancedGraphTop()));
    }
    juce::RangedAudioParameter* macroParameter(int i) const{return p.apvts.getParameter(SkellierProcessor::knobId(s,i));}
    float macro(int i) const{return toReal(info(type).knobs[i],macroParameter(i)->getValue());}
    void paintToneHardware(juce::Graphics& g){
        const bool res=type==Resonance;const auto ink=juce::Colour(res?0xffd3a4df:0xffe8cb80);const auto face=juce::Rectangle<float>(22,82,636,151);
        g.setGradientFill(juce::ColourGradient::vertical(juce::Colour(0xff242624),face.getY(),juce::Colour(0xff0c100f),face.getBottom()));g.fillRoundedRectangle(face,5);g.setColour(ink.withAlpha(.45f));g.drawRoundedRectangle(face,5,1);
        auto plot=juce::Rectangle<float>(38,112,394,92);g.setColour(ink.withAlpha(.12f));for(int i=0;i<5;++i){g.drawHorizontalLine(int(plot.getY()+i*plot.getHeight()/4),plot.getX(),plot.getRight());g.drawVerticalLine(int(plot.getX()+i*plot.getWidth()/4),plot.getY(),plot.getBottom());}
        g.setFont(font(11.f,true));g.setColour(ink);g.drawText(res?"DETECTOR COVERAGE / SCHEMATIC":"NONLINEAR TRANSFER / SCHEMATIC",38,90,394,18,juce::Justification::centredLeft);
        juce::Path path;for(int i=0;i<=120;++i){const float t=i/120.f;float v=0;if(res){const float f=100.f*std::pow(160.f,t);const float center=std::min(macro(2),p.advancedGraphTop()/std::pow(2.f,1.5f));const float x=std::log2(f/std::max(100.f,center));v=std::exp(-.5f*x*x/(.75f*.75f));}else{const float z=2*t-1,drive=db2g(macro(1));v=.5f+.5f*(std::tanh(z*drive)/drive);}
            const float x=plot.getX()+t*plot.getWidth(),y=plot.getBottom()-v*plot.getHeight();if(i==0)path.startNewSubPath(x,y);else path.lineTo(x,y);}
        g.setColour(ink.withAlpha(.22f));g.strokePath(path,juce::PathStrokeType(5.f));g.setColour(ink);g.strokePath(path,juce::PathStrokeType(1.5f));g.setFont(font(10.f));g.drawText(res?"100 Hz":"-1",38,207,90,16,juce::Justification::centredLeft);g.drawText(res?"FREQUENCY":"INPUT / TRANSFER",128,207,214,16,juce::Justification::centred);g.drawText(res?"16 kHz":"+1",342,207,90,16,juce::Justification::centredRight);
        const auto read=juce::Rectangle<float>(450,96,190,124);g.setColour(juce::Colour(0xff080d0c));g.fillRoundedRectangle(read,4);g.setColour(ink.withAlpha(.4f));g.drawRoundedRectangle(read,4,.8f);g.setColour(ink);g.setFont(font(11.f,true));g.drawText(res?"LIVE GAIN REDUCTION":"RESIDUAL BLEND",read.withHeight(22),juce::Justification::centred);
        const float val=res?std::clamp(p.gainReduction(s),0.f,12.f):macro(2)*100;g.setFont(font(30.f,true));g.drawText(juce::String(val,1)+(res?" dB":" %"),read.withTrimmedTop(23).withHeight(43),juce::Justification::centred);
        auto track=read.reduced(12,0).withY(read.getY()+76).withHeight(8);g.setColour(ink.withAlpha(.14f));g.fillRoundedRectangle(track,2);g.setColour(ink);g.fillRoundedRectangle(track.withWidth(track.getWidth()*std::clamp(val/(res?12.f:100.f),0.f,1.f)),2);
        g.setFont(font(10.f));g.drawText(res?"0 - 12 dB / MEASURED":"BLEND / PARAMETER",read.withY(read.getBottom()-27).withHeight(18),juce::Justification::centred);
    }
    void bindTone(){for(int j=0;j<9;++j){const bool show=j<info(type).numKnobs;labels[j].setVisible(show);sliders[j].setVisible(show);if(!show)continue;auto k=info(type).knobs[j];labels[j].setText(k.label,juce::dontSendNotification);sliders[j].setRange(0,1,0);attachments[j]=std::make_unique<juce::SliderParameterAttachment>(*macroParameter(j),sliders[j]);sliders[j].textFromValueFunction=[k](double v){return knobText(k,float(v));};sliders[j].valueFromTextFunction=[k](const juce::String& text){auto t=text.trim().toLowerCase();float v=t.getFloatValue();if(k.unit==Unit::Hz&&t.contains("khz"))v*=1000;if(k.unit==Unit::Pct)v*=.01f;return toNorm(k,v);};sliders[j].updateText();}}
    bool valid() const{return p.slotType(s)==type;}
    bool enabled() const{return valid()&&parameter(advMode)->getValue()>.5f;}
    juce::RangedAudioParameter* parameter(int i) const{return p.apvts.getParameter(SkellierProcessor::advancedId(s,i));}
    float value(int i) const{return advancedReal(i,parameter(i)->getValue());}
    juce::Rectangle<float> graph()const{return {22,82,636,184};}
    float xOf(float hz)const{return graph().getX()+graph().getWidth()*std::clamp(std::log(std::max(30.f,hz)/30.f)/std::log(16000.f/30.f),0.f,1.f);}
    float hzAt(float x)const{return 30.f*std::pow(16000.f/30.f,std::clamp((x-graph().getX())/graph().getWidth(),0.f,1.f));}
    std::array<float,5> crossovers()const{std::array<float,5>a{};float prev=25;for(int j=0;j<5;++j){const float upper=p.advancedGraphTop()/std::pow(1.2f,float(4-j));a[j]=std::clamp(value(advCross+j),std::min(upper,prev*1.2f),upper);prev=a[j];}return a;}
    void bind(){for(auto& a:attachments)a.reset();if(type==Resonance||type==Exciter){bindTone();return;}int n=0;if(type==Multiband){for(int j=0;j<7;++j)bound[n++]=bandControl(selected,j);if(selected>=6){bound[n++]=floatingControl(selected-6,0);bound[n++]=floatingControl(selected-6,1);}else{if(selected>0)bound[n++]=advCross+selected-1;if(selected<5)bound[n++]=advCross+selected;}}else if(type==Doubler)for(int j=0;j<4;++j)bound[n++]=voiceControl(selected,j);boundCount=n;for(int j=0;j<9;++j){labels[j].setVisible(j<n);sliders[j].setVisible(j<n);sliders[j].constrain={};if(j>=n)continue;const auto k=advancedSpec(bound[j]);if(k.unit==Unit::Hz)sliders[j].constrain=[this,id=bound[j]](double v){return double(constrainFrequency(id,float(v)));};labels[j].setText(bound[j]>=advCross&&bound[j]<advBand?(bound[j]==advCross+selected?"High split":"Low split"):k.label,juce::dontSendNotification);attachments[j]=std::make_unique<juce::SliderParameterAttachment>(*parameter(bound[j]),sliders[j]);sliders[j].textFromValueFunction=[k,type=type,id=bound[j]](double v){return type==Doubler&&(id-advVoice)%4==1?juce::String(toReal(k,float(v)),1)+" ct":knobText(k,float(v));};sliders[j].valueFromTextFunction=[k](const juce::String& text){auto t=text.trim().toLowerCase();float v=t.getFloatValue();if(k.unit==Unit::Hz&&t.contains("khz"))v*=1000;if(k.unit==Unit::Pct)v*=.01f;if(k.unit==Unit::Pan){if(t.startsWith("l"))v=-t.substring(1).getFloatValue()*.01f;else if(t.startsWith("r"))v=t.substring(1).getFloatValue()*.01f;else if(t=="center")v=0;}if(k.unit==Unit::Steps){if(t=="on")v=1;else if(t=="off")v=0;}return toNorm(k,v);};sliders[j].setRange(0,1,k.steps>1?1:0);sliders[j].updateText();}
        if(type==Multiband&&solo.getToggleState())p.bandSolo[size_t(s)].store(enabled()?selected:-1);}
    void endDrag(){if(drag>=0){parameter(drag)->endChangeGesture();drag=-1;}}
    void timerCallback() override {if(!valid()){setEnabled(false);p.bandSolo[size_t(s)].store(-1);}else if(type==Multiband){if(!enabled()){p.bandSolo[size_t(s)].store(-1);solo.setToggleState(false,juce::dontSendNotification);}repaint(graph().toNearestInt());repaint(357,296,301,27);}else if(type==Resonance||type==Exciter)repaint(22,82,636,151);}
    void paintGraph(juce::Graphics& g){auto r=graph();g.setColour(juce::Colour(0xff0a0e0d));g.fillRoundedRectangle(r,5);g.setColour(juce::Colour(0xff514a3e));g.drawRoundedRectangle(r,5,1);auto hz=crossovers();float left=r.getX();const float baseline=r.getY()+65;const float bottom=r.getBottom()-25,gridBottom=bottom-18;
        // Band fills are the actual detector gain-reduction values. A fixed
        // reference scale avoids misleading normalization to the loudest band.
        for(int j=0;j<6;++j){const float right=j<5?xOf(hz[j]):r.getRight();const auto rect=juce::Rectangle<float>(left,baseline,std::max(0.f,right-left),gridBottom-baseline);const bool selectedBand=j==selected;const bool on=value(bandControl(j,6))>.5f;const float gr=enabled()&&on?std::clamp(p.advancedBandReduction(s,j),0.f,24.f):0;g.setColour(juce::Colour(selectedBand?0xff263d34:0xff15211c));g.fillRect(rect.reduced(.6f,0));g.setColour(juce::Colour(0xffb99462).withAlpha(on?.82f:.24f));g.fillRect(rect.reduced(1.5f,0).withHeight(rect.getHeight()*gr/24.f));
            if(selectedBand){g.setColour(juce::Colour(0xff86bca0));g.drawRect(rect.reduced(1),1.f);}if(rect.getWidth()>27){g.setColour(juce::Colour(on?0xffdfd8c5:0xff7a8179));g.setFont(font(10.f,true));g.drawFittedText("B"+juce::String(j+1),rect.withHeight(15).translated(0,-17).toNearestInt(),juce::Justification::centred,1);if(rect.getWidth()>40){g.setFont(font(10.f));g.drawFittedText(on?juce::String(gr,1)+" dB":"OFF",rect.withY(gridBottom+2).withHeight(15).toNearestInt(),juce::Justification::centred,1);}}
            if(j<5){g.setColour(juce::Colour(0xffb1a380));g.drawVerticalLine(int(right),r.getY()+7,bottom);g.fillRoundedRectangle(right-4,r.getY()+8,8,21,3);g.setFont(font(10.f));const auto label=hz[j]<1000?juce::String(int(hz[j])):juce::String(hz[j]/1000,1)+"k";const float prev=j?xOf(hz[j-1]):r.getX();if(right-prev>43)g.drawFittedText(label,juce::Rectangle<int>(int(right)-24,int(r.getBottom())-21,48,18),juce::Justification::centred,1);}left=right;}
        for(int db:{0,6,12,18,24}){const float y=baseline+(gridBottom-baseline)*db/24.f;g.setColour(juce::Colour(0xff9caa9e).withAlpha(.16f));g.drawHorizontalLine(int(y),r.getX()+1,r.getRight()-1);}
        for(int j=0;j<2;++j){float x=xOf(std::min(p.advancedGraphTop(),value(floatingControl(j,0))));const bool on=value(bandControl(j+6,6))>.5f;const float gr=enabled()?p.advancedBandReduction(s,j+6):0;g.setColour(on?juce::Colour(0xff6aaf95):juce::Colour(0xff5b625e));g.drawVerticalLine(int(x),r.getY()+14,gridBottom);g.fillEllipse(x-8,r.getY()+17+j*18,16,16);if(j+6==selected){g.setColour(juce::Colour(0xffb8e1ce));g.drawEllipse(x-10,r.getY()+15+j*18,20,20,1.f);}g.setColour(juce::Colour(0xffe4dbc6));g.setFont(font(10.f,true));g.drawText(juce::String(j+1),juce::Rectangle<float>(x-8,r.getY()+17+j*18,16,16),juce::Justification::centred);if(on){const float y=baseline+(gridBottom-baseline)*std::clamp(gr,0.f,24.f)/24.f;g.setColour(juce::Colour(0xff9cdcc0));g.fillEllipse(x-3,y-3,6,6);g.setColour(juce::Colour(0xff10231c));g.drawEllipse(x-3,y-3,6,6,1.f);}}
    }
    SkellierProcessor& p;int s,type,selected=0,drag=-1,boundCount=0;juce::ToggleButton mode;juce::ComboBox selector;juce::TextButton solo;std::unique_ptr<juce::ButtonParameterAttachment>modeAttachment;std::array<FrequencySlider,9> sliders;std::array<juce::Label,9>labels;std::array<int,9>bound{};std::array<std::unique_ptr<juce::SliderParameterAttachment>,9>attachments;
};
}
