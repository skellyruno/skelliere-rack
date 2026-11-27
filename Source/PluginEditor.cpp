#include "PluginEditor.h"
#include "MeterCalibration.h"

using namespace skl;
using namespace skl::ui;
using juce::Graphics;
using juce::Rectangle;

static juce::String slotLetter(int i) { return juce::String::charToString((juce::juce_wchar) ('A' + i)); }
static int textWidth(const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidthInt(f, s); }

// A single measured aggregate output peak, not invented stereo channels.
static void paintMixHeadPeak(Graphics& g,Rectangle<float> r,float db,bool plate){
    if(plate){
        g.setColour(Colour(0xff291813));g.fillRoundedRectangle(r,5.f);
        g.setColour(Colour(0xffc48c59));g.drawRoundedRectangle(r.reduced(1),4.f,1.f);
        g.setColour(Colour(0xff10140f));g.fillRoundedRectangle(r.reduced(7),3.f);
        g.setColour(Colour(0xffceccb4));g.setFont(font(10.f,true));
        g.drawText("OUTPUT  /  PEAK",r.withY(r.getY()+11).withHeight(16).toNearestInt(),juce::Justification::centred,false);
        const auto track=r.reduced(16,0).withY(r.getY()+39).withHeight(29);
        for(int i=0;i<30;++i){const float step=track.getWidth()/30.f;g.setColour(Colour(0xff262b20));g.fillRoundedRectangle({track.getX()+i*step,track.getY(),step-1.f,track.getHeight()},.7f);}
        const char* labels[]={"-60","-40","-20","-6","0"};const float points[]={0.f,1.f/3.f,2.f/3.f,.9f,1.f};
        g.setFont(font(8.5f));for(int i=0;i<5;++i){const float x=track.getX()+track.getWidth()*points[i];g.setColour(Colour(0xffadac95));g.drawVerticalLine(int(x),track.getBottom()+2,track.getBottom()+6);g.drawText(labels[i],Rectangle<float>(x-12,track.getBottom()+7,24,12).toNearestInt(),juce::Justification::centred,false);}
    }else{
        const auto track=r.reduced(16,0).withY(r.getY()+39).withHeight(29);
        const int lit=juce::jlimit(0,30,int(std::ceil((db+60.f)*.5f)));
        for(int i=0;i<lit;++i){const float step=track.getWidth()/30.f;auto b=Rectangle<float>(track.getX()+i*step,track.getY(),step-1.f,track.getHeight());const auto c=i>=29?Colour(0xffef6252):i>=25?Colour(0xffffc362):Colour(0xff8ed456);g.setGradientFill(juce::ColourGradient::vertical(c.brighter(.35f),b.getY(),c.darker(.55f),b.getBottom()));g.fillRoundedRectangle(b,.7f);}
        g.setColour(Colour(0xffd0e7a1));g.setFont(font(12.f,true));g.drawText((db<=-60.f?juce::String("-INF"):juce::String(db,1))+" dBFS",r.withY(r.getBottom()-29).withHeight(18).toNearestInt(),juce::Justification::centred,false);
    }
}

static void paintHushMeter(Graphics& g,Rectangle<float> r,float db,bool plate){
    auto track=r.reduced(12,0).withTrimmedTop(27).withTrimmedBottom(22);track.setWidth(16.f);
    if(plate){paintHardwarePlate(g,r,Colour(0xff29343a),false,5.f,3.f);paintRecessedWindow(g,track,2);g.setFont(font(9.5f,true));g.setColour(Colour(0xffcbd5cf));g.drawText("GR dB",r.withHeight(24),juce::Justification::centred);for(int i=0;i<=4;++i){float y=track.getY()+track.getHeight()*i*.25f;g.setColour(Colour(0xff80908d));g.drawHorizontalLine(int(y),track.getRight()+3,track.getRight()+8);g.drawText(juce::String(i*6),Rectangle<float>(track.getRight()+10,y-7,22,14),juce::Justification::centredLeft);}}
    else {const float v=juce::jlimit(0.f,1.f,std::abs(db)/24.f);auto bar=track.reduced(2).withHeight(track.reduced(2).getHeight()*v);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffdcffb5),bar.getY(),Colour(0xff2a986b),bar.getBottom()));g.fillRoundedRectangle(bar,1);}
}
// Two independent real meter scales: gain reduction in dB and output peak in dBFS.
static void paintSSLReduction(Graphics& g,Rectangle<float> r,float db,float peak,bool plate){
    if(plate){paintHardwarePlate(g,r,Colour(0xff242725),false,3.f,2.2f);paintRecessedWindow(g,r.reduced(4.f),2.f);return;}
    const float left=r.getX()+8.f,width=r.getWidth()-16.f;const int n=16;const float step=width/n;
    auto line=[&](float y,const char* label,float value,int lit,bool output){
        g.setColour(Colour(0xffc9cfc1));g.setFont(font(7.8f,true));
        g.drawText(label,left,y,width*.62f,11,juce::Justification::centredLeft);
        g.drawText((output&&value<=-60.f)?juce::String("-INF"):juce::String(value,1),left+width*.55f,y,width*.45f,11,juce::Justification::centredRight);
        for(int i=0;i<n;++i){const auto v=Rectangle<float>(left+i*step,y+12,step-.9f,8.f);const bool on=i<lit;const auto c=output?(i>=14?Colour(0xffef6150):i>=11?Colour(0xffd8ae44):Colour(0xff73cb67)):Colour(0xffe6ad51);
            g.setGradientFill(juce::ColourGradient::vertical(on?c.brighter(.32f):Colour(0xff30342a),v.getY(),on?c.darker(.42f):Colour(0xff161915),v.getBottom()));g.fillRoundedRectangle(v,.65f);}
    };
    line(r.getY()+7,"GR dB",std::abs(db)<.05f?0.f:-std::abs(db),juce::jlimit(0,n,int(std::ceil(std::abs(db)*n/20.f))),false);
    line(r.getY()+34,"OUT dBFS",peak,juce::jlimit(0,n,int(std::ceil((peak+60.f)*n/60.f))),true);
}
// Real post-effect aggregate peak, red LED ladder like the rack hardware.
static void paintDopePeak(Graphics& g,Rectangle<float> r,float db,bool plate){
    const auto track=r.reduced(9,0).withY(r.getY()+27).withHeight(9);
    if(plate){paintRecessedWindow(g,r,3.f);g.setColour(Colour(0xffddd7ce));g.setFont(font(10.f,true));g.drawText("OUTPUT LEVEL",r.withY(r.getY()+6).withHeight(16),juce::Justification::centred);return;}
    const int n=32;const float step=track.getWidth()/n;const int lit=juce::jlimit(0,n,int(std::ceil((db+60.f)*n/60.f)));
    for(int i=0;i<n;++i){auto b=Rectangle<float>(track.getX()+i*step,track.getY(),step-.85f,track.getHeight());const auto c=i<lit?Colour(0xffff3e22):Colour(0xff391714);g.setGradientFill(juce::ColourGradient::vertical(c.brighter(.18f),b.getY(),c.darker(.45f),b.getBottom()));g.fillRoundedRectangle(b,.6f);}
    g.setColour(Colour(0xffd9b6a0));g.setFont(font(9.f));g.drawText(db<=-60.f?"-INF dBFS":juce::String(db,1)+" dBFS",r.withY(r.getY()+41).withHeight(18),juce::Justification::centred);
}
// =================================================================== KnobCell
KnobCell::KnobCell(const juce::String& label, juce::RangedAudioParameter& param, Fmt fmt, juce::Colour cap, juce::Colour pointer,
                   double resetValue, double step, bool compact)
    : label_(label.toUpperCase()), fmt_(std::move(fmt)), compact_(compact), attachment(param, slider) {
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
    slider.setMouseDragSensitivity(180);
    slider.setScrollWheelEnabled(false);
    slider.setDoubleClickReturnValue(true, resetValue);
    slider.setTitle(label);
    slider.getProperties().set("cap", (int) cap.getARGB());
    slider.getProperties().set("ptr", (int) pointer.getARGB());
    if (step > 0.0) slider.setRange(slider.getMinimum(), slider.getMaximum(), step);
    slider.onValueChange = [this] { text_ = fmt_(slider.getValue()); repaint(); };
    text_ = fmt_(slider.getValue());
    addAndMakeVisible(slider);
}

void KnobCell::setLightFace(bool light) {
    lightFace_ = light;
    slider.getProperties().set("lightFace", light);
    slider.repaint();
    repaint();
}

void KnobCell::setSprite(int) { slider.setSliderStyle(juce::Slider::RotaryVerticalDrag); }

void KnobCell::resized() {
    if(bool(slider.getProperties().getWithDefault("consoleCell",false))&&!compact_){
        const int size=juce::jmax(1,juce::jmin(getWidth()-2,getHeight()-20,bool(slider.getProperties().getWithDefault("dopeRow",false))?52:47));
        slider.setBounds((getWidth()-size)/2,juce::jmax(0,(getHeight()-size-20)/2),size,size);return;
    }
    if(bool(slider.getProperties().getWithDefault("hushFader",false))&&!compact_){slider.setBounds(5,3,getWidth()-10,getHeight()-36);return;}
    if(compact_){
        const int size=juce::jmax(1,juce::jmin(getHeight(),getWidth(),48));
        slider.setBounds(0,(getHeight()-size)/2,size,size);
    }else if(sideReadout_){
        const int size=juce::jmax(1,juce::jmin(getHeight()-4,getWidth()/2,88));
        slider.setBounds(6,(getHeight()-size)/2,size,size);
    }else{
        // VMR-style control proportions. At 100% a paired metal face is
        // roughly 62px, not 85px; single controls can be slightly larger.
        const int limit=bool(slider.getProperties().getWithDefault("dopeSwitch",false))?76:getWidth()<90?58:getWidth()<125?76:96;
        const int size=juce::jmax(1,juce::jmin(getWidth()-4,getHeight()-32,limit));
        slider.setBounds((getWidth()-size)/2,juce::jmax(0,(getHeight()-size-32)/2),size,size);
    }
}
bool KnobCell::controlGeometryValid() const noexcept {
    const auto b=slider.getBounds();
    if(b.isEmpty()||!getLocalBounds().contains(b))return false;
    if(bool(slider.getProperties().getWithDefault("consoleCell",false)))return b.getWidth()==b.getHeight()&&b.getWidth()<=(bool(slider.getProperties().getWithDefault("dopeRow",false))?52:47)&&b.getBottom()+18<=getHeight();
    if(bool(slider.getProperties().getWithDefault("hushFader",false))&&!compact_)return b.getBottom()+28<=getHeight();
    if(b.getWidth()!=b.getHeight())return false;
    if(compact_)return b.getWidth()<=48;
    if(sideReadout_)return b.getWidth()<=88&&b.getRight()+12<getWidth();
    const int limit=bool(slider.getProperties().getWithDefault("dopeSwitch",false))?76:getWidth()<90?58:getWidth()<125?76:96;
    return b.getWidth()<=limit&&b.getBottom()+28<=getHeight();
}

void KnobCell::paint(Graphics& g) {
    // The slider clips its own paint to its square. Render only the matching
    // outside portion of the cached knob shadow here, behind the child.
    // This keeps the original cap size while allowing a real cast shadow.
    if(!compact_&&!slider.isSwitch()&&!bool(slider.getProperties().getWithDefault("hushFader",false))&&!slider.getBounds().isEmpty()){
        juce::Graphics::ScopedSaveState save(g);
        const auto b=slider.getBounds().toFloat();
        const float capR=b.getWidth()*(b.getWidth()<61.f?.397f:.405f);
        const float shadowR=capR*192.f/132.f;
        g.excludeClipRegion(slider.getBounds());
        const int clipBottom=sideReadout_?getHeight():juce::jmin(getHeight(),slider.getBottom()+3);
        g.reduceClipRegion(0,0,getWidth(),clipBottom);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        g.setOpacity(1.f);
        g.drawImage(capMount(),Rectangle<float>(b.getCentreX()-shadowR,b.getCentreY()-shadowR,2*shadowR,2*shadowR));
    }
    if (compact_) {
        auto r = getLocalBounds().withTrimmedLeft(slider.getRight() + 6);
        g.setColour(pal::ink); g.setFont(font(14.0f,false).withExtraKerningFactor(.085f));
        g.drawText(label_, r.removeFromTop(getHeight() / 2 + 2), juce::Justification::bottomLeft, false);
        g.setColour(pal::ink); g.setFont(font(13.0f));
        g.drawText(text_, r, juce::Justification::topLeft, false);
        return;
    }
    if(sideReadout_) {
        const auto readout=getLocalBounds().withTrimmedLeft(slider.getRight()+12).reduced(5,0);
        const int cy=getHeight()/2;
        g.setColour(lightFace_?Colour(0xff292219):pal::ink);g.setFont(font(16.5f,false).withExtraKerningFactor(.075f));
        g.drawFittedText(label_,readout.withY(cy-24).withHeight(21),juce::Justification::centred,1);
        auto pill=readout.withY(cy+1).withHeight(20).toFloat().reduced(4,0);
        g.setGradientFill(juce::ColourGradient::vertical(lightFace_?Colour(0xff30291f).withAlpha(.22f):Colour(0xff090e12).withAlpha(.72f),pill.getY(),lightFace_?Colour(0xffe0c6a0).withAlpha(.15f):Colour(0xff414a52).withAlpha(.38f),pill.getBottom()));g.fillRoundedRectangle(pill,10);
        g.setColour((lightFace_?Colour(0xff504638):pal::ink).withAlpha(.72f));g.drawRoundedRectangle(pill,10,.85f);
        g.setColour(lightFace_?Colour(0xff292219):Colour(0xffeee3cc));g.setFont(font(14.5f));
        g.drawFittedText(text_,pill.reduced(4,1).toNearestInt(),juce::Justification::centred,1);
        // Only display genuine bipolar endpoints. Bell's sweep and Q are
        // not relabeled as the reference concept's three shelving bands.
        if(rangeLow.isNotEmpty()&&slider.getY()>=8&&getHeight()-slider.getBottom()>=8){
            const auto b=slider.getBounds();
            g.setColour(lightFace_?Colour(0xff423426):Colour(0xffe0d6c3));g.setFont(font(10.5f));
            g.drawText("0",b.getX(),b.getY()-9,b.getWidth(),9,juce::Justification::centred,false);
            g.drawText(rangeLow,b.getX()-3,b.getBottom(),30,9,juce::Justification::centredLeft,false);
            g.drawText(rangeHigh,b.getRight()-27,b.getBottom(),30,9,juce::Justification::centredRight,false);
        }
        return;
    }
    const int y=juce::jmin(getHeight()-30,slider.getBottom()+3);
    if(bool(slider.getProperties().getWithDefault("consoleCell",false))){
        const int yy=juce::jmin(getHeight()-19,slider.getBottom()+1);
        const float fs=bool(slider.getProperties().getWithDefault("dopeRow",false))?10.8f:getWidth()<40?8.3f:9.7f;
        g.setColour(lightFace_?Colour(0xff252322):Colour(0xffe3e1d9));g.setFont(font(fs,true));
        g.drawFittedText(label_,Rectangle<int>(0,yy,getWidth(),10),juce::Justification::centred,1);
        g.setColour(lightFace_?Colour(0xff37332a):Colour(0xffb4c2b9));g.setFont(font(fs));
        g.drawFittedText(text_,Rectangle<int>(0,yy+10,getWidth(),9),juce::Justification::centred,1);return;
    }
    g.setColour(lightFace_?Colour(0xff211b14):Colour(0xffffecd1));g.setFont(font(15.5f,true).withExtraKerningFactor(.055f));
    g.drawFittedText(label_,Rectangle<int>(2,y,getWidth()-4,15),juce::Justification::centred,1);
    g.setColour(lightFace_?Colour(0xff30261b):Colour(0xffeee0c8));g.setFont(font(14.5f));
    g.drawFittedText(text_,Rectangle<int>(2,y+15,getWidth()-4,15),juce::Justification::centred,1);
}

// =================================================================== small displays
void LcdDisplay::paint(Graphics& g) {
    g.setColour(pal::inkDim);g.setFont(font(getWidth()>350?14.5f:10.5f).withExtraKerningFactor(.18f));
    g.drawFittedText(text,getLocalBounds().reduced(3,0),juce::Justification::centredLeft,1);
}

void LevelMeter::paint(Graphics& g) {
    auto r = getLocalBounds().toFloat();
    auto bar = [&](Rectangle<float> a, float db, const juce::String& name) {
        g.setColour(pal::inkDim); g.setFont(font(10.5f,false));
        g.drawText(name, a.removeFromLeft(23.0f), juce::Justification::centredLeft, false);
        paintRecessedWindow(g,a,4.f);a=a.reduced(2.5f,1.2f);
        const int segs = 36;
        const float lit = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f) * (float) segs;
        const float sw = a.getWidth() / (float) segs;
        for (int i = 0; i < segs; ++i) {
            const float pos = (float) i / (float) segs;
            const Colour c=pos>.9f?Colour(0xfff28e31):pos>.72f?Colour(0xffebd05b):Colour(0xff71b54e);
            const bool on=i<(int)lit;
            auto seg=Rectangle<float>(a.getX()+float(i)*sw+.45f,a.getY()+.9f,sw-.9f,a.getHeight()-1.8f);
            g.setGradientFill(juce::ColourGradient::vertical(on?c.brighter(.25f):c.withAlpha(.13f),seg.getY(),on?c.darker(.35f):c.withAlpha(.05f),seg.getBottom()));
            g.fillRoundedRectangle(seg,.65f);
            if(on){g.setColour(c.brighter(.45f).withAlpha(.55f));g.drawHorizontalLine(int(seg.getY())+1,seg.getX()+.4f,seg.getRight()-.4f);}
        }
    };
    bar(r.removeFromTop(r.getHeight() * 0.5f).reduced(0.0f, 2.0f), inDb_, "IN");
    bar(r.reduced(0.0f, 2.0f), outDb_, "OUT");
}

void PowerButton::paintButton(Graphics& g, bool hover, bool down) {
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    const float cx = r.getCentreX(), cy = r.getCentreY(), R = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillEllipse(cx - R, cy - R + 1.5f, 2 * R, 2 * R);
    g.setGradientFill(juce::ColourGradient(Colour(0xffd2bea0),cx-R*.55f,cy-R*.65f,Colour(0xff40382c),cx+R*.65f,cy+R,true));
    g.fillEllipse(cx - R, cy - R, 2 * R, 2 * R);
    g.setColour(Colour(0xff1b1b1b)); g.drawEllipse(cx - R, cy - R, 2 * R, 2 * R, 1.0f);
    const float lr = R * 0.57f;
    if (getToggleState()) {
        g.setColour(pal::amber.withAlpha(0.35f)); g.fillEllipse(cx - lr * 1.8f, cy - lr * 1.8f, lr * 3.6f, lr * 3.6f);
        g.setGradientFill(juce::ColourGradient(Colour(0xfffff7cf), cx - lr * 0.3f, cy - lr * 0.4f, pal::amber.darker(0.4f), cx + lr, cy + lr, true));
    } else {
        g.setColour(Colour(0xff26241d));
    }
    g.fillEllipse(cx - lr, cy - lr, 2 * lr, 2 * lr);
    if (hover || down) { g.setColour(juce::Colours::white.withAlpha(down ? 0.25f : 0.12f)); g.fillEllipse(cx - R, cy - R, 2 * R, 2 * R); }
}

void StripButton::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) {
        juce::PopupMenu m;
        m.addItem(1, "Delete strip");
        juce::Component::SafePointer<StripButton> safeThis(this);
        m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [safeThis](int result) {
            if (result == 1 && safeThis != nullptr && safeThis->onDelete) safeThis->onDelete();
        });
        return;
    }
    juce::TextButton::mouseDown(e);
}

// =================================================================== SlotComponent
SlotComponent::SlotComponent(SkellierProcessor& p, int i) : proc(p), index(i) {
    addAndMakeVisible(powerBtn);
    addAndMakeVisible(artButton);
    artButton.setTooltip("Cycle original Skell metal finishes");
    artButton.onClick=[this]{setArtworkChoice((proc.artworkChoices[static_cast<size_t>(index)].load()+1)%4);};
    powerAtt = std::make_unique<juce::ButtonParameterAttachment>(*proc.apvts.getParameter(SkellierProcessor::onId(i)), powerBtn);
    for (auto* b : {&leftBtn, &rightBtn, &removeBtn, &soloBtn}) addAndMakeVisible(*b);
    leftBtn.onClick = [this] { proc.moveSlot(index, index - 1, false); };
    rightBtn.onClick = [this] { proc.moveSlot(index, index + 1, false); };
    removeBtn.onClick = [this] { proc.removeSlot(index); };
    soloBtn.setClickingTogglesState(true);
    soloBtn.getProperties().set("plain", true);
    soloBtn.onClick = [this] { proc.soloSlot.store(soloBtn.getToggleState() ? index : -1); };
    for(auto* b:{&leftBtn,&rightBtn,&removeBtn,&soloBtn,&artButton}){b->getProperties().set("plain",true);b->getProperties().set("quiet",true);}
    powerBtn.setTooltip("Enable or bypass this effect");
    leftBtn.setTooltip("Move module left"); rightBtn.setTooltip("Move module right");
    removeBtn.setTooltip("Remove module (Undo restores it)"); soloBtn.setTooltip("Solo this slot");
    rebuild();
}

void SlotComponent::rebuild() {
    type = proc.slotType(index);
    cells.clear();
    const bool filled = type != Empty;
    artButton.setVisible(filled&&editing);
    appliedArtworkChoice = -1; panelCache={};
    powerBtn.setVisible(filled);
    for(auto* c:{&leftBtn,&rightBtn,&removeBtn,&soloBtn})c->setVisible(filled&&editing);
    if (filled) {
        const auto& mi = info(type);
        for (int j = 0; j < uiKnobCount(type); ++j) {
            const bool ext=type==SkellSSL&&j>=kMaxKnobs;
            const int extIndex=ext?j-kMaxKnobs:-1;
            Knob k=ext?sslControl(extIndex).knob:mi.knobs[j];
            if(type==SkellSSL){if(j==0)k.label="E/G";if(extIndex==SslThreshold)k.label="THR BASE";if(extIndex==SslRatio)k.label="RATIO";}
            auto* param=proc.apvts.getParameter(ext?SkellierProcessor::sslId(index,extIndex):SkellierProcessor::knobId(index,j));
            auto cell=std::make_unique<KnobCell>(k.label,*param,[k,extIndex,type=type](double n){
                if(type!=SkellSSL)return knobText(k,float(n));
                const float v=extIndex>=0?sslReal(extIndex,float(n)):toReal(k,float(n));
                if(extIndex==SslHP&&v<16.5f)return juce::String("OFF");
                if(extIndex==SslLP&&n<.001)return juce::String("OFF");
                if(k.unit==Unit::Hz){const float hz=extIndex==SslLP?std::max(3000.f,v):v;return hz>=1000?juce::String(hz/1000,1)+"k":juce::String(int(std::lround(hz)));}
                if(k.unit==Unit::Ms)return juce::String(v/1000,2)+"s";
                if(k.unit==Unit::Db)return juce::String(v>0?"+":"")+juce::String(v,1);
                if(k.unit==Unit::Scalar)return juce::String(v,1);
                return knobText(k,float(n));},capColour(mi.cat),pointerColour(mi.cat),double(ext?sslDefault(extIndex):defaultNorm(type,j)),
                                                   k.steps > 1 ? 1.0 / (double) (k.steps - 1) : 0.0);
            if((type==SkellDope||type==SkellMixHead||type==SkellDemon||type==SkellSSL)&&k.steps==2){
                cell->slider.getProperties().set("colorSwitch",true);
                cell->slider.getProperties().set("dopeSwitch",type==SkellDope);
                cell->slider.getProperties().set("switchLabels",juce::String(k.names));
                cell->slider.setTooltip("Click to switch mode");
                cell->slider.setWantsKeyboardFocus(true);
            }
            if(type==SkellDope){cell->slider.getProperties().set("houseTint",juce::String("ff353431"));if(j>0){cell->slider.getProperties().set("consoleCell",true);cell->slider.getProperties().set("dopeRow",true);}}
            if(type==SkellNoise&&j==0){cell->slider.getProperties().set("hushFader",true);cell->slider.setSliderStyle(juce::Slider::LinearVertical);}
            if(type==SkellSSL){
                cell->slider.getProperties().set("consoleCell",true);
                static const unsigned caps[]={0xffb0aba0,0xffb66e30,0xff347ab5,0xff38895d,0xffb63235,0xffa33b36};
                unsigned cap=ext?0xffa5a6a0:caps[j];
                if(extIndex==SslInput||extIndex==SslOutput)cap=0xffb6443c;
                if(extIndex==SslHP||extIndex==SslLP)cap=0xff51865d;
                if(extIndex==SslExpThreshold)cap=0xff41865b;
                if(extIndex==SslThreshold)cap=0xffb63a33;
                cell->slider.getProperties().set("houseTint",juce::String::toHexString(int(cap)));
                if(extIndex==SslThreshold||extIndex==SslRatio)cell->slider.setTooltip("Base setting. COMP macro also lowers threshold and raises ratio (preserves legacy automation).");
                if(extIndex==SslHFBell||extIndex==SslLFBell)cell->slider.setTooltip("Shelf/bell shape in E circuit; G uses its native shelf law.");
                if(extIndex==SslHP||extIndex==SslLP)cell->slider.setTooltip("Filter frequency; turn fully down for OFF.");
            }
            if(k.unit==Unit::Db&&k.lo<0.f&&k.hi>0.f&&std::abs(k.lo+k.hi)<.001f)cell->setBipolarLegend(k.lo,k.hi);
            addAndMakeVisible(*cell);
            cells.push_back(std::move(cell));
        }
    }
    needle = 0.0f; shownNeedle = -1.0f; lastMeterTime=0;outputDb=-60.f;artworkMotion={};
    syncArtwork();
    syncSolo();
    resized();
    repaint();
}

void SlotComponent::setArtworkChoice(int choice) {
    proc.artworkChoices[static_cast<size_t>(index)].store(juce::jlimit(0, 3, choice));
    syncArtwork();
}

void SlotComponent::syncArtwork() {
    const int choice=juce::jlimit(0,3,proc.artworkChoices[static_cast<size_t>(index)].load());
    // The signature Dope panel follows the actual 361/180 switch; explicit
    // user finish selections remain independent and are never rewritten.
    const bool darkDope=type==SkellDope&&choice==1&&proc.apvts.getRawParameterValue(SkellierProcessor::knobId(index,0))->load()>=.5f;
    const int key=choice*2+(darkDope?1:0);
    if(key==appliedArtworkChoice)return;
    appliedArtworkChoice=key;nativeChoice=darkDope?3:choice;panelCache={};
    const auto spec=originalSpec(type,nativeChoice);
    static const char* names[]={"GRAPHITE","SIGNATURE","SILVER","MIDNIGHT"};
    artButton.setButtonText("FINISH");artButton.setTooltip(juce::String("Metal finish: ")+names[choice]+" (appearance only; click to cycle)");
    for(auto& c:cells){
        c->setNativeFace(true,originalLayout(type,preferredWidth(),editing).sideReadouts);c->setLightFace(spec.light);
        c->slider.getProperties().set("cap",(int)spec.cap.getARGB());
        c->slider.getProperties().set("ptr",(int)Colour(0xff191510).getARGB());
        c->slider.getProperties().set("silverCap",choice==2);
        if(type==SkellDope)c->slider.getProperties().set("houseTint",spec.light?juce::String("ff353431"):juce::String());
        c->slider.repaint();
    }
    resized();repaint();
}

bool SlotComponent::artworkLayoutIsValid() const {
    if(type==Empty)return cells.empty();
    if(cells.size()!=static_cast<size_t>(uiKnobCount(type)))return false;
    const auto body=getLocalBounds().withTrimmedTop(100).withTrimmedBottom(editing?46:23);
    if(!powerBtn.isVisible()||leftBtn.isVisible()!=editing||rightBtn.isVisible()!=editing||soloBtn.isVisible()!=editing||removeBtn.isVisible()!=editing||artButton.isVisible()!=editing)return false;
    for(size_t i=0;i<cells.size();++i){
        const auto& c=*cells[i];
        // The central fifth compressor control may extend below the general
        // body, but not into the side fasteners or real power-button bounds.
        const bool makeupBay=!editing&&i==4&&cells.size()==5&&info(type).hasVU&&!vuRect.isEmpty();
        const auto safe=makeupBay?getLocalBounds().reduced(24,0).withTrimmedTop(100).withTrimmedBottom(10):body;
        if(!safe.contains(c.getBounds())||!c.controlGeometryValid()||c.getBounds().intersects(vuRect)||c.getBounds().intersects(footRect)||c.getBounds().intersects(originalLayout(type,getWidth(),editing).detail))return false;
        for(size_t j=0;j<i;++j)if(c.getBounds().intersects(cells[j]->getBounds()))return false;
    }
    if(type==SkellDope){for(int j=2;j<=4;++j){if(cells[j]->getBounds().getCentreY()!=cells[1]->getBounds().getCentreY()||cells[j]->getBounds().getCentreX()-cells[j-1]->getBounds().getCentreX()!=cells[2]->getBounds().getCentreX()-cells[1]->getBounds().getCentreX())return false;}}
    return true;
}

bool SlotComponent::exerciseSpriteBindings() {
    if(cells.empty())return true;
    bool ok=true;
    for(size_t i=0;i<cells.size();++i){
        auto* p=proc.apvts.getParameter(type==SkellSSL&&i>=kMaxKnobs?SkellierProcessor::sslId(index,int(i)-kMaxKnobs):SkellierProcessor::knobId(index,int(i)));
        const float old=p->getValue();
        p->setValueNotifyingHost(0);ok=ok&&std::abs(cells[i]->slider.getValue())<1e-5;
        cells[i]->slider.setValue(1,juce::sendNotificationSync);ok=ok&&std::abs(p->getValue()-1)<1e-5;
        p->setValueNotifyingHost(old);
    }
    auto* on=proc.apvts.getParameter(SkellierProcessor::onId(index));const float old=on->getValue();
    on->setValueNotifyingHost(0);ok=ok&&!powerBtn.getToggleState();
    powerBtn.setToggleState(true,juce::sendNotificationSync);ok=ok&&on->getValue()>.5f;on->setValueNotifyingHost(old);
    return ok;
}

void SlotComponent::syncSolo() { soloBtn.setToggleState(proc.soloSlot.load() == index, juce::dontSendNotification); }
void SlotComponent::syncPowerVisual() {
    const bool on=proc.slotOn(index);
    if(on!=shownPower){shownPower=on;repaint();}
}

void SlotComponent::setOutputPeak(float peak) {
    if(!hasOutputPeak(type)||vuRect.isEmpty())return;
    const float target=peak>1e-5f&&std::isfinite(peak)?juce::jmax(-60.f,20.f*std::log10(peak)):-60.f;
    const float next=juce::jmax(target,outputDb-1.5f);
    if(std::abs(next-outputDb)>.05f){outputDb=next;repaint(vuRect.expanded(2));}
}
void SlotComponent::setMeter(float gr) {
    if (type == Empty || !info(type).hasVU) return;
    const auto now=juce::Time::getMillisecondCounterHiRes();
    const float elapsed=lastMeterTime>0?static_cast<float>((now-lastMeterTime)*.001):1.0f/30.0f;
    lastMeterTime=now;
    needle=smoothReduction(needle,gr,elapsed);
    if (std::fabs(needle - shownNeedle) > 0.04f) {
        shownNeedle = needle;
        repaint(vuRect.expanded(2));
    }
}

void SlotComponent::setEditMode(bool enabled) {
    if(editing==enabled)return;
    editing=enabled;panelCache={};
    for(auto* c:{&leftBtn,&rightBtn,&removeBtn,&soloBtn,&artButton})c->setVisible(type!=Empty&&editing);
    resized();repaint();
}
void SlotComponent::resized() {
    sectionRects.clear();vuRect={};panelBounds={};
    const int w=getWidth(),h=getHeight();footRect=editing?Rectangle<int>(10,h-43,w-20,32):Rectangle<int>(25,h-24,17,17);
    if(type==Empty)return;
    powerBtn.setBounds(editing?Rectangle<int>(16,h-39,25,25):Rectangle<int>(25,h-24,17,17));soloBtn.setBounds(49,h-38,27,23);
    leftBtn.setBounds(82,h-38,21,23);rightBtn.setBounds(106,h-38,21,23);
    artButton.setBounds(130,h-38,62,23);removeBtn.setBounds(w-35,h-38,20,23);
    const auto layout=originalLayout(type,w,editing);vuRect=layout.meter;
    for(size_t i=0;i<cells.size();++i)cells[i]->setBounds(layout.cells[i]);
}

void SlotComponent::paintEmpty(Graphics& g, Rectangle<float> b) {
    paintHardwarePlate(g,b,Colour(0xff252729),true,9.f,3.4f);
    auto inner=b.reduced(13.0f,18.0f);
    g.setColour(Colour(0xff07080a).withAlpha(.42f));
    g.fillRoundedRectangle(inner,4.0f);
    g.setColour(Colour(0xff393d43).withAlpha(.52f));
    g.drawRoundedRectangle(inner.reduced(.5f),4.0f,1.0f);
    for(int i=0;i<4;++i)
        paintScrew(g,i%2?b.getWidth()-9.0f:9.0f,i<2?9.0f:b.getHeight()-9.0f,4.0f,0.7f*(index+i));
    const float cx=b.getCentreX(),cy=b.getCentreY();
    const bool hot=dragOver||isMouseOver(true);
    // Discoverable insertion target, also in library-hidden mode.
    const auto add=Rectangle<float>(cx-22.f,cy-22.f,44.f,44.f);
    g.setColour(hot?Colour(0xff62503b):Colour(0xff30302d));g.fillRoundedRectangle(add,10.f);
    g.setColour(hot?pal::amber:Colour(0xff887e6e));g.drawRoundedRectangle(add,10.f,.9f);
    g.setColour(hot?pal::amber:Colour(0xffb7ad9c));
    g.drawLine(cx-11.0f,cy,cx+11.0f,cy,1.5f);
    g.drawLine(cx,cy-11.0f,cx,cy+11.0f,1.5f);
    g.setFont(font(13.f,false).withExtraKerningFactor(.08f));
    g.drawText(dragOver?"DROP MODULE HERE":"CLICK TO ADD EFFECT",
               Rectangle<float>(10.0f,cy+23.0f,b.getWidth()-20.0f,18.0f),
               juce::Justification::centred,false);
}



juce::Rectangle<int> SlotComponent::artworkMotionBounds() const {
    if(type==DeEss&&!editing)return {16,434,38,62};
    if(type==SkellDeEsserFX)return {24,32,24,29};
    if(type==Empty)return {};
    return originalLayout(type,getWidth(),editing).detail;
}
void SlotComponent::advanceArtworkMotion(float seconds,bool visible,bool enabled,float signal){
    const auto bounds=artworkMotionBounds();
    if(bounds.isEmpty()||!artworkMotion.advance(seconds,visible,enabled,signal))return;
    repaint(bounds.expanded(1));
}
void SlotComponent::paintNativeFace(Graphics& g) {
    const auto p=originalSpec(type,nativeChoice);const auto& mi=info(type);
    const int w=getWidth(),h=getHeight();if(w<=0||h<=0)return;
    const Colour ink=p.light?Colour(0xff292219):Colour(0xfff0e4cd);
    constexpr int rasterScale=2;
    if(!panelCache.isValid()||panelCache.getWidth()!=w*rasterScale||panelCache.getHeight()!=h*rasterScale){
        panelCache=juce::Image(juce::Image::ARGB,w*rasterScale,h*rasterScale,true);Graphics pg(panelCache);
        pg.addTransform(juce::AffineTransform::scale(float(rasterScale)));
        paintOriginalSurface(pg,getLocalBounds().toFloat(),type,nativeChoice);
        if(type==SkellSSL){
            pg.setColour(Colour(0xff111514));pg.fillRect(juce::Rectangle<float>(10,29,float(w-20),48));
            pg.setColour(Colour(0xffa8aba4).withAlpha(.32f));pg.drawHorizontalLine(77,10.f,float(w-10));
            paintMonogram(pg,Rectangle<float>(18,39,15,21),Colour(0xffe0ded2));
            pg.setColour(Colour(0xffe0ded2));pg.setFont(font(16.f,true).withExtraKerningFactor(.045f));pg.drawText("SSL STRIP",40,32,w-53,23,juce::Justification::centredLeft);
            pg.setColour(Colour(0xffaab1a9));pg.setFont(font(9.5f,true).withExtraKerningFactor(.14f));pg.drawText("SKELL / E-G CHANNEL",40,55,w-53,14,juce::Justification::centredLeft);
        }else{
        paintMonogram(pg,Rectangle<float>(w*.5f-11,12,22,29),ink);
        pg.setColour(ink);pg.setFont(font(14.5f,false).withExtraKerningFactor(.22f));
        pg.drawText("SKELL",18,45,w-36,17,juce::Justification::centred,false);
        pg.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultSerifFontName(),25.5f,juce::Font::bold|juce::Font::italic)).withExtraKerningFactor(.045f));
        const auto title=juce::String(displayName(type)).fromFirstOccurrenceOf(" ",false,false).toUpperCase();
        pg.setColour(p.light?Colour(0xfff8e5c5).withAlpha(.60f):juce::Colours::black.withAlpha(.65f));
        pg.drawFittedText(title,Rectangle<int>(14,61,w-28,30),juce::Justification::centred,1);
        pg.setColour(ink);pg.drawFittedText(title,Rectangle<int>(14,60,w-28,30),juce::Justification::centred,1);
        pg.setColour(ink.withAlpha(.58f));
        const int titleWidth=juce::jlimit(70,w-40,textWidth(pg.getCurrentFont(),title)+16);
        pg.drawHorizontalLine(75,20,float(w-titleWidth)*.5f);
        pg.drawHorizontalLine(75,float(w+titleWidth)*.5f,w-20);
        pg.setColour((p.light?juce::Colours::white:juce::Colours::black).withAlpha(.24f));
        pg.drawHorizontalLine(76,20,float(w-titleWidth)*.5f);pg.drawHorizontalLine(76,float(w+titleWidth)*.5f,w-20);
        }
        const auto layout=originalLayout(type,w,editing);
        if(!layout.detail.isEmpty()){if(type==SkellDemon)paintDemonDetail(pg,layout.detail.toFloat());else if(isAirPanel(type))paintAirDetail(pg,layout.detail.toFloat(),type);else if(type==Reverb||type==Chorus)paintSpaceDetail(pg,layout.detail.toFloat(),type);else if(type==Sat||type==SoftClip||type==Eq||type==Tremolo||type==Phaser||type==Stereo||type==Transient)paintWorkshopDetail(pg,layout.detail.toFloat(),type);else paintTapeDetail(pg,layout.detail.toFloat(),type==Delay&&nativeChoice==1);}
        if(type==SkellSSL){
            const int lane=(w-22)/2;
            pg.setColour(Colour(0xff111414));pg.fillRect(9,84,w-18,447);
            pg.setColour(Colour(0xff292d2c));pg.drawVerticalLine(11+lane,84,531);
            auto section=[&](int x,int y,int ww,const char* label){
                pg.setColour(Colour(0xff5c625e).withAlpha(.8f));pg.drawHorizontalLine(y,float(x),float(x+ww));
                pg.setColour(Colour(0xffdfdfd4));pg.setFont(font(9.f,true).withExtraKerningFactor(.025f));
                pg.drawFittedText(label,Rectangle<int>(x+2,y+2,ww-4,15),juce::Justification::centredLeft,1);
            };
            section(10,85,lane-2,"INPUT / OUTPUT");section(10,167,lane-2,"FILTER / CIRCUIT");section(10,249,lane-2,"COMPRESSOR");section(10,385,lane-2,"GATE / EXP");
            section(12+lane,85,lane-2,"HF");section(12+lane,175,lane-2,"HMF");section(12+lane,265,lane-2,"LMF");section(12+lane,355,lane-2,"LF");section(12+lane,445,lane-2,"DYNAMICS / OUT");
            paintSSLReduction(pg,layout.meter.toFloat(),0,-60,true);
        }
        else if(type==SkellDope){
            auto plaque=Rectangle<float>(91,131,float(w-105),72);
            paintHardwarePlate(pg,plaque,Colour(0xff31312e),true,4.f,2.6f);
            paintRecessedWindow(pg,plaque.reduced(6.f),2.f);
            pg.setColour(Colour(0xffd8d8c9));pg.setFont(font(11.f,true));pg.drawText("SKELL",plaque.withY(plaque.getY()+13).withHeight(15),juce::Justification::centred);
            pg.setFont(font(14.f,true));pg.drawText("A-TYPE",plaque.withY(plaque.getY()+29).withHeight(19),juce::Justification::centred);
            pg.setColour(Colour(0xffb6b5a9));pg.setFont(font(8.5f));pg.drawFittedText("DYNAMIC COLOR",plaque.withY(plaque.getY()+49).withHeight(16).toNearestInt(),juce::Justification::centred,1);
            pg.setColour(ink.withAlpha(.35f));pg.drawHorizontalLine(227,16,w-16);pg.setColour(juce::Colours::black.withAlpha(.25f));pg.drawHorizontalLine(228,16,w-16);
            // Simple stamped horizontal control bay and live red LED readout.
            auto bay=Rectangle<float>(9,232,float(w-18),94);pg.setColour(ink.withAlpha(.27f));pg.drawRect(bay,.8f);
            paintDopePeak(pg,layout.meter.toFloat(),-60,true);
            pg.setColour(ink.withAlpha(.55f));pg.setFont(font(10.f,true).withExtraKerningFactor(.11f));pg.drawText("DUAL CIRCUIT",14,473,w-28,16,juce::Justification::centred);
            pg.setColour(ink.withAlpha(.40f));pg.setFont(font(9.f));pg.drawText("SPECTRAL 4",14,491,w-28,14,juce::Justification::centred);

        }
        else if(type==SkellNoise&&!layout.meter.isEmpty())paintHushMeter(pg,layout.meter.toFloat(),0,true);
        else if(type==SkellMixHead&&!layout.meter.isEmpty())paintMixHeadPeak(pg,layout.meter.toFloat(),-60.f,true);
        else if(!layout.meter.isEmpty())paintVU(pg,layout.meter.toFloat(),0.f,hasOutputPeak(type),MeterLayer::plate);
        // Subtle engraved separators on five-control hardware, outside all controls.
        if(mi.numKnobs==5&&!layout.meter.isEmpty()){
            const float y=float(layout.cells[4].getCentreY());
            pg.setColour(ink.withAlpha(.34f));pg.drawHorizontalLine(int(y),24.f,float(layout.cells[4].getX()-8));
            pg.drawHorizontalLine(int(y),float(layout.cells[4].getRight()+8),float(w-24));
        }
        // Sibilance: a small etched bone figure sits outside the centred lower control.
        if((type==DeEss||type==SkellDeEsserFX)&&!editing&&mi.numKnobs==3){
            // A tactile etched grille, deliberately static, aligned with the original figure.
            for(int row=0;row<6;++row)for(int col=0;col<4;++col){const float xx=float(w)-45.f+col*5.f,yy=449.f+row*5.f;pg.setColour(juce::Colours::black.withAlpha(.35f));pg.fillEllipse(xx,yy+.65f,2.f,2.f);pg.setColour(ink.withAlpha(.40f));pg.fillEllipse(xx,yy,1.5f,1.5f);}
        }
        // Side-mounted value pills provide the separation; no extra grid lines.
        if(editing){pg.setColour(ink.withAlpha(.24f));pg.drawHorizontalLine(h-52,20,w-20);
            pg.setColour(ink.withAlpha(.68f));pg.setFont(font(8.5f,true));
            pg.drawText(slotLetter(index)+"  /  "+p.series,14,h-49,w-28,8,juce::Justification::centred,false);}
    }
    g.setOpacity(1.f);g.drawImage(panelCache,getLocalBounds().toFloat());
    const auto detail=originalLayout(type,w,editing).detail;
    if(!detail.isEmpty()&&g.clipRegionIntersects(detail)){
        // Repaint only the optical scene; the heavy chassis/faceplate stays cached.
        juce::Graphics::ScopedSaveState scene(g);g.reduceClipRegion(detail);
        const auto r=detail.toFloat();const auto& m=artworkMotion;
        if(type==SkellDemon)paintDemonDetail(g,r,m.phase,m.level,false);else if(isAirPanel(type))paintAirDetail(g,r,type,m.phase,m.level,false);
        else if(type==Reverb||type==Chorus)paintSpaceDetail(g,r,type,m.phase,m.level,false);
        else if(type==Delay||type==Slap)paintTapeDetail(g,r,type==Delay&&nativeChoice==1,m.left,m.right,m.phase);
        else paintWorkshopDetail(g,r,type,m.phase,m.level,false);
    }
    if(type==DeEss&&!editing)
        paintEngravedFigure(g,{20.f,438.f,30.f,53.f},ink.withAlpha(.65f),BonePose::stand,artworkMotion.phase);
    if(type==SkellDeEsserFX)
        paintEngravedFigure(g,{27.f,34.f,18.f,24.f},ink.withAlpha(.72f),BonePose::stand,artworkMotion.phase);
    if(type==SkellSSL&&!vuRect.isEmpty()){paintSSLReduction(g,vuRect.toFloat(),needle,outputDb,false);}
    else if(type==SkellDope&&!vuRect.isEmpty()){paintDopePeak(g,vuRect.toFloat(),outputDb,false);}
    else if(type==SkellNoise&&!vuRect.isEmpty()){paintHushMeter(g,vuRect.toFloat(),needle,false);}
    else if(type==SkellMixHead&&!vuRect.isEmpty()){
        paintMixHeadPeak(g,vuRect.toFloat(),outputDb,false);
    }else if(hasOutputPeak(type)&&!vuRect.isEmpty()){
        paintVU(g,vuRect.toFloat(),outputDb,true,MeterLayer::needle);

    }else if(mi.hasVU&&!vuRect.isEmpty()){
        paintVU(g,vuRect.toFloat(),needle,false,MeterLayer::needle);

    }
    if(!powerBtn.getToggleState()){g.setColour(juce::Colours::black.withAlpha(.22f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(3),5);}
}

void SlotComponent::paintFaceplate(Graphics& g,Rectangle<float>) {paintNativeFace(g);}

void SlotComponent::paint(Graphics& g) {
    auto b = getLocalBounds().toFloat();
    if (type == Empty) paintEmpty(g, b); else paintFaceplate(g, b);
    if (dragOver) { g.setColour(pal::amber); g.drawRect(b.reduced(2.0f), 2.0f); }
}

void SlotComponent::mouseDown(const juce::MouseEvent& e) {
    dragging=false;
    if(type!=Empty&&!e.mods.isPopupMenu())return;
    // Empty slots can add effects without opening the module library.
    // Popups are asynchronous; do not retain an unsafe editor pointer.
    juce::PopupMenu menu;
    if(type==Empty){
        const int cats[]={CatDynamics,CatTone,CatSpace,CatUtility};
        const char* labels[]={"Dynamics","Tone / EQ","Modulation / Space","Utility"};
        for(int i=0;i<4;++i){
            juce::PopupMenu group;
            for(int t=1;t<kNumRackTypes;++t)if(info(t).cat==cats[i])
                group.addItem(100+t,displayName(t));
            menu.addSubMenu(labels[i],group);
        }
    }else{
        menu.addItem(1,proc.slotOn(index)?"Bypass effect":"Enable effect");
        menu.addItem(2,proc.soloSlot.load()==index?"Exit solo":"Solo effect");
        menu.addItem(3,"Move left",index>0);
        menu.addItem(4,"Move right",index<kNumSlots-1);
        menu.addItem(5,"Duplicate to first empty slot");
        menu.addSeparator();menu.addItem(6,"Remove effect");
    }
    juce::Component::SafePointer<SlotComponent> safe(this);
    const int originalType=type;
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),[safe,originalType](int choice){
        if(safe==nullptr||choice==0||safe->proc.slotType(safe->index)!=originalType)return;
        auto& p=safe->proc;const int slot=safe->index;
        if(choice>=101&&choice<100+kNumRackTypes){
            if(!p.addModule(choice-100,slot)&&safe->onNotice)safe->onNotice("Rack full");
        }else if(choice==1){
            p.apvts.getParameter(SkellierProcessor::onId(slot))->setValueNotifyingHost(p.slotOn(slot)?0.f:1.f);
        }else if(choice==2)p.soloSlot.store(p.soloSlot.load()==slot?-1:slot);
        else if(choice==3||choice==4)p.moveSlot(slot,slot+(choice==3?-1:1),false);
        else if(choice==5){
            int free=-1;for(int i=0;i<kNumSlots;++i)if(p.slotType(i)==Empty){free=i;break;}
            if(free>=0){p.moveSlot(slot,free,true);if(safe->onSelected)safe->onSelected(free);}
            else if(safe->onNotice)safe->onNotice("Rack full — remove an effect to duplicate");
        }else if(choice==6)p.removeSlot(slot);
    });
}

void SlotComponent::mouseDrag(const juce::MouseEvent& e) {
    if (type == Empty || dragging || e.getMouseDownY() > 44 || e.getDistanceFromDragStart() < 6) return;
    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor(this)) { dragging = true; c->startDragging("slot:" + juce::String(index), this); }
}

bool SlotComponent::isInterestedInDragSource(const SourceDetails& d) {
    const auto s = d.description.toString();
    return s.startsWith("mod:") || s.startsWith("slot:");
}

void SlotComponent::itemDropped(const SourceDetails& d) {
    dragOver = false; dragging = false; repaint();
    const auto s = d.description.toString();
    if (s.startsWith("mod:")) {if(!proc.addModule(s.substring(4).getIntValue(), index)&&onNotice)onNotice("Cannot insert — no empty slot to the right");}
    else if (s.startsWith("slot:")) proc.moveSlot(s.substring(5).getIntValue(), index, juce::ModifierKeys::currentModifiers.isAltDown());
}

RackStrip::RackStrip(SkellierProcessor& p) {
    for (int i = 0; i < kNumSlots; ++i) {
        slots[(size_t) i] = std::make_unique<SlotComponent>(p, i);
        addAndMakeVisible(*slots[(size_t) i]);
    }
    resized();
}

void RackStrip::resized() {
    int x=kPad;
    for(auto& slot:slots) if(slot) {
        const int w=rackgeo::width(libraryMode);
        slot->setBounds(x,2,w,SlotComponent::kHeight);
        x+=w+kGap;
    }
    const int total=x-kGap+kPad;
    if(getWidth()!=total || getHeight()!=SlotComponent::kHeight+4)
        setSize(total,SlotComponent::kHeight+4);
}

// =================================================================== library
ModuleTile::ModuleTile(int t, std::function<void(int)> sel, std::function<void(int)> add)
    : type(t), onSelect(std::move(sel)), onAdd(std::move(add)) {}

void ModuleTile::paint(Graphics& g) {
    const auto& mi=info(type);
    auto r=getLocalBounds().toFloat().reduced(.5f);
    const bool active=selected,hover=isMouseOver();
    g.setGradientFill(juce::ColourGradient::vertical(Colour(active?0xff51412c:hover?0xff302b25:0xff1d1b18),r.getY(),Colour(active?0xff251c13:0xff14120f),r.getBottom()));g.fillRect(r);
    // Continuous recessed list, with a fine separator rather than a thick box around each row.
    g.setColour(Colour(0xff090807));g.drawHorizontalLine(int(r.getBottom())-1,r.getX()+3,r.getRight()-3);
    if(active){g.setColour(pal::amber);g.drawRoundedRectangle(r,4,1.2f);}
    g.setColour(juce::Colours::white.withAlpha(hover?.18f:.055f));g.drawHorizontalLine(int(r.getY())+1,r.getX()+4,r.getRight()-4);
    auto sw=r.reduced(6).removeFromLeft(44);
    g.setColour(juce::Colours::black.withAlpha(.55f));g.fillRoundedRectangle(sw.expanded(1.2f).translated(0,1),4);
    // Cache icon at fixed logical dimensions (not the whole tile or its hover state).
    static std::array<juce::Image,skl::kNumRackTypes> minis;
    auto& mini=minis[static_cast<size_t>(type)];
    if(!mini.isValid()){
        mini=juce::Image(juce::Image::ARGB,84,70,true);Graphics mg(mini);
        mg.addTransform(juce::AffineTransform::scale(2.f));paintOriginalMini(mg,Rectangle<float>(0,0,42,35),type);
    }
    g.setOpacity(1.f);g.drawImage(mini,sw);
    auto text=r.reduced(5).withTrimmedLeft(57);
    g.setColour(pal::ink);g.setFont(font(17.f,true).withExtraKerningFactor(.012f));
    g.drawFittedText(juce::String(displayName(type)),text.removeFromTop(21).toNearestInt(),juce::Justification::centredLeft,1);
    g.setColour(Colour(0xffd8cbb7));g.setFont(font(12.5f));
    g.drawFittedText(mi.sub,text.toNearestInt(),juce::Justification::centredLeft,1);
}

void ModuleTile::mouseDrag(const juce::MouseEvent& e) {
    if (e.getDistanceFromDragStart() < 6) return;
    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor(this))
        if (!c->isDragAndDropActive()) c->startDragging("mod:" + juce::String(type), this);
}

LibraryPanel::LibraryPanel(SkellierProcessor& p) : proc(p) {
    static const char* names[5] = {"ALL", "DYN", "EQ", "FX", "UTIL"};
    static const int map[5] = {-1, CatDynamics, CatTone, CatSpace, CatUtility};
    for (int i = 0; i < 5; ++i) {
        cats[i].setButtonText(names[i]);
        cats[i].setClickingTogglesState(true);
        cats[i].setRadioGroupId(31);
        cats[i].getProperties().set("plain", true);
        cats[i].onClick = [this, i] { category = map[i]; refresh(); };
        addAndMakeVisible(cats[i]);
    }
    cats[0].setToggleState(true, juce::dontSendNotification);
    search.setTextToShowWhenEmpty("Search modules...", pal::inkDim);
    search.setFont(font(14.5f));search.setIndents(29,6);search.getProperties().set("skellSearch",true);
    search.onTextChange = [this] { refresh(); };
    addAndMakeVisible(search);
    tileViewport.setViewedComponent(&tileContent, false);
    tileViewport.setScrollBarsShown(true, false);
    tileViewport.setScrollBarThickness(6);
    addAndMakeVisible(tileViewport);
    std::vector<int> order{Comp,Bell,Delay,Flanger,Eq,DeEss,Sat,Air};
    for(int t=Eq;t<kNumRackTypes;++t)if(std::find(order.begin(),order.end(),t)==order.end())order.push_back(t);
    for(int t:order) {
        auto* tile = new ModuleTile(t, [this](int type) { select(type); }, [this](int type) { proc.addModule(type, -1); });
        tiles.add(tile);
        tileContent.addAndMakeVisible(tile);
    }
    infoLabel.setFont(font(12.f));
    infoLabel.setColour(juce::Label::textColourId, pal::inkDim);
    infoLabel.setJustificationType(juce::Justification::topLeft);
    infoLabel.setMinimumHorizontalScale(1.0f);
    infoLabel.setText(juce::String(skl::kNumRackTypes-1)+" original Skell effects. Double-click or drag a module into the rack.",juce::dontSendNotification);
    addAndMakeVisible(infoLabel);
    refresh();
}

void LibraryPanel::select(int type) {
    selectedType = type;
    for (auto* t : tiles) { t->selected = t->type == type; t->repaint(); }
    const auto& mi = info(type);
    const juce::String hint = (type == Eq || type == Air)
        ? "\n\nChoose a Skell metal finish at the bottom of the slot. All knobs remain active."
        : "\n\nDouble-click or drag into a slot.";
    infoLabel.setTooltip(juce::String(mi.desc)+hint);
    infoLabel.setText(juce::String(displayName(type))+"\n"+mi.sub+"\nDouble-click or drag to add.",juce::dontSendNotification);
}

void LibraryPanel::refresh() {
    const auto q = search.getText().trim().toLowerCase();
    int n = 0;
    const int w=juce::jmax(80,tileViewport.getWidth()-6);
    for (auto* t : tiles) {
        const auto& mi = info(t->type);
        const bool show = (category < 0 || (int) mi.cat == category) &&
                          (q.isEmpty() || (juce::String(displayName(t->type)) + " " + mi.sub).toLowerCase().contains(q));
        t->setVisible(show);
        if(show){t->setBounds(0,n*rowHeight,w,rowHeight-1);++n;}
    }
    tileContent.setSize(w,n*rowHeight);
}

void LibraryPanel::resized() {
    hardwareCache={};
    const int w=getWidth();search.setBounds(23,53,w-46,34);
    const int bw=(w-46-12)/5;
    for(int k=0;k<5;++k)cats[k].setBounds(23+k*(bw+3),100,bw,29);
    const int listHeight=juce::jmax(rowHeight,((getHeight()-listTop-52)/rowHeight)*rowHeight);
    tileViewport.setBounds(18,listTop,w-36,listHeight);
    infoLabel.setBounds(22,listTop+listHeight+8,w-44,juce::jmax(0,getHeight()-listTop-listHeight-22));
    refresh();
}

void LibraryPanel::paint(Graphics& g) {
    if(getWidth()<=0||getHeight()<=0)return;
    if(!hardwareCache.isValid()){
        hardwareCache=juce::Image(juce::Image::ARGB,getWidth()*2,getHeight()*2,true);
        Graphics cached(hardwareCache);cached.addTransform(juce::AffineTransform::scale(2.f));paintHardware(cached);
    }
    g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);g.setOpacity(1.f);g.drawImage(hardwareCache,getLocalBounds().toFloat());
}
void LibraryPanel::paintHardware(Graphics& g) {
    auto r=getLocalBounds().toFloat().reduced(.5f);
    paintHardwarePlate(g,r,Colour(0xff24201d),true,11.f,4.5f);
    g.setColour(pal::ink);g.setFont(font(17.5f,false).withExtraKerningFactor(.15f));
    g.drawText("MODULE LIBRARY",r.withY(15).withHeight(30),juce::Justification::centred,false);
    auto list=r.reduced(16).withY(float(listTop-2)).withHeight(float(tileViewport.getHeight()+4));
    paintRecessedWindow(g,list,8.f);
    for(int k=0;k<4;++k)paintScrew(g,k%2?r.getRight()-12:r.getX()+12,k<2?r.getY()+12:r.getBottom()-12,6.f,k*.5f);
}

PathStrip::PathStrip(SkellierProcessor& p):proc(p){
    for(auto* b:{&previous,&next})addAndMakeVisible(*b);
    previous.setTooltip("Previous rack page");next.setTooltip("Next rack page");
    previous.onClick=[this]{if(onPage)onPage(-1);};
    next.onClick=[this]{if(onPage)onPage(1);};
}
void PathStrip::resized(){
    previous.setBounds(20,31,26,23);next.setBounds(145,31,26,23);
    updateHitRects();
}
void PathStrip::setVisibleRange(int first,int last){
    const auto text=slotLetter(first)+" - "+slotLetter(last);
    if(text!=rangeText){rangeText=text;repaint(18,30,155,25);}
    previous.setEnabled(first>0);next.setEnabled(last<kNumSlots-1);
}
void PathStrip::updateHitRects(){
    hitRects.fill(juce::Rectangle<int>());
    int count=0;for(int i=0;i<kNumSlots;++i)if(proc.slotType(i)!=Empty)++count;
    const auto geo=rackgeo::signalPath(getWidth(),count);
    const int arrow=geo.arrow,cell=geo.cell;int x=geo.x+geo.port+arrow;
    for(int i=0;i<kNumSlots;++i)if(proc.slotType(i)!=Empty){
        hitRects[static_cast<size_t>(i)]={x,getHeight()/2-16,cell,32};x+=cell+arrow;
    }
}
int PathStrip::slotAt(juce::Point<int> point) const{
    for(int i=0;i<kNumSlots;++i)if(hitRects[static_cast<size_t>(i)].contains(point))return i;
    return -1;
}
void PathStrip::mouseDown(const juce::MouseEvent& e){
    updateHitRects();const int slot=slotAt(e.getPosition());
    if(slot>=0&&onNavigate)onNavigate(slot);
}
void PathStrip::paint(Graphics& g) {
    updateHitRects();
    const int h=getHeight();g.setColour(pal::ink);g.setFont(font(15.5f,false).withExtraKerningFactor(.12f));
    g.drawText("SIGNAL PATH",36,5,157,25,juce::Justification::centredLeft,false);
    g.setColour(pal::inkDim);g.setFont(font(12,true));g.drawText(rangeText,48,31,94,23,juce::Justification::centred,false);
    g.setColour(Colour(0xff8b7d66).withAlpha(.65f));g.drawVerticalLine(199,8,h-8);
    const bool bypass=proc.apvts.getRawParameterValue("bypass")->load()>.5f;
    const int solo=proc.soloSlot.load();
    int count=0;for(int s=0;s<kNumSlots;++s)if(proc.slotType(s)!=Empty)++count;
    const auto geo=rackgeo::signalPath(getWidth(),count);
    const int port=geo.port,arrowWidth=geo.arrow,cell=geo.cell;int x=geo.x;
    auto key=[&](const juce::String& text,int width,bool lit,bool model,bool focused=false){
        auto r=Rectangle<float>(float(x),float(h)*.5f-16.f,float(width),32.f);
        g.setColour(juce::Colours::black.withAlpha(.76f));g.fillRoundedRectangle(r.translated(0,2),6);
        paintRaisedKey(g,r,Colour(focused?0xff49392a:lit?0xff292622:0xff211e1b),focused,false);
        if(focused){for(int halo=5;halo>=1;--halo){g.setColour(Colour(0xffeeb66e).withAlpha(.028f*float(6-halo)));g.drawRoundedRectangle(r.expanded(float(halo)),7.f,float(halo));}}
        g.setColour(focused?Colour(0xffffdfaa):lit?Colour(0xff96836a):Colour(0xff575043));g.drawRoundedRectangle(r.reduced(.7f),5,focused?1.55f:.7f);
        if(model&&lit&&!focused){g.setColour(Colour(0xffa58c66));g.fillEllipse(r.getX()+6,r.getCentreY()-1.3f,2.6f,2.6f);}
        g.setColour(lit?pal::ink:pal::inkDim);g.setFont(font(count>6?11.5f:16.5f,false).withExtraKerningFactor(.025f));
        g.drawFittedText(text,r.reduced(model?9.f:3.f,1.f).toNearestInt(),juce::Justification::centred,1);x+=width;
    };
    auto arrow=[&]{g.setColour(pal::inkDim);juce::Path p;const float cy=h*.5f,cx=x+arrowWidth*.5f;
        p.startNewSubPath(cx-2.5f,cy-4);p.lineTo(cx+1.5f,cy);p.lineTo(cx-2.5f,cy+4);g.strokePath(p,juce::PathStrokeType(1.2f));x+=arrowWidth;};
    key("IN",port,true,false);
    for(int s=0;s<kNumSlots;++s){const int t=proc.slotType(s);if(t==Empty)continue;arrow();
        key(juce::String(displayName(t)),cell,!bypass&&proc.slotOn(s)&&(solo<0||solo==s),true,s==focusedSlot);}
    arrow();key("OUT",port,true,false);
}

// =================================================================== RackUI
static juce::String fmtDb(double v) { return juce::String(v > 0.05 ? "+" : "") + juce::String(v, 1) + " dB"; }

RackUI::RackUI(SkellierProcessor& p)
    : proc(p),
      inKnob("Input", *p.apvts.getParameter("in"), fmtDb, Colour(0xffc9cbce), Colour(0xff17181a), 0.0, 0.0, true),
      outKnob("Output", *p.apvts.getParameter("out"), fmtDb, Colour(0xffc9cbce), Colour(0xff17181a), 0.0, 0.0, true),
      library(p), strip(p), pathStrip(p) {
    setSize(kW, kH);
    for (auto* b : {&prevPreset, &nextPreset, &snapA, &snapB, &snapCopy, &undoBtn, &redoBtn, &clearBtn, &bypassBtn}) addAndMakeVisible(*b);
    for (int i = 0; i < SkellierProcessor::numFactory(); ++i) presetBox.addItem(SkellierProcessor::factoryName(i), i + 1);
    presetBox.setSelectedItemIndex(0, juce::dontSendNotification);
    presetBox.onChange = [this] {
        curPreset = juce::jmax(0, presetBox.getSelectedItemIndex());
        proc.applyFactory(curPreset);
        say("Rack preset: " + SkellierProcessor::factoryName(curPreset));
    };
    addAndMakeVisible(presetBox);
    auto stepPreset = [this](int d) {
        const int n = SkellierProcessor::numFactory();
        curPreset = (curPreset + d + n) % n;
        presetBox.setSelectedItemIndex(curPreset, juce::sendNotificationSync);
    };
    prevPreset.onClick = [stepPreset] { stepPreset(-1); };
    nextPreset.onClick = [stepPreset] { stepPreset(1); };
    for (auto* b : {&snapA, &snapB}) { b->setClickingTogglesState(true); b->setRadioGroupId(41); b->getProperties().set("plain", true); }
    snapA.setToggleState(true, juce::dontSendNotification);
    snapA.onClick = [this] { proc.switchSnapshot(0); say("Snapshot A"); };
    snapB.onClick = [this] { proc.switchSnapshot(1); say("Snapshot B"); };
    snapCopy.onClick = [this] { proc.copySnapshotToOther(); say(proc.currentSnapshot() == 0 ? "Copied A to B" : "Copied B to A"); };
    undoBtn.setButtonText("UNDO");undoBtn.setTooltip("Undo rack change");
    undoBtn.onClick = [this] { proc.undo(); say("Undo"); };
    redoBtn.onClick = [this] { proc.redo(); say("Redo"); };
    clearBtn.onClick = [this] { proc.clearRack(); say("Rack cleared. Undo brings it back."); };
    bypassBtn.setClickingTogglesState(true);
    bypassAtt = std::make_unique<juce::ButtonParameterAttachment>(*proc.apvts.getParameter("bypass"), bypassBtn);
    static const char* zt[5] = {"80%", "90%", "100%", "125%", "150%"};
    static const float zv[5] = {0.8f, 0.9f, 1.0f, 1.25f,1.5f};
    for (int i = 0; i < 5; ++i) {
        scaleBtn[i].setButtonText(zt[i]);
        scaleBtn[i].setClickingTogglesState(true);
        scaleBtn[i].setRadioGroupId(42);
        scaleBtn[i].getProperties().set("plain", true);
        scaleBtn[i].onClick = [this, i] { if (onScale) onScale(zv[i]); };
        addAndMakeVisible(scaleBtn[i]);
    }
    scaleBtn[2].setToggleState(true, juce::dontSendNotification);
    lcd.setText("SKELL AUDIO    /    ANALOG-STYLE PROCESSORS");
    addAndMakeVisible(lcd); addAndMakeVisible(meters); addAndMakeVisible(inKnob); addAndMakeVisible(outKnob);
    stripsLabel.setText("DREAM STRIPS", juce::dontSendNotification);
    stripsLabel.setFont(font(12.0f, true));
    stripsLabel.setColour(juce::Label::textColourId, pal::inkDim);
    addChildComponent(stripDrawer);
    addAndMakeVisible(stripsToggle);addAndMakeVisible(editToggle);
    editToggle.setClickingTogglesState(true);editToggle.getProperties().set("plain",true);
    editToggle.setTooltip("Show rack editing, snapshots, undo/redo, finishes and zoom controls");
    editToggle.onClick=[this]{setEditMode(editToggle.getToggleState());};
    stripsToggle.setClickingTogglesState(true);stripsToggle.getProperties().set("plain",true);
    stripsToggle.onClick=[this]{stripDrawer.setVisible(stripsToggle.getToggleState());if(stripDrawer.isVisible())stripDrawer.toFront(false);};
    stripDrawer.addAndMakeVisible(stripsLabel);
    defaultStrip.onClick = [this] { curPreset = 0; presetBox.setSelectedItemIndex(0, juce::sendNotificationSync); };
    stripDrawer.addAndMakeVisible(defaultStrip);
    stripName.setTextToShowWhenEmpty("Name this rack", pal::inkDim);
    stripName.setFont(font(13.0f));
    stripDrawer.addAndMakeVisible(stripName);
    saveStripBtn.onClick = [this] {
        const auto name = stripName.getText().trim();
        if (name.isEmpty()) { say("Type a name for the strip first."); return; }
        if (proc.saveStrip(name)) { stripName.clear(); rebuildStripButtons(); say("Saved dream strip: " + name); }
        else say("Could not save the strip. Check folder permissions.");
    };
    stripDrawer.addAndMakeVisible(saveStripBtn);
    rebuildStripButtons();
    addAndMakeVisible(library);
    libraryToggle.setComponentID("rack.library.toggle");
    libraryToggle.setWantsKeyboardFocus(true);
    libraryToggle.setTooltip("Hide the module library to see five full rack slots");
    libraryToggle.onClick=[this]{setLibraryVisible(!libraryVisible);};
    addAndMakeVisible(libraryToggle);
    viewport.setViewedComponent(&strip, false);
    viewport.setScrollBarsShown(false, true);
    viewport.setScrollBarThickness(8);
    addAndMakeVisible(viewport);
    addAndMakeVisible(pathStrip);
    pathStrip.onNavigate=[this](int slot){revealSlot(slot);};
    pathStrip.onPage=[this](int direction){pageRack(direction);};
    for(auto& slot:strip.slots){slot->onSelected=[this](int i){revealSlot(i);};slot->onNotice=[this](const juce::String& message){say(message);};}
    stripDrawer.toFront(false);
    for (int s = 0; s < kNumSlots; ++s) { proc.apvts.addParameterListener(SkellierProcessor::typeId(s), this); proc.apvts.addParameterListener(SkellierProcessor::extraId(s), this); proc.apvts.addParameterListener(SkellierProcessor::onId(s), this); }
    proc.apvts.addParameterListener("bypass",this);
    setEditMode(false);startTimerHz(30);
}

RackUI::~RackUI() {
    stopTimer();proc.apvts.removeParameterListener("bypass",this);
    cancelPendingUpdate();
    for (int s = 0; s < kNumSlots; ++s) { proc.apvts.removeParameterListener(SkellierProcessor::typeId(s), this); proc.apvts.removeParameterListener(SkellierProcessor::extraId(s), this); proc.apvts.removeParameterListener(SkellierProcessor::onId(s), this); }
}

void RackUI::say(const juce::String& t) { lcd.setText(t); }

void RackUI::rebuildStripButtons() {
    stripButtons.clear();
    const auto names = proc.listStrips();
    for (int i = 0; i < juce::jmin(6, names.size()); ++i) {
        auto* b = new StripButton();
        const auto name = names[i];
        b->setButtonText(name);
        b->onClick = [this, name] { if (proc.loadStrip(name)) say("Dream strip: " + name); };
        b->onDelete = [this, name] {
            proc.deleteStrip(name);
            juce::Component::SafePointer<RackUI> safe(this);
            juce::MessageManager::callAsync([safe] { if (safe != nullptr) { safe->rebuildStripButtons(); safe->resized(); safe->say("Deleted strip"); } });
        };
        stripButtons.add(b);
        stripDrawer.addAndMakeVisible(b);
    }
    resized();
}

void RackUI::handleAsyncUpdate() {
    for (int s=0;s<kNumSlots;++s) {
        auto& slot=*strip.slots[(size_t)s];
        if(slot.currentType()!=proc.slotType(s))slot.rebuild();
        else slot.syncPowerVisual();
    }
    pathStrip.repaint();
}

void RackUI::timerCallback() {
    const double artworkNow=juce::Time::getMillisecondCounterHiRes();
    const float artworkDt=lastArtworkTick>0.0?float((artworkNow-lastArtworkTick)*.001):0.f;
    lastArtworkTick=artworkNow;
    const int solo=proc.soloSlot.load();
    if(solo!=shownSolo){shownSolo=solo;pathStrip.repaint();}
    const bool bypass=proc.apvts.getRawParameterValue("bypass")->load()>.5f;
    auto toDb = [](float g) { return g > 1.0e-5f ? 20.0f * std::log10(g) : -90.0f; };
    inShown = juce::jmax(toDb(proc.takeInputPeak()), inShown - 2.0f);
    outShown = juce::jmax(toDb(proc.takeOutputPeak()), outShown - 2.0f);
    meters.setLevels(inShown, outShown);
    for (int s = 0; s < kNumSlots; ++s) {
        strip.slots[(size_t) s]->setMeter(proc.gainReduction(s));
        if(hasOutputPeak(proc.slotType(s))){
            const float peak=proc.takeJetPeak(s);
            strip.slots[(size_t)s]->setOutputPeak(!bypass&&proc.slotOn(s)&&(solo<0||solo==s)?peak:0.f);
        }
        strip.slots[(size_t) s]->syncSolo();
        strip.slots[(size_t) s]->syncArtwork();
        auto& slot=*strip.slots[(size_t)s];
        // Ambient light follows the existing rack input peak, not a fabricated per-effect measurement.
        const float light=juce::jlimit(0.f,1.f,(inShown+48.f)/48.f);
        slot.advanceArtworkMotion(artworkDt,isShowing()&&slot.isShowing()&&viewport.getViewArea().intersects(slot.getBounds()),!bypass&&proc.slotOn(s)&&(solo<0||solo==s),light);
    }
    undoBtn.setEnabled(proc.canUndo());
    redoBtn.setEnabled(proc.canRedo());
    refreshRackNavigation();
}

void RackUI::setEditMode(bool enabled) {
    editing=enabled;editToggle.setToggleState(enabled,juce::dontSendNotification);
    for(auto* b:{&snapA,&snapB,&snapCopy,&undoBtn,&redoBtn,&clearBtn,&stripsToggle})b->setVisible(enabled);
    for(auto& b:scaleBtn)b.setVisible(enabled);
    if(!enabled){stripsToggle.setToggleState(false,juce::dontSendNotification);stripDrawer.setVisible(false);}
    for(auto& slot:strip.slots)if(slot)slot->setEditMode(enabled);
    resized();repaint();
}
void RackUI::revealSlot(int slot){
    if(slot<0||slot>=kNumSlots)return;
    pathStrip.setFocusedSlot(slot);
    const int first=rackgeo::firstFromOffset(viewport.getViewPositionX(),libraryVisible);
    viewport.setViewPosition(rackgeo::offset(rackgeo::firstForSlot(first,slot,libraryVisible),libraryVisible),0);
    refreshRackNavigation();
}
bool RackUI::slotIsFullyVisible(int slot) const{
    return slot>=0&&slot<kNumSlots&&viewport.getViewArea().contains(strip.slots[static_cast<size_t>(slot)]->getBounds());
}
void RackUI::refreshRackNavigation(){
    const auto area=viewport.getViewArea();
    int first=0,last=kNumSlots-1;
    for(int i=0;i<kNumSlots;++i)if(strip.slots[static_cast<size_t>(i)]->getX()>=area.getX()){first=i;break;}
    for(int i=kNumSlots-1;i>=0;--i)if(strip.slots[static_cast<size_t>(i)]->getRight()<=area.getRight()){last=i;break;}
    pathStrip.setVisibleRange(first,last);
}
void RackUI::pageRack(int direction){
    const int first=rackgeo::firstFromOffset(viewport.getViewPositionX(),libraryVisible);
    viewport.setViewPosition(rackgeo::offset(first+direction*rackgeo::count(libraryVisible),libraryVisible),0);
    refreshRackNavigation();
}
void RackUI::setLibraryVisible(bool visible) {
    const int first=rackgeo::firstFromOffset(viewport.getViewPositionX(),libraryVisible);
    libraryVisible=visible;
    library.setVisible(visible);
    libraryToggle.setButtonText(visible?"HIDE LIBRARY":"SHOW LIBRARY");
    libraryToggle.setTooltip(visible?"Hide the module library to see five full rack slots":
                                     "Show the module library to browse, search and add effects");
    resized();
    viewport.setViewPosition(rackgeo::offset(first,libraryVisible),0);
    refreshRackNavigation();repaint();
}
bool RackUI::fittedRackIsValid() const {
    const auto area=viewport.getViewArea();
    const auto w=rackgeo::width(libraryVisible);
    if(viewport.getRight()!=rackgeo::end(libraryVisible)||area.getWidth()!=rackgeo::viewWidth(libraryVisible))return false;
    for(int i=0;i<kNumSlots;++i){
        const auto b=strip.slots[static_cast<size_t>(i)]->getBounds();
        if(b.getX()!=rackgeo::pad+i*(w+rackgeo::gap)||b.getWidth()!=w)return false;
    }
    return (1252-(viewport.getRight()-rackgeo::pad))==14;
}
bool RackUI::libraryLayoutIsValid() const {
    const int left=libraryVisible?visibleRackX:hiddenRackX;
    const int required=rackgeo::viewWidth(libraryVisible);
    return library.isVisible()==libraryVisible && libraryToggle.isVisible()
        && libraryToggle.getButtonText()==(libraryVisible?"HIDE LIBRARY":"SHOW LIBRARY")
        && viewport.getBounds()==Rectangle<int>(left,rackY,required,rackH)
        && library.getBounds()==Rectangle<int>(libraryX,libraryY,libraryW,libraryH)
        && (!libraryVisible||library.getRight()<viewport.getX())
        && viewport.getWidth()==required && fittedRackIsValid()
        && viewport.getHeight()>=SlotComponent::kHeight+4+viewport.getScrollBarThickness()
        && !prevPreset.getBounds().intersects(presetBox.getBounds())
        && !presetBox.getBounds().intersects(nextPreset.getBounds())
        && !nextPreset.getBounds().intersects(inKnob.getBounds())
        && !inKnob.getBounds().intersects(meters.getBounds())
        && !meters.getBounds().intersects(outKnob.getBounds())
        && (!editing||editToggle.getRight()<=scaleBtn[0].getX())
        && bypassBtn.getX()-libraryToggle.getRight()>=6
        && bypassBtn.getX()-libraryToggle.getRight()<=12
        && editToggle.getX()-bypassBtn.getRight()>=6
        && editToggle.getX()-bypassBtn.getRight()<=12
        && !libraryToggle.getBounds().intersects(editToggle.getBounds())
        && !libraryToggle.getBounds().intersects(bypassBtn.getBounds());
}

void RackUI::resized() {
    prevPreset.setBounds(446,15,39,36);presetBox.setBounds(487,15,195,36);nextPreset.setBounds(686,15,39,36);
    inKnob.setBounds(780,4,115,59);meters.setBounds(898,13,187,39);outKnob.setBounds(1093,4,148,59);
    lcd.setBounds(editing?24:30,67,editing?300:750,28);
    snapA.setBounds(335,66,30,27);snapB.setBounds(368,66,30,27);snapCopy.setBounds(402,66,49,27);
    undoBtn.setBounds(466,66,51,27);redoBtn.setBounds(520,66,51,27);clearBtn.setBounds(574,66,51,27);
    stripsToggle.setBounds(642,66,69,27);bypassBtn.setBounds(editing?827:1062,66,76,27);
    editToggle.setBounds(editing?911:1150,66,editing?81:87,27);
    for(int k=0;k<5;++k)scaleBtn[k].setBounds(1005+k*47,66,43,27);
    stripDrawer.setBounds(8,105,1244,49);
    stripsLabel.setBounds(10,9,104,30);defaultStrip.setBounds(116,9,82,30);
    for(int k=0;k<stripButtons.size();++k)stripButtons[k]->setBounds(203+k*110,9,105,30);
    stripName.setBounds(892,9,192,30);saveStripBtn.setBounds(1090,9,132,30);
    libraryToggle.setBounds(editing?719:950,66,100,27);
    library.setBounds(libraryX,libraryY,libraryW,libraryH);
    const int rackLeft=libraryVisible?visibleRackX:hiddenRackX;
    strip.setLibraryMode(libraryVisible);
    viewport.setBounds(rackLeft,rackY,rackgeo::viewWidth(libraryVisible),rackH);
    pathStrip.setBounds(25,704,1210,62);
    refreshRackNavigation();
}

void RackUI::paint(Graphics& g) {
    if(getWidth()<=0||getHeight()<=0)return;
    // Static shell: meter and knob repaints reuse grain, rails and footer.
    const int rw=getWidth()*3/2,rh=getHeight()*3/2;
    if(!chassisCache.isValid()||chassisCache.getWidth()!=rw||chassisCache.getHeight()!=rh||cachedLibraryVisible!=libraryVisible){
        chassisCache=juce::Image(juce::Image::RGB,rw,rh,true);cachedLibraryVisible=libraryVisible;
        Graphics cached(chassisCache);cached.addTransform(juce::AffineTransform::scale(1.5f));paintChassis(cached);
    }
    g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);g.setOpacity(1.f);g.drawImage(chassisCache,getLocalBounds().toFloat());
}
void RackUI::paintChassis(Graphics& g) {
    g.fillAll(Colour(0xff11100f));
    paintHardwarePlate(g,getLocalBounds().toFloat().reduced(1.f),Colour(0xff1c1b19),true,8.f,4.f);
    const auto header=Rectangle<float>(8,4,1244,95);
    // Keep the exposed chassis dark like the original reference; do not
    // darken the more colorful module panels or attenuate the real LEDs.
    g.setColour(Colour(0xff0c0c0b).withAlpha(.52f));g.fillRoundedRectangle(header,5.f);
    {juce::Graphics::ScopedSaveState save(g);juce::Path clip;clip.addRoundedRectangle(header.getX(),header.getY(),header.getWidth(),header.getHeight(),5.f);g.reduceClipRegion(clip);paintBrushed(g,header,.24f);}
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffb5a38a).withAlpha(.04f),header.getY(),juce::Colours::transparentBlack,header.getBottom()));g.fillRoundedRectangle(header,5.f);
    // Gentle upper-left reflection across the machined header; not a second border.
    g.setGradientFill(juce::ColourGradient(Colour(0xffd6bf9f).withAlpha(.045f),header.getX()+120.f,header.getY(),juce::Colours::transparentWhite,header.getRight(),header.getBottom(),true));g.fillRoundedRectangle(header,5.f);
    g.setColour(Colour(0xff91816b).withAlpha(.44f));g.drawHorizontalLine(61,20,1240);
    g.setColour(juce::Colours::black.withAlpha(.75f));g.drawHorizontalLine(62,20,1240);g.setColour(Colour(0xff777064).withAlpha(.5f));
    g.drawHorizontalLine(100,12,1248);
    paintMonogram(g,Rectangle<float>(21,12,27,38),pal::ink);
    g.setColour(Colour(0xfff4e5c8));g.setFont(font(27.f,true).withExtraKerningFactor(.075f));
    g.setColour(juce::Colours::black.withAlpha(.75f));g.drawFittedText("SKELLIERE RACK",Rectangle<int>(65,11,349,43),juce::Justification::centredLeft,1);
    g.setColour(Colour(0xfff4e5c8));g.drawFittedText("SKELLIERE RACK",Rectangle<int>(65,10,349,43),juce::Justification::centredLeft,1);
    g.setColour(Colour(0xffc8b99c).withAlpha(.52f));
    g.drawVerticalLine(54,14,49);g.drawVerticalLine(427,14,49);g.drawVerticalLine(754,14,49);
    const float railLeft=libraryVisible?269.f:8.f;
    const auto frame=Rectangle<float>(railLeft,103,1252.f-railLeft,595);
    g.setColour(juce::Colours::black.withAlpha(.9f));g.fillRoundedRectangle(frame.expanded(2.f).translated(0.f,2.f),11);
    paintHardwarePlate(g,frame,Colour(0xff262827),true,8.f,4.5f);
    g.setColour(Colour(0xff070605));g.fillRoundedRectangle(frame.reduced(3.5f),6);
    // A machined upper and lower lip, not a flat gold outline.
    auto rail=frame.reduced(5.f).withHeight(6.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff756f63),rail.getY(),Colour(0xff080807),rail.getBottom()));g.fillRoundedRectangle(rail,2.f);
    rail=rail.withY(frame.getBottom()-6.f);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff090908),rail.getY(),Colour(0xff746a59),rail.getBottom()));g.fillRoundedRectangle(rail,2.f);
    g.setColour(Colour(0xff0e0c0a));g.drawRoundedRectangle(frame.reduced(4),7,1.7f);
    paintScrew(g,1245,117,4.5f,.6f);paintScrew(g,1245,685,4.5f,1.5f);
    if(!libraryVisible){paintScrew(g,14,117,4.2f,.9f);paintScrew(g,14,685,4.2f,1.3f);}
    const auto foot=Rectangle<float>(8,700,1244,72);
    g.setColour(juce::Colours::black.withAlpha(.72f));g.fillRoundedRectangle(foot.translated(0,2),10);
    paintHardwarePlate(g,foot,Colour(0xff10100e),true,12.f,4.5f);
    g.setColour(Colour(0xff100f0e).withAlpha(.22f));g.fillRoundedRectangle(foot.reduced(5.f),7.f);
    paintScrew(g,25,736,6.f,.8f);paintScrew(g,1235,736,6.f,1.4f);
}

// =================================================================== editor window
SkellierEditor::SkellierEditor(SkellierProcessor& p) : juce::AudioProcessorEditor(&p) {
    setLookAndFeel(&look);
    ui = std::make_unique<skl::ui::RackUI>(p);
    addAndMakeVisible(*ui);
    ui->onScale = [this](float z) { applyScale(z); };
    applyScale(1.0f);
}

SkellierEditor::~SkellierEditor() {
    ui.reset();
    setLookAndFeel(nullptr);
}

void SkellierEditor::applyScale(float z) {
    ui->setTransform(juce::AffineTransform::scale(z));
    ui->setBounds(0, 0, skl::ui::RackUI::kW, skl::ui::RackUI::kH);
    setSize(juce::roundToInt((float) skl::ui::RackUI::kW * z), juce::roundToInt((float) skl::ui::RackUI::kH * z));
}

