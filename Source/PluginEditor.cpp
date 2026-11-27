#include "PluginEditor.h"
#include "MeterCalibration.h"
#include "AdvancedPanel.h"
#include "IdentityPanels.h"

using namespace skl;
using namespace skl::ui;
using juce::Graphics;
using juce::Rectangle;

static juce::String slotLetter(int i) { return juce::String::charToString((juce::juce_wchar) ('A' + i)); }
static int textWidth(const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidthInt(f, s); }

// A single measured aggregate output peak, not invented stereo channels.
static void paintMixHeadPeak(Graphics& g,Rectangle<float> r,float db,bool plate){
    const auto track=r.reduced(7.f,0.f).withTrimmedTop(31.f).withTrimmedBottom(31.f);
    constexpr int n=32;const float step=track.getHeight()/n;
    if(plate){
        paintHardwarePlate(g,r,Colour(0xff312521),false,3.f,2.5f);
        paintRecessedWindow(g,r.reduced(4.f),2.f);
        g.setColour(Colour(0xffe1cfb0));g.setFont(font(9.f,true));
        g.drawText("OUT",r.withTrimmedTop(8).withHeight(14),juce::Justification::centred);
        g.setFont(font(7.2f));
        for(int i=0;i<=5;++i){float y=track.getY()+i*track.getHeight()/5.f;g.drawText(juce::String(-i*12),Rectangle<float>(track.getRight()-13,y-5,15,10),juce::Justification::centred);}
        return;
    }
    const int lit=juce::jlimit(0,n,int(std::ceil((db+60.f)*n/60.f)));
    for(int i=0;i<n;++i){
        auto b=Rectangle<float>(track.getX(),track.getBottom()-(i+1)*step,track.getWidth()-15.f,step-1.2f);
        auto c=i>=29?Colour(0xffff4b25):i>=23?Colour(0xffffc45c):Colour(0xffd9b65a);
        if(i>=lit)c=Colour(0xff35291d);
        g.setGradientFill(juce::ColourGradient::vertical(c.brighter(.28f),b.getY(),c.darker(.46f),b.getBottom()));g.fillRoundedRectangle(b,.6f);
    }
    g.setColour(Colour(0xffe6d1a4));g.setFont(font(9.f,true));g.drawText(db<=-60.f?juce::String("-INF"):juce::String(db,1),r.withY(r.getBottom()-27).withHeight(13),juce::Justification::centred);
    g.setFont(font(7.5f));g.drawText("dBFS",r.withY(r.getBottom()-14).withHeight(10),juce::Justification::centred);
}

static void paintMixHeadBadge(Graphics& g,Rectangle<float> r){
    paintHardwarePlate(g,r,Colour(0xff372a22),false,4.f,3.f);
    const auto glass=r.reduced(5.f);paintRecessedWindow(g,glass,2.f);
    auto lcd=glass.reduced(4.f);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff5b6025),lcd.getY(),Colour(0xff202b0f),lcd.getBottom()));g.fillRoundedRectangle(lcd,1.2f);
    g.setColour(Colour(0xffbbc96a).withAlpha(.20f));for(int y=int(lcd.getY()+1);y<int(lcd.getBottom());y+=3)g.drawHorizontalLine(y,lcd.getX()+2,lcd.getRight()-2);
    g.setColour(Colour(0xffa9b976));g.setFont(font(8.f,true).withExtraKerningFactor(.07f));g.drawFittedText("4X TAPE  /  12 SAMPLES",lcd.withY(lcd.getBottom()-12.f).withHeight(11.f).toNearestInt(),juce::Justification::centred,1);
    g.setColour(Colour(0xffe8ecd0).withAlpha(.13f));g.fillRect(lcd.reduced(1.f).withHeight(2.f));
}

static Rectangle<int> mixHeadBadgeBounds(int width){return {12,tallPanelY(100),width-24,tallPanelY(162)-tallPanelY(100)};}
static void paintMixHeadReadout(Graphics& g,Rectangle<float> r,float input,float drive,float output){
    auto lcd=r.reduced(9.f);const float cw=lcd.getWidth()/3.f;
    const juce::String labels[]={"IN dB","DRIVE %","OUT dB"};
    const juce::String vals[]={juce::String(input,1),juce::String(drive*100.f,0),juce::String(output,1)};
    for(int i=0;i<3;++i){auto col=lcd.withX(lcd.getX()+i*cw).withWidth(cw);g.setColour(Colour(0xffa9b976));g.setFont(font(8.f,true));g.drawFittedText(labels[i],col.withHeight(12.f).toNearestInt(),juce::Justification::centred,1);
        g.setColour(Colour(0xffe7f4a2));g.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(),17.f,juce::Font::bold)));g.drawFittedText(vals[i],col.withY(col.getY()+13.f).withHeight(22.f).toNearestInt(),juce::Justification::centred,1);
        if(i<2){g.setColour(Colour(0xffa9b976).withAlpha(.22f));g.drawVerticalLine(int(col.getRight()),col.getY()+3.f,col.getY()+34.f);}
    }
}

static void paintHushMeter(Graphics& g,Rectangle<float> r,float db,bool plate){
    auto track=r.reduced(12,0).withTrimmedTop(27).withTrimmedBottom(22);track.setWidth(16.f);
    if(plate){paintHardwarePlate(g,r,Colour(0xff29343a),false,5.f,3.f);paintRecessedWindow(g,track,2);g.setFont(font(9.5f,true));g.setColour(Colour(0xffcbd5cf));g.drawText("GR dB",r.withHeight(24),juce::Justification::centred);for(int i=0;i<=4;++i){float y=track.getY()+track.getHeight()*i*.25f;g.setColour(Colour(0xff80908d));g.drawHorizontalLine(int(y),track.getRight()+3,track.getRight()+8);g.drawText(juce::String(i*6),Rectangle<float>(track.getRight()+10,y-7,22,14),juce::Justification::centredLeft);}}
    else {const float v=juce::jlimit(0.f,1.f,std::abs(db)/24.f);auto bar=track.reduced(2).withHeight(track.reduced(2).getHeight()*v);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xffdcffb5),bar.getY(),Colour(0xff2a986b),bar.getBottom()));g.fillRoundedRectangle(bar,1);}
}
// Two independent real meter scales: gain reduction in dB and output peak in dBFS.
static void paintSSLReduction(Graphics& g,Rectangle<float> r,float db,float peak,bool plate){
    // Slim console-style LED banks, engraved directly into the panel. Both
    // lanes use measured telemetry: gain reduction and aggregate output peak.
    const float left=r.getX()+5.f,width=r.getWidth()-10.f;const int n=16;const float step=width/n;
    auto line=[&](float y,const char* label,float value,int lit,bool output){
        auto track=Rectangle<float>(left-1,y+12.f,width+2,7.f);
        if(plate){g.setColour(Colour(0xff090d0c));g.fillRoundedRectangle(track,1.2f);g.setColour(Colour(0xff565c54));g.drawRoundedRectangle(track.expanded(.65f),1.2f,.6f);return;}
        g.setColour(Colour(0xffd5d8cd));g.setFont(font(8.1f,true));
        g.drawText(label,left,y,width*.63f,11,juce::Justification::centredLeft);
        g.drawText((output&&value<=-60.f)?juce::String("-INF"):juce::String(value,1),left+width*.55f,y,width*.45f,11,juce::Justification::centredRight);
        for(int i=0;i<n;++i){const float d=juce::jmin(4.1f,step-1.f);const auto v=Rectangle<float>(left+i*step+(step-d)*.5f,y+13.3f,d,d);const bool on=i<lit;const auto c=output?(i>=14?Colour(0xffef6150):i>=11?Colour(0xffd8ae44):Colour(0xff73cb67)):Colour(0xffe6ad51);
            g.setGradientFill(juce::ColourGradient::vertical(on?c.brighter(.32f):Colour(0xff30342a),v.getY(),on?c.darker(.42f):Colour(0xff161915),v.getBottom()));g.fillEllipse(v);}
    };
    line(r.getY()+2,"GR dB",std::abs(db)<.05f?0.f:-std::abs(db),juce::jlimit(0,n,int(std::ceil(std::abs(db)*n/20.f))),false);
    line(r.getY()+27,"OUT dBFS",peak,juce::jlimit(0,n,int(std::ceil((peak+60.f)*n/60.f))),true);
    if(plate){g.setColour(Colour(0xff83897f).withAlpha(.67f));g.setFont(font(7.f));const char* ticks[]={"-60","-40","-20","0"};for(int i=0;i<4;++i){float x=left+i*width/3.f;auto label=Rectangle<float>(x-12,r.getY()+48.f,24,10);if(i==0)label.setX(left-1);if(i==3)label.setRight(left+width);g.drawFittedText(ticks[i],label.toNearestInt(),i==0?juce::Justification::centredLeft:i==3?juce::Justification::centredRight:juce::Justification::centred,1);}}
}
// Real post-effect aggregate peak, red LED ladder like the rack hardware.
static void paintDopePeak(Graphics& g,Rectangle<float> r,float db,bool plate){
    const auto track=r.reduced(10.f,0).withTrimmedTop(37.f).withTrimmedBottom(36.f);
    const int n=32;const float step=track.getHeight()/n;
    if(plate){paintHardwarePlate(g,r,Colour(0xff35322d),false,3.f,2.f);paintRecessedWindow(g,r.reduced(4.f),2.f);g.setColour(Colour(0xffddd7ce));g.setFont(font(10.f,true));g.drawText("OUTPUT",r.withY(r.getY()+9).withHeight(16),juce::Justification::centred);return;}
    const int lit=juce::jlimit(0,n,int(std::ceil((db+60.f)*n/60.f)));
    for(int i=0;i<n;++i){auto b=Rectangle<float>(track.getX(),track.getBottom()-(i+1)*step,track.getWidth()*.68f,step-1.2f);const auto c=i<lit?Colour(0xffff3e22):Colour(0xff391714);g.setGradientFill(juce::ColourGradient::vertical(c.brighter(.25f),b.getY(),c.darker(.55f),b.getBottom()));g.fillRoundedRectangle(b,.7f);}
    g.setColour(Colour(0xffbcb6a7));g.setFont(font(7.4f));for(int i=0;i<=3;++i){const float yy=track.getY()+i*track.getHeight()/3.f;g.drawText(juce::String(-i*20),Rectangle<float>(track.getX()+track.getWidth()*.70f,yy-5,track.getWidth()*.32f+3,11),juce::Justification::centredLeft);}
    g.setColour(Colour(0xfff1b8a0));g.setFont(font(9.f,true));g.drawText(db<=-60.f?"-INF":juce::String(db,1),r.withY(r.getBottom()-32).withHeight(14),juce::Justification::centred);
    g.setFont(font(8.f));g.drawText("dBFS",r.withY(r.getBottom()-18).withHeight(12),juce::Justification::centred);
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
    if(bool(slider.getProperties().getWithDefault("sslAux",false))&&!compact_){
        const int size=juce::jmax(1,juce::jmin(getWidth()/2,getHeight()-3,43));
        slider.setBounds(1,(getHeight()-size)/2,size,size);return;
    }
    if(bool(slider.getProperties().getWithDefault("sslSwitch",false))&&!compact_){
        const int size=juce::jmax(1,juce::jmin(getWidth()-4,getHeight()-11,43));
        slider.setBounds((getWidth()-size)/2,0,size,size);return;
    }
    if(bool(slider.getProperties().getWithDefault("sslOutput",false))&&!compact_){
        const int size=juce::jmax(1,juce::jmin(getWidth()-4,getHeight()-12,45));
        slider.setBounds((getWidth()-size)/2,0,size,size);return;
    }
    if(bool(slider.getProperties().getWithDefault("consoleCell",false))&&!compact_){
        const int size=juce::jmax(1,juce::jmin(getWidth()-2,getHeight()-20,bool(slider.getProperties().getWithDefault("dopeRow",false))?72:bool(slider.getProperties().getWithDefault("sslGain",false))?49:43));
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
        const int limit=bool(slider.getProperties().getWithDefault("tapeSelector",false))?70:bool(slider.getProperties().getWithDefault("dopeSwitch",false))?76:getWidth()<90?58:getWidth()<125?76:96;
        const int size=juce::jmax(1,juce::jmin(getWidth()-4,getHeight()-32,limit));
        slider.setBounds((getWidth()-size)/2,juce::jmax(0,(getHeight()-size-32)/2),size,size);
    }
}
bool KnobCell::controlGeometryValid() const noexcept {
    const auto b=slider.getBounds();
    if(b.isEmpty()||!getLocalBounds().contains(b))return false;
    if(bool(slider.getProperties().getWithDefault("sslAux",false)))return b.getWidth()==b.getHeight()&&b.getRight()+15<getWidth();
    if(bool(slider.getProperties().getWithDefault("sslSwitch",false)))return b.getWidth()==b.getHeight()&&b.getBottom()+10<=getHeight();
    if(bool(slider.getProperties().getWithDefault("sslOutput",false)))return b.getWidth()==b.getHeight()&&b.getWidth()<=45&&b.getBottom()+11<=getHeight();
    if(bool(slider.getProperties().getWithDefault("consoleCell",false)))return b.getWidth()==b.getHeight()&&b.getWidth()<=(bool(slider.getProperties().getWithDefault("dopeRow",false))?72:bool(slider.getProperties().getWithDefault("sslGain",false))?49:43)&&b.getBottom()+18<=getHeight();
    if(bool(slider.getProperties().getWithDefault("hushFader",false))&&!compact_)return b.getBottom()+28<=getHeight();
    if(b.getWidth()!=b.getHeight())return false;
    if(compact_)return b.getWidth()<=48;
    if(sideReadout_)return b.getWidth()<=88&&b.getRight()+12<getWidth();
    const int limit=bool(slider.getProperties().getWithDefault("tapeSelector",false))?70:bool(slider.getProperties().getWithDefault("dopeSwitch",false))?76:getWidth()<90?58:getWidth()<125?76:96;
    return b.getWidth()<=limit&&b.getBottom()+28<=getHeight();
}

void KnobCell::paint(Graphics& g) {
    // The slider clips its own paint to its square. Render only the matching
    // outside portion of the cached knob shadow here, behind the child.
    // This keeps the original cap size while allowing a real cast shadow.
    if(!compact_&&!slider.isSwitch()&&!bool(slider.getProperties().getWithDefault("sslConsoleCap",false))&&!bool(slider.getProperties().getWithDefault("hushFader",false))&&!slider.getBounds().isEmpty()){
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
    if(bool(slider.getProperties().getWithDefault("sslAux",false))){
        auto text=getLocalBounds().withTrimmedLeft(slider.getRight()+2);const int cy=getHeight()/2;
        g.setColour(Colour(0xffe8e5d9));g.setFont(font(9.6f,true));g.drawFittedText(label_,text.withY(cy-10).withHeight(10),juce::Justification::centred,1);
        g.setColour(Colour(0xffb4c2b9));g.setFont(font(9.4f));g.drawFittedText(text_,text.withY(cy).withHeight(11),juce::Justification::centred,1);return;
    }
    if(bool(slider.getProperties().getWithDefault("sslSwitch",false))){
        g.setColour(Colour(0xffece9dc));g.setFont(font(9.f,true));
        g.drawFittedText(label_,Rectangle<int>(0,getHeight()-11,getWidth(),11),juce::Justification::centred,1);return;
    }
    if(bool(slider.getProperties().getWithDefault("sslOutput",false))){
        g.setColour(Colour(0xffe3e1d9));g.setFont(font(9.5f,true));
        g.drawFittedText(label_+"  "+text_,Rectangle<int>(0,getHeight()-11,getWidth(),11),juce::Justification::centred,1);return;
    }
    if(bool(slider.getProperties().getWithDefault("consoleCell",false))){
        const int yy=juce::jmin(getHeight()-19,slider.getBottom()+1);
        const float fs=bool(slider.getProperties().getWithDefault("dopeRow",false))?12.f:getWidth()<40?8.9f:10.2f;
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
    const float lr=R*.74f;
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff33342f),cy-lr,Colour(0xff10130f),cy+lr));g.fillEllipse(cx-lr,cy-lr,2*lr,2*lr);
    const auto ink=getToggleState()?Colour(0xffb8edac):Colour(0xff777b70);
    if(getToggleState()){g.setColour(ink.withAlpha(.17f));g.fillEllipse(cx-lr,cy-lr,2*lr,2*lr);}
    juce::Path symbol;symbol.addCentredArc(cx,cy+.3f,lr*.62f,lr*.62f,0,.67f,juce::MathConstants<float>::twoPi-.67f,true);
    g.setColour(ink);g.strokePath(symbol,juce::PathStrokeType(1.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));g.drawLine(cx,cy-lr*.8f,cx,cy,1.1f);
    if(hover||down){g.setColour(juce::Colours::white.withAlpha(down?.15f:.07f));g.fillEllipse(cx-R,cy-R,2*R,2*R);}

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
    addChildComponent(modulePresetButton);modulePresetButton.setTooltip("Apply a starting-point preset to this effect only");modulePresetButton.getProperties().set("plain",true);modulePresetButton.getProperties().set("quiet",true);
    modulePresetButton.onClick=[this]{const int originalType=type;const auto names=proc.modulePresetNames(type);if(names.isEmpty())return;juce::PopupMenu menu;for(int j=0;j<names.size();++j)menu.addItem(j+1,names[j]);juce::Component::SafePointer<SlotComponent> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&modulePresetButton).withMinimumWidth(140),[safe,originalType](int choice){if(safe!=nullptr&&choice>0&&safe->type==originalType&&safe->proc.applyModulePreset(safe->index,originalType,choice-1))safe->repaint();});};
    addAndMakeVisible(artButton);addChildComponent(advancedButton);advancedButton.setTooltip("Open advanced DSP engine and controls");advancedButton.getProperties().set("plain",true);
    advancedButton.onClick=[this]{if(!hasAdvancedEngine(type))return;auto* parent=findParentComponentOfClass<juce::AudioProcessorEditor>();if(!parent)return;juce::CallOutBox::launchAsynchronously(std::make_unique<AdvancedPanel>(proc,index,type),parent->getLocalArea(this,advancedButton.getBounds()),parent);};
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
    removeBtn.getProperties().set("moduleClose",true);removeBtn.setTooltip("Remove module (Undo restores it)"); soloBtn.setTooltip("Solo this slot");
    rebuild();
}

void SlotComponent::rebuild() {
    type = proc.slotType(index);
    cells.clear();tuneHover=-1;
    const bool filled = type != Empty;
    artButton.setVisible(filled&&editing);advancedButton.setVisible(hasAdvancedEngine(type));modulePresetButton.setVisible(filled&&!proc.modulePresetNames(type).isEmpty());
    appliedArtworkChoice = -1; panelCache={};
    powerBtn.setVisible(filled);
    for(auto* c:{&leftBtn,&rightBtn,&soloBtn})c->setVisible(filled&&editing);removeBtn.setVisible(filled);
    if (filled) {
        const auto& mi = info(type);
        for (int j = 0; j < uiKnobCount(type)+(type==Tune?1:0); ++j) {
            const bool ext=type==SkellSSL&&j>=kMaxKnobs;
            const int extIndex=ext?j-kMaxKnobs:-1;
            const bool human=type==Tune&&j==5;
            Knob k=human?Knob{"Humanize",0,1,false,0,Unit::Pct,0,nullptr}:ext?sslControl(extIndex).knob:mi.knobs[j];
            if(type==SkellSSL){if(j==0)k.label="E/G";if(extIndex==SslThreshold)k.label="THRESH";if(extIndex==SslRatio)k.label="RATIO";}
            auto* param=proc.apvts.getParameter(human?SkellierProcessor::humanizeId(index):ext?SkellierProcessor::sslId(index,extIndex):SkellierProcessor::knobId(index,j));
            auto cell=std::make_unique<KnobCell>(k.label,*param,[k,extIndex,type=type](double n){
                if(type==EchoUnified&&extIndex<0&&juce::String(k.label)=="Time / Sync")return knobText(k,float(n))+" / "+juce::StringArray{"1/16","1/8","1/8 D","1/4","1/4 D","1/2"}[juce::jlimit(0,5,int(std::lround(n*5)))];
                if(type!=SkellSSL)return knobText(k,float(n));
                const float v=extIndex>=0?sslReal(extIndex,float(n)):toReal(k,float(n));
                if(extIndex==SslHP&&v<16.5f)return juce::String("OFF");
                if(extIndex==SslLP&&n<.001)return juce::String("OFF");
                if(k.unit==Unit::Hz){const float hz=extIndex==SslLP?std::max(3000.f,v):v;return hz>=1000?juce::String(hz/1000,1)+"k":juce::String(int(std::lround(hz)));}
                if(k.unit==Unit::Ms)return juce::String(v/1000,2)+"s";
                if(k.unit==Unit::Db)return juce::String(v>0?"+":"")+juce::String(v,1);
                if(k.unit==Unit::Scalar)return juce::String(v,1);
                return knobText(k,float(n));},capColour(mi.cat),pointerColour(mi.cat),double(human?0.f:ext?sslDefault(extIndex):defaultNorm(type,j)),
                                                   k.steps > 1 ? 1.0 / (double) (k.steps - 1) : 0.0);
            if((type==SkellDope||type==SkellMixHead||type==SkellDemon||type==SkellSSL)&&k.steps==2){
                cell->slider.getProperties().set("colorSwitch",true);
                cell->slider.getProperties().set("dopeSwitch",type==SkellDope);
                if(type==SkellMixHead)cell->slider.getProperties().set("tapeSelector",true);
                cell->slider.getProperties().set("switchLabels",juce::String(k.names));
                cell->slider.setTooltip("Click to switch mode");
                cell->slider.setWantsKeyboardFocus(true);
            }
            if(type==SkellDope&&j>0){cell->slider.getProperties().set("consoleCell",true);cell->slider.getProperties().set("dopeRow",true);}
            if(type==SkellMixHead&&(j==0||j==1||j==3)){
                auto previous=cell->slider.onValueChange;
                cell->slider.onValueChange=[this,previous]{if(previous)previous();repaint(mixHeadBadgeBounds(getWidth()));};
            }
            if(identityMeter(type)||identityArt(type)){auto previous=cell->slider.onValueChange;cell->slider.onValueChange=[this,previous]{if(previous)previous();auto l=originalLayout(type,getWidth(),editing);repaint(l.meter.getUnion(l.detail).expanded(2));};}
            if(type==SkellNoise&&j==1)cell->slider.setTooltip("Adaptive noise threshold: maximum tracked noise level in dB. Raise for louder background noise; lower to protect quiet vocal detail. Amount sets suppression strength.");
            if(type==SkellNoise&&j==2)cell->slider.setTooltip("Recovery time of the attenuation as signal level changes.");
            if(type==SkellNoise&&j==0){cell->slider.getProperties().set("hushFader",true);cell->slider.setSliderStyle(juce::Slider::LinearVertical);}
            if(type==SkellSSL){
                cell->slider.getProperties().set("consoleCell",true);
                if(k.steps==2)cell->slider.getProperties().set("sslSwitch",true);
                if(extIndex==SslHMQ||extIndex==SslLMQ)cell->slider.getProperties().set("sslAux",true);
                if(!ext&&j>=1&&j<=4)cell->slider.getProperties().set("sslGain",true);
                // Restore shared rotating Skelliere skull cap; keep compact console geometry.
                static const unsigned caps[]={0xffa5a79e,0xffac7544,0xff397eac,0xff42865a,0xffbc383a,0xffbb393b};
                unsigned cap=ext?0xffa5a6a0:caps[j];
                if(extIndex==SslInput)cap=0xffb6443c;
                if(extIndex==SslOutput){cap=0xffa9aaa5;cell->slider.getProperties().set("sslOutput",true);}
                if(extIndex==SslHP)cap=0xff51865d;
                if(extIndex==SslLP)cap=0xffa16b3e;
                if(extIndex==SslHFFreq||extIndex==SslHMFreq||extIndex==SslHMQ||extIndex==SslLMFreq||extIndex==SslLMQ||extIndex==SslLFFreq)cap=0xffa9aaa5;
                if(extIndex==SslExpThreshold)cap=0xff41865b;
                if(extIndex==SslThreshold)cap=0xffb63a33;
                cell->slider.getProperties().set("houseTint",juce::String::toHexString(int(cap)));
                if(extIndex==SslThreshold||extIndex==SslRatio)cell->slider.setTooltip("Base setting. COMP macro also lowers threshold and raises ratio (preserves legacy automation).");
                if(extIndex==SslHFBell||extIndex==SslLFBell)cell->slider.setTooltip("Shelf/bell shape in E circuit; G uses its native shelf law.");
                if(extIndex==SslHP||extIndex==SslLP)cell->slider.setTooltip("Filter frequency; turn fully down for OFF.");
            }
            if(k.unit==Unit::Db&&k.lo<0.f&&k.hi>0.f&&std::abs(k.lo+k.hi)<.001f)cell->setBipolarLegend(k.lo,k.hi);
            addAndMakeVisible(*cell);
            if(type==Tune){cell->setVisible(j==0||j==5);if(j==0)cell->slider.setTooltip("0 ms hard tune; higher values slow pitch correction. Existing 0-200 ms automation preserved.");if(j==5)cell->slider.setTooltip("Preserve fast pitch expression and soften correction near the target note");}
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
        if(type==SkellDope)c->slider.getProperties().remove("houseTint");
        // Restore the shared rotating skull cap for all standard effects.
        // SSL retains its established console-specific cap colours.
        if(type!=SkellSSL)c->slider.getProperties().remove("houseTint");
        c->slider.repaint();
    }
    resized();repaint();
}

bool SlotComponent::artworkLayoutIsValid() const {
    if(type==Empty)return cells.empty();
    if(cells.size()!=static_cast<size_t>(uiKnobCount(type)+(type==Tune?1:0)))return false;
    const auto body=getLocalBounds().withTrimmedTop(100).withTrimmedBottom(editing?46:23);
    if(!powerBtn.isVisible()||leftBtn.isVisible()!=editing||rightBtn.isVisible()!=editing||soloBtn.isVisible()!=editing||!removeBtn.isVisible()||artButton.isVisible()!=editing)return false;
    const auto header=getLocalBounds().withHeight(60).reduced(22,0);
    if(modulePresetButton.isVisible()!=!proc.modulePresetNames(type).isEmpty())return false;
    if(modulePresetButton.isVisible()&&(!header.contains(modulePresetButton.getBounds())||modulePresetButton.getBounds().intersects(powerBtn.getBounds())||modulePresetButton.getBounds().intersects(removeBtn.getBounds())||(advancedButton.isVisible()&&modulePresetButton.getBounds().intersects(advancedButton.getBounds()))))return false;
    if(!header.contains(powerBtn.getBounds())||!header.contains(removeBtn.getBounds())||powerBtn.getBounds().intersects(removeBtn.getBounds()))return false;
    if(advancedButton.isVisible()&&(!header.contains(advancedButton.getBounds())||advancedButton.getBounds().intersects(powerBtn.getBounds())||advancedButton.getBounds().intersects(removeBtn.getBounds())))return false;
    for(size_t i=0;i<cells.size();++i){
        const auto& c=*cells[i];
        if(type==Tune&&i>=1&&i<=4){if(c.isVisible())return false;continue;}
        // The central fifth compressor control may extend below the general
        // body, but not into the side fasteners or real power-button bounds.
        const bool makeupBay=!body.contains(c.getBounds())&&!editing&&i==4&&cells.size()==5&&info(type).hasVU&&!vuRect.isEmpty();
        const auto safe=makeupBay?getLocalBounds().reduced(24,0).withTrimmedTop(100).withTrimmedBottom(10):body;
        if(!safe.contains(c.getBounds())||!c.controlGeometryValid()||c.getBounds().intersects(vuRect)||c.getBounds().intersects(footRect)||(type!=Tune&&c.getBounds().intersects(originalLayout(type,getWidth(),editing).detail)))return false;
        if(type==Tune){const auto d=originalLayout(type,getWidth(),editing).detail.toFloat();for(const auto b:{tuneOrbBounds(d),tuneRootBounds(d),tuneScaleBounds(d),tunePianoBounds(d)})if(c.getBounds().intersects(b.toNearestInt()))return false;}
        for(size_t j=0;j<i;++j)if(cells[j]->isVisible()&&c.getBounds().intersects(cells[j]->getBounds()))return false;
    }
    if(type==SkellDope){if(cells[0]->getBounds().getCentreX()!=cells[1]->getBounds().getCentreX()||cells[0]->getBounds().getBottom()>cells[1]->getBounds().getY())return false;for(int j=2;j<=4;++j){if(cells[j]->getBounds().getCentreX()!=cells[1]->getBounds().getCentreX()||cells[j]->getBounds().getCentreY()-cells[j-1]->getBounds().getCentreY()!=cells[2]->getBounds().getCentreY()-cells[1]->getBounds().getCentreY())return false;}}
    return true;
}

bool SlotComponent::exerciseSpriteBindings() {
    if(cells.empty())return true;
    bool ok=true;
    for(size_t i=0;i<cells.size();++i){
        auto* p=proc.apvts.getParameter(type==Tune&&i==5?SkellierProcessor::humanizeId(index):type==SkellSSL&&i>=kMaxKnobs?SkellierProcessor::sslId(index,int(i)-kMaxKnobs):SkellierProcessor::knobId(index,int(i)));
        const float old=p->getValue();
        p->setValueNotifyingHost(0);ok=ok&&std::abs(cells[i]->slider.getValue())<1e-5;
        cells[i]->slider.setValue(1,juce::sendNotificationSync);ok=ok&&std::abs(p->getValue()-1)<1e-5;
        p->setValueNotifyingHost(old);
    }
    if(type==Tune){
        const auto detail=originalLayout(type,getWidth(),editing).detail.toFloat(),keys=tunePianoBounds(detail),modes=tuneScaleBounds(detail);
        auto* key=proc.apvts.getParameter(SkellierProcessor::knobId(index,3));auto* scale=proc.apvts.getParameter(SkellierProcessor::knobId(index,4));const float oldKey=key->getValue(),oldScale=scale->getValue();
        const int white[]={0,2,4,5,7,9,11},black[]={1,3,6,8,10},after[]={0,1,3,4,5};const float kw=keys.getWidth()/7.f;
        for(int j=0;j<7;++j){const juce::Point<float> point(keys.getX()+(j+.5f)*kw,keys.getBottom()-3);const float before=key->getValue();ok=selectTuneKey(point,false)&&ok&&key->getValue()==before;ok=selectTuneKey(point,true)&&ok&&std::abs(key->getValue()-white[j]/11.f)<1e-5f;}
        for(int j=0;j<5;++j){const juce::Point<float> point(keys.getX()+(after[j]+1)*kw,keys.getY()+3);ok=selectTuneKey(point,true)&&ok&&std::abs(key->getValue()-black[j]/11.f)<1e-5f;}
        const auto roots=tuneRootBounds(detail);ok=ok&&tuneKeyAt(detail,roots.getCentre())<0&&tuneKeyAt(detail,modes.getCentre())<0;
        for(int j=0;j<12;++j){cells[3]->slider.setValue(j/11.0,juce::sendNotificationSync);ok=ok&&std::abs(key->getValue()-j/11.f)<1e-5f&&!tuneNoteName(j,false).isEmpty()&&!tuneNoteName(j,true).isEmpty();}
        for(int j=0;j<3;++j){cells[4]->slider.setValue(j*.5,juce::sendNotificationSync);ok=ok&&std::abs(scale->getValue()-j*.5f)<1e-5f;}
        key->setValueNotifyingHost(oldKey);scale->setValueNotifyingHost(oldScale);tuneHover=-1;
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
    if(hasAdvancedEngine(type)){const bool adv=proc.apvts.getRawParameterValue(SkellierProcessor::advancedId(index,advMode))->load()>.5f;const juce::String label=adv?"ADVANCED":"ENGINE";if(advancedButton.getButtonText()!=label)advancedButton.setButtonText(label);}
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
    needle=(type==StereoUnified||type==VoxRider)?gr+(needle-gr)*float(std::exp(-elapsed*15.f)):smoothReduction(needle,gr,elapsed);
    if(type==Multiband&&hasAdvancedEngine(type)&&proc.apvts.getRawParameterValue(SkellierProcessor::advancedId(index,advMode))->load()>.5f)repaint(vuRect.expanded(2));
    if (std::fabs(needle - shownNeedle) > (type==StereoUnified?.01f:.04f)) {
        shownNeedle = needle;
        repaint(vuRect.expanded(2));
    }
}

void SlotComponent::setEditMode(bool enabled) {
    if(editing==enabled)return;
    editing=enabled;panelCache={};
    for(auto* c:{&leftBtn,&rightBtn,&soloBtn,&artButton})c->setVisible(type!=Empty&&editing);removeBtn.setVisible(type!=Empty);
    resized();repaint();
}
void SlotComponent::resized() {
    sectionRects.clear();vuRect={};panelBounds={};
    const int w=getWidth(),h=getHeight();footRect=editing?Rectangle<int>(10,h-43,w-20,32):Rectangle<int>();
    if(type==Empty)return;
    const int top=type==SkellSSL?3:10;
    powerBtn.setBounds(25,top,24,24);removeBtn.setBounds(w-49,top,24,24);
    advancedButton.setBounds(w/2+17,top+4,w-49-(w/2+17)-5,16);
    modulePresetButton.setBounds(54,top+4,w/2-71,16);
    advancedButton.getProperties().set("moduleHeader",true);modulePresetButton.getProperties().set("moduleHeader",true);
    soloBtn.setBounds(25,h-38,27,23);leftBtn.setBounds(59,h-38,21,23);rightBtn.setBounds(84,h-38,21,23);
    artButton.setBounds(w-90,h-38,65,23);
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
    if(type==Empty||type==Trim||type==DualFilter||type==ClipUnified)return {};
    const auto detail=originalLayout(type,getWidth(),editing).detail;
    return type==Tune?tuneOrbBounds(detail.toFloat()).toNearestInt():detail;
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
            pg.setColour(Colour(0xff222625));pg.fillRect(juce::Rectangle<float>(9,25,float(w-18),43));
            pg.setColour(Colour(0xffa8aba4).withAlpha(.32f));pg.drawHorizontalLine(68,9.f,float(w-9));
            paintMonogram(pg,Rectangle<float>(16,33,14,20),Colour(0xffe0ded2));
            pg.setColour(Colour(0xffe0ded2));pg.setFont(font(16.f,true).withExtraKerningFactor(.045f));pg.drawText("SSL STRIP",36,28,w-48,23,juce::Justification::centredLeft);
            pg.setColour(Colour(0xffaab1a9));pg.setFont(font(9.5f,true).withExtraKerningFactor(.14f));pg.drawText("SKELL / E-G CHANNEL",36,49,w-48,13,juce::Justification::centredLeft);
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
        if(nativeChoice==1&&(type==FastComp||type==BusComp||type==Limiter||type==Multiband||type==DynamicEQ||type==Resonance||type==Exciter||type==Doubler||type==Trim||type==DualFilter)){
            // Recessed control bays and mechanical trim, never covering labels/knobs.
            const int top=(type==Limiter?layout.cells[1]:layout.cells[0]).getY()-5, bottom=editing?h-54:h-23;
            if(bottom>top+15){auto bay=Rectangle<float>(10.f,float(top),float(w-20),float(bottom-top));
                pg.setGradientFill(juce::ColourGradient::vertical(Colour(0xff080b0a).withAlpha(.22f),bay.getY(),Colour(0xff080909).withAlpha(.05f),bay.getBottom()));pg.fillRoundedRectangle(bay,3.f);
                pg.setColour(Colour(0xff080909).withAlpha(.6f));pg.drawRoundedRectangle(bay,3.f,.9f);pg.setColour(ink.withAlpha(.24f));pg.drawRoundedRectangle(bay.reduced(1),2.f,.6f);
                for(int j=1;j<mi.numKnobs;++j){const auto cell=layout.cells[j];bool newRow=true;for(int k=0;k<j;++k)if(std::abs(cell.getY()-layout.cells[k].getY())<3)newRow=false;if(newRow){const float yy=cell.getY()-2;pg.setColour(Colour(0xff0a0a09).withAlpha(.48f));pg.drawHorizontalLine(int(yy),bay.getX()+7,bay.getRight()-7);pg.setColour(ink.withAlpha(.19f));pg.drawHorizontalLine(int(yy+1),bay.getX()+7,bay.getRight()-7);}}
            }
        }
        if(!layout.detail.isEmpty()){const int art=legacyArtworkType(type);if(identityArt(type))paintIdentityArt(pg,layout.detail.toFloat(),type,0,0,0,true);else if(type==SkellDemon)paintDemonDetail(pg,layout.detail.toFloat());else if(isAirPanel(type))paintAirDetail(pg,layout.detail.toFloat(),art);else if(art==Reverb||art==Chorus)paintSpaceDetail(pg,layout.detail.toFloat(),art);else if(art==Sat||art==SoftClip||art==Eq||art==Tremolo||art==Phaser||art==Stereo||art==Transient)paintWorkshopDetail(pg,layout.detail.toFloat(),art);else paintTapeDetail(pg,layout.detail.toFloat(),art==Delay&&nativeChoice==1);}
        if(type==SkellSSL){
            const int split=w*46/100,left=10,right=split+6,lw=split-15,rw=w-right-10;
            const auto console=Rectangle<float>(11,75,float(w-22),float(tallPanelY(548)-75));
            pg.setGradientFill(juce::ColourGradient::vertical(Colour(0xff292c2d),console.getY(),Colour(0xff15191a),console.getBottom()));pg.fillRect(console);
            paintPeened(pg,console,.62f);
            paintAnalogWear(pg,console,false,4632,.42f);
            pg.setColour(Colour(0xff121715));pg.drawVerticalLine(split,75,tallPanelY(548));
            pg.setColour(Colour(0xff6c7169).withAlpha(.55f));pg.drawVerticalLine(split+1,75,tallPanelY(548));
            auto section=[&](int x,int y,int ww,const char* label){
                y=tallPanelY(y);pg.setColour(Colour(0xff121615));pg.drawHorizontalLine(y,float(x),float(x+ww));
                pg.setColour(Colour(0xffc5c8bb).withAlpha(.63f));
                if(x==right&&y>tallPanelY(82)&&y<tallPanelY(425)){juce::Path trace;trace.startNewSubPath(float(x),float(y+1));trace.lineTo(float(x+ww*.30f),float(y+1));trace.lineTo(float(x+ww*.39f),float(y+4));trace.lineTo(float(x+ww),float(y+4));pg.strokePath(trace,juce::PathStrokeType(.8f));}
                else pg.drawHorizontalLine(y+1,float(x),float(x+ww));
                pg.setColour(Colour(0xfff2f0e5));pg.setFont(font(9.4f,true).withExtraKerningFactor(.025f));
                pg.drawFittedText(label,Rectangle<int>(x+2,y+2,ww-4,15),juce::Justification::centredLeft,1);
            };
            section(left,82,lw,"INPUT");section(left,158,lw,"FILTERS");section(left,232,lw,"COMPRESSOR");section(left,354,lw,"GATE / EXP");section(left,477,lw,"OUTPUT");
            section(right,82,rw,"EQUALIZER / HF");section(right,168,rw,"HMF");section(right,254,rw,"LMF");section(right,340,rw,"LF");
            section(right,425,rw,"CIRCUIT / E-G");section(right,466,rw,"DYNAMICS");
            paintSSLReduction(pg,layout.meter.toFloat(),0,-60,true);
        }
        else if(type==SkellDope){
            // Matching recessed vertical housings flank the control column.
            auto plaque=layout.meter.toFloat().withX(12.f);
            paintHardwarePlate(pg,plaque,Colour(0xff35322d),false,3.f,2.f);
            paintRecessedWindow(pg,plaque.reduced(4.f),2.f);
            auto brand=plaque.withY(plaque.getY()+10.f).withHeight(28.f).reduced(11.f,2.f);
            pg.setColour(Colour(0xffc5c7bc));pg.fillRoundedRectangle(brand,1.5f);paintMonogram(pg,brand.reduced(6.f,2.f),Colour(0xff20231e));
            {juce::Graphics::ScopedSaveState label(pg);pg.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi,plaque.getCentreX(),plaque.getCentreY()));
             auto words=Rectangle<float>(plaque.getCentreX()-plaque.getHeight()*.34f,plaque.getCentreY()-17,plaque.getHeight()*.68f,34.f);
             pg.setColour(Colour(0xffe6dfc9));pg.setFont(font(23.f,true).withExtraKerningFactor(.16f));pg.drawFittedText("A-TYPE",words.toNearestInt(),juce::Justification::centred,1);}
            pg.setColour(Colour(0xffbcb6a7).withAlpha(.27f));pg.drawRoundedRectangle(plaque.reduced(8.f,43.f),2.f,.65f);
            pg.setColour(Colour(0xffbcb6a7));pg.setFont(font(8.f,true));pg.drawFittedText("SPECTRAL",plaque.withY(plaque.getBottom()-40.f).withHeight(16.f).toNearestInt(),juce::Justification::centred,1);pg.drawFittedText("4",plaque.withY(plaque.getBottom()-24.f).withHeight(14.f).toNearestInt(),juce::Justification::centred,1);
            paintDopePeak(pg,layout.meter.toFloat(),-60,true);
        }
        else if(type==SkellNoise&&!layout.meter.isEmpty())paintHushMeter(pg,layout.meter.toFloat(),0,true);
        else if(type==SkellMixHead&&!layout.meter.isEmpty()){
            paintMixHeadBadge(pg,mixHeadBadgeBounds(w).toFloat());
            paintMixHeadPeak(pg,layout.meter.toFloat(),-60.f,true);
            pg.setColour(ink.withAlpha(.62f));pg.setFont(font(8.4f,true).withExtraKerningFactor(.11f));
            pg.drawFittedText("DIGITAL TAPE SATURATION",Rectangle<int>(12,tallPanelY(518),w-24,18),juce::Justification::centred,1);
        }
        else if((type==StereoUnified||type==VoxRider)&&!layout.meter.isEmpty())identityFrame(pg,layout.meter.toFloat(),identityInk(type));
        else if(identityMeter(type)&&!layout.meter.isEmpty())paintIdentityMeter(pg,layout.meter.toFloat(),type,0,1000,2,0,false,{},true);
        else if(!layout.meter.isEmpty())paintVU(pg,layout.meter.toFloat(),0.f,hasOutputPeak(type),MeterLayer::plate);
        // Subtle engraved separators on five-control hardware, outside all controls.
        if(type!=SkellDope&&mi.numKnobs==5&&!layout.meter.isEmpty()){
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
    if(type==SkellMixHead&&cells.size()>=4&&g.clipRegionIntersects(mixHeadBadgeBounds(w))){
        auto value=[&](int j){return toReal(info(type).knobs[j],float(cells[size_t(j)]->slider.getValue()));};
        paintMixHeadReadout(g,mixHeadBadgeBounds(w).toFloat(),value(0),value(1),value(3));
    }
    const auto detail=originalLayout(type,w,editing).detail;
    if(!detail.isEmpty()&&g.clipRegionIntersects(detail)){
        // Repaint only the optical scene; the heavy chassis/faceplate stays cached.
        juce::Graphics::ScopedSaveState scene(g);g.reduceClipRegion(detail);
        const auto r=detail.toFloat();const auto& m=artworkMotion;const int art=legacyArtworkType(type);
        if(identityArt(type)){std::array<float,6> vals{};for(int j=0;j<std::min(int(cells.size()),6);++j)vals[j]=type==Tune&&j==5?float(cells[j]->slider.getValue()):toReal(info(type).knobs[j],float(cells[j]->slider.getValue()));paintIdentityArt(g,r,type,m.phase,m.level,type==ClipUnified?int(vals[3]):0,false,vals,tuneHover,proc.tuneFlats.load(std::memory_order_relaxed));}else if(type==SkellDemon)paintDemonDetail(g,r,m.phase,m.level,false);else if(isAirPanel(type))paintAirDetail(g,r,art,m.phase,m.level,false);
        else if(art==Reverb||art==Chorus)paintSpaceDetail(g,r,art,m.phase,m.level,false);
        else if(art==Delay||art==Slap)paintTapeDetail(g,r,art==Delay&&nativeChoice==1,m.left,m.right,m.phase);
        else paintWorkshopDetail(g,r,art,m.phase,m.level,false);
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

    }else if((type==StereoUnified||type==VoxRider)&&!vuRect.isEmpty()){
        paintSignedInstrument(g,vuRect.toFloat(),type,needle);
    }else if(identityMeter(type)&&!vuRect.isEmpty()){
        auto v=[&](int j){return j<int(cells.size())?toReal(info(type).knobs[j],float(cells[size_t(j)]->slider.getValue())):0.f;};
        const bool advanced=hasAdvancedEngine(type)&&proc.apvts.getRawParameterValue(SkellierProcessor::advancedId(index,advMode))->load()>.5f;
        std::array<float,8> gr{};if(type==Multiband&&advanced)for(int j=0;j<8;++j)gr[j]=proc.advancedBandReduction(index,j);
        paintIdentityMeter(g,vuRect.toFloat(),type,needle,type==Resonance?v(2):v(0),type==Resonance?v(3):v(1),type==DynamicEQ?v(3):0,advanced,gr,false);
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

bool SlotComponent::selectTuneKey(juce::Point<float> point,bool commit){
    if(type!=Tune)return false;const auto detail=originalLayout(type,getWidth(),editing).detail.toFloat();
    const int note=tuneKeyAt(detail,point);if(note!=tuneHover){tuneHover=note;repaint(detail.toNearestInt());}
    if(note>=0){if(commit&&cells.size()==6)cells[3]->slider.setValue(note/11.0,juce::sendNotificationSync);return true;}

    return false;
}
bool SlotComponent::showTuneMenu(juce::Point<float> point){
    if(type!=Tune||cells.size()!=6)return false;
    const auto detail=originalLayout(type,getWidth(),editing).detail.toFloat();const auto roots=tuneRootBounds(detail),modes=tuneScaleBounds(detail);const bool root=roots.contains(point);if(!root&&!modes.contains(point))return false;
    juce::PopupMenu menu;const int current=int(std::lround(cells[root?3:4]->slider.getValue()*(root?11:2)));
    if(root){const bool flats=proc.tuneFlats.load(std::memory_order_relaxed);for(int n=0;n<12;++n)menu.addItem(n+1,tuneNoteName(n,flats),true,n==current);menu.addSeparator();menu.addItem(30,"Use sharps (#)",true,!flats);menu.addItem(31,"Use flats (b)",true,flats);}
    else{const char* labels[]={"Chromatic","Major","Minor"};for(int n=0;n<3;++n)menu.addItem(n+1,labels[n],true,n==current);}
    juce::Component::SafePointer<SlotComponent> safe(this);const auto target=localAreaToGlobal((root?roots:modes).toNearestInt());
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(target).withMinimumWidth(120),[safe,root](int choice){
        if(safe==nullptr||safe->type!=Tune||safe->proc.slotType(safe->index)!=Tune||safe->cells.size()!=6||choice==0)return;
        if(root&&(choice==30||choice==31)){safe->proc.tuneFlats.store(choice==31,std::memory_order_relaxed);safe->repaint();return;}
        if(choice>=1&&choice<=(root?12:3)){safe->cells[root?3:4]->slider.setValue(double(choice-1)/(root?11:2),juce::sendNotificationSync);safe->repaint();}
    });return true;
}
void SlotComponent::mouseMove(const juce::MouseEvent& e){selectTuneKey(e.position,false);}
void SlotComponent::mouseDown(const juce::MouseEvent& e) {
    dragging=false;
    if(!e.mods.isPopupMenu()&&(showTuneMenu(e.position)||selectTuneKey(e.position,true)))return;
    if(type!=Empty&&!e.mods.isPopupMenu())return;
    // Empty slots can add effects without opening the module library.
    // Popups are asynchronous; do not retain an unsafe editor pointer.
    juce::PopupMenu menu;
    if(type==Empty){
        const int cats[]={CatDynamics,CatTone,CatSpace,CatUtility};
        const char* labels[]={"Dynamics","Tone / EQ","Modulation / Space","Utility"};
        for(int i=0;i<4;++i){
            juce::PopupMenu group;
            for(int t=1;t<kNumRackTypes;++t)if(catalogVisible(t)&&info(t).cat==cats[i])
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
    if(type==Tune&&tuneKeyAt(originalLayout(type,getWidth(),editing).detail.toFloat(),e.getMouseDownPosition().toFloat())>=0){selectTuneKey(e.position,true);return;}
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

rackgeo::PackedRack RackStrip::geometry() const {
    std::array<bool,kNumSlots> narrow{},fixed{};
    for(int i=0;i<kNumSlots;++i)if(slots[i]){narrow[i]=slots[i]->currentType()==SkellSSL;fixed[i]=slots[i]->currentType()==SkellDemon;}
    return rackgeo::pack(libraryMode,narrow,fixed);
}
void RackStrip::resized() {
    const auto geo=geometry();
    for(int i=0;i<kNumSlots;++i)if(slots[i])
        slots[i]->setBounds(geo.x[i],2,geo.w[i],SlotComponent::kHeight);
    if(getWidth()!=geo.total||getHeight()!=SlotComponent::kHeight+4)
        setSize(geo.total,SlotComponent::kHeight+4);
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
        mg.addTransform(juce::AffineTransform::scale(2.f));if(identityMeter(type)||identityArt(type))paintIdentityMini(mg,Rectangle<float>(0,0,42,35),type);else paintOriginalMini(mg,Rectangle<float>(0,0,42,35),type);
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
    std::vector<int> order{NoiseGateUnified,VoxRider,DynamicEQ,Multiband,Resonance,ProgramOpto,DeEsserUnified,AirUnified,Doubler,Exciter};
    for(int t=Eq;t<kNumRackTypes;++t)if(std::find(order.begin(),order.end(),t)==order.end())order.push_back(t);
    for(int t:order) {
        if(!catalogVisible(t))continue;
        auto* tile = new ModuleTile(t, [this](int type) { select(type); }, [this](int type) { proc.addModule(type, -1); });
        tiles.add(tile);
        tileContent.addAndMakeVisible(tile);
    }
    infoLabel.setFont(font(12.f));
    infoLabel.setColour(juce::Label::textColourId, pal::inkDim);
    infoLabel.setJustificationType(juce::Justification::topLeft);
    infoLabel.setMinimumHorizontalScale(1.0f);
    infoLabel.setText(juce::String(skl::activeCatalogCount())+" Skell effects. Double-click or drag a module into the rack.",juce::dontSendNotification);
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
    const int anchor=strip.geometry().first(viewport.getViewPositionX());
    bool changed=false;
    for (int s=0;s<kNumSlots;++s) {
        auto& slot=*strip.slots[(size_t)s];
        if(slot.currentType()!=proc.slotType(s)){slot.rebuild();changed=true;}
        else slot.syncPowerVisual();
    }
    strip.resized();
    if(changed)viewport.setViewPosition(strip.geometry().anchor(anchor),0);
    refreshRackNavigation();
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
    strip.resized();
    pathStrip.setFocusedSlot(slot);
    viewport.setViewPosition(strip.geometry().reveal(slot,viewport.getViewPositionX()),0);
    refreshRackNavigation();
}
bool RackUI::slotIsFullyVisible(int slot) const{
    return slot>=0&&slot<kNumSlots&&viewport.getViewArea().contains(strip.slots[static_cast<size_t>(slot)]->getBounds());
}
void RackUI::refreshRackNavigation(){
    const auto geo=strip.geometry();const int scroll=viewport.getViewPositionX();
    pathStrip.setVisibleRange(geo.first(scroll),geo.last(scroll));
}
void RackUI::pageRack(int direction){
    viewport.setViewPosition(strip.geometry().page(direction,viewport.getViewPositionX(),libraryVisible),0);
    refreshRackNavigation();
}
void RackUI::setLibraryVisible(bool visible) {
    const int first=strip.geometry().first(viewport.getViewPositionX());
    libraryVisible=visible;
    library.setVisible(visible);
    libraryToggle.setButtonText(visible?"HIDE LIBRARY":"SHOW LIBRARY");
    libraryToggle.setTooltip(visible?"Hide the module library to see five full rack slots":
                                     "Show the module library to browse, search and add effects");
    resized();
    viewport.setViewPosition(strip.geometry().anchor(first),0);
    refreshRackNavigation();repaint();
}
bool RackUI::fittedRackIsValid() const {
    const auto area=viewport.getViewArea();const auto geo=strip.geometry();
    if(viewport.getRight()!=rackgeo::end(libraryVisible)||area.getWidth()!=geo.view||strip.getWidth()!=geo.total)return false;
    for(int i=0;i<kNumSlots;++i){
        const auto b=strip.slots[static_cast<size_t>(i)]->getBounds();
        if(b.getX()!=geo.x[i]||b.getWidth()!=geo.w[i])return false;
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
    pathStrip.setBounds(25,784,1210,62);
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
    const auto frame=Rectangle<float>(railLeft,103,1252.f-railLeft,675);
    g.setColour(juce::Colours::black.withAlpha(.9f));g.fillRoundedRectangle(frame.expanded(2.f).translated(0.f,2.f),11);
    paintHardwarePlate(g,frame,Colour(0xff262827),true,8.f,4.5f);
    g.setColour(Colour(0xff070605));g.fillRoundedRectangle(frame.reduced(3.5f),6);
    // A machined upper and lower lip, not a flat gold outline.
    auto rail=frame.reduced(5.f).withHeight(6.f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff756f63),rail.getY(),Colour(0xff080807),rail.getBottom()));g.fillRoundedRectangle(rail,2.f);
    rail=rail.withY(frame.getBottom()-6.f);g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff090908),rail.getY(),Colour(0xff746a59),rail.getBottom()));g.fillRoundedRectangle(rail,2.f);
    g.setColour(Colour(0xff0e0c0a));g.drawRoundedRectangle(frame.reduced(4),7,1.7f);
    paintScrew(g,1245,117,4.5f,.6f);paintScrew(g,1245,765,4.5f,1.5f);
    if(!libraryVisible){paintScrew(g,14,117,4.2f,.9f);paintScrew(g,14,765,4.2f,1.3f);}
    const auto foot=Rectangle<float>(8,780,1244,72);
    g.setColour(juce::Colours::black.withAlpha(.72f));g.fillRoundedRectangle(foot.translated(0,2),10);
    paintHardwarePlate(g,foot,Colour(0xff10100e),true,12.f,4.5f);
    g.setColour(Colour(0xff100f0e).withAlpha(.22f));g.fillRoundedRectangle(foot.reduced(5.f),7.f);
    paintScrew(g,25,816,6.f,.8f);paintScrew(g,1235,816,6.f,1.4f);
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

