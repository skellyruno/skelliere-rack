#include "PluginEditor.h"
#include "MeterCalibration.h"

using namespace skl;
using namespace skl::ui;
using juce::Graphics;
using juce::Rectangle;

static juce::String slotLetter(int i) { return juce::String::charToString((juce::juce_wchar) ('A' + i)); }
static int textWidth(const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidthInt(f, s); }

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
    if(compact_)slider.setBounds(0,0,getHeight(),getHeight());
    else if(sideReadout_){
        const int size=juce::jmin(getHeight()-4,getWidth()/2+8,118);
        slider.setBounds(2,(getHeight()-size)/2,size,size);
    } else {
        const int size=juce::jmax(1,juce::jmin(getWidth()-4,getHeight()-30,154));
        slider.setBounds((getWidth()-size)/2,juce::jmax(0,(getHeight()-size-28)/2),size,size);
    }
}

void KnobCell::paint(Graphics& g) {
    if (compact_) {
        auto r = getLocalBounds().withTrimmedLeft(getHeight() + 4);
        g.setColour(pal::ink); g.setFont(font(12.0f, true));
        g.drawText(label_, r.removeFromTop(getHeight() / 2 + 2), juce::Justification::bottomLeft, false);
        g.setColour(pal::inkDim); g.setFont(font(11.0f));
        g.drawText(text_, r, juce::Justification::topLeft, false);
        return;
    }
    if(sideReadout_) {
        const auto readout=getLocalBounds().withTrimmedLeft(slider.getRight()+5).reduced(2,0);
        const int cy=getHeight()/2;
        g.setColour(lightFace_?Colour(0xff292219):pal::ink);g.setFont(font(14,true).withExtraKerningFactor(.035f));
        g.drawFittedText(label_,readout.withY(cy-24).withHeight(21),juce::Justification::centred,1);
        auto pill=readout.withY(cy+1).withHeight(22).toFloat().reduced(2,0);
        g.setColour(lightFace_?Colour(0xff30291f).withAlpha(.12f):Colour(0xff10161b).withAlpha(.45f));g.fillRoundedRectangle(pill,10);
        g.setColour((lightFace_?Colour(0xff504638):pal::ink).withAlpha(.55f));g.drawRoundedRectangle(pill,10,.8f);
        g.setColour(lightFace_?Colour(0xff292219):Colour(0xffeee3cc));g.setFont(font(12.5f));
        g.drawFittedText(text_,pill.reduced(4,1).toNearestInt(),juce::Justification::centred,1);
        return;
    }
    const int y=juce::jmin(getHeight()-30,slider.getBottom()+1);
    g.setColour(lightFace_?Colour(0xff211b14):Colour(0xffffecd1));g.setFont(font(13.5f,true).withExtraKerningFactor(.035f));
    g.drawFittedText(label_,Rectangle<int>(2,y,getWidth()-4,15),juce::Justification::centred,1);
    g.setColour(lightFace_?Colour(0xff30261b):Colour(0xffe0d4bd));g.setFont(font(12.5f));
    g.drawFittedText(text_,Rectangle<int>(2,y+15,getWidth()-4,15),juce::Justification::centred,1);
}

// =================================================================== small displays
void LcdDisplay::paint(Graphics& g) {
    g.setColour(pal::inkDim);g.setFont(font(10.5f).withExtraKerningFactor(.09f));
    g.drawFittedText(text,getLocalBounds().reduced(3,0),juce::Justification::centredLeft,1);
}

void LevelMeter::paint(Graphics& g) {
    auto r = getLocalBounds().toFloat();
    auto bar = [&](Rectangle<float> a, float db, const juce::String& name) {
        g.setColour(pal::inkDim); g.setFont(font(10.5f, true));
        g.drawText(name, a.removeFromLeft(26.0f), juce::Justification::centredLeft, false);
        g.setColour(Colour(0xff050506)); g.fillRoundedRectangle(a, 2.0f);
        const int segs = 40;
        const float lit = juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f) * (float) segs;
        const float sw = a.getWidth() / (float) segs;
        for (int i = 0; i < segs; ++i) {
            const float pos = (float) i / (float) segs;
            const Colour c=pos>.9f?Colour(0xffc67838):pos>.72f?Colour(0xffd6bb68):Colour(0xff82a86b);
            g.setColour(i < (int) lit ? c : c.withAlpha(0.12f));
            g.fillRect(a.getX() + (float) i * sw + 0.6f, a.getY() + 1.5f, sw - 1.2f, a.getHeight() - 3.0f);
        }
    };
    bar(r.removeFromTop(r.getHeight() * 0.5f).reduced(0.0f, 2.0f), inDb_, "IN");
    bar(r.reduced(0.0f, 2.0f), outDb_, "OUT");
}

void PowerButton::paintButton(Graphics& g, bool hover, bool down) {
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    const float cx = r.getCentreX(), cy = r.getCentreY(), R = juce::jmin(r.getWidth(), r.getHeight()) * 0.5f;
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillEllipse(cx - R, cy - R + 1.5f, 2 * R, 2 * R);
    g.setGradientFill(juce::ColourGradient(Colour(0xfffafafa), cx - R * 0.4f, cy - R * 0.6f, Colour(0xff666666), cx + R * 0.7f, cy + R, true));
    g.fillEllipse(cx - R, cy - R, 2 * R, 2 * R);
    g.setColour(Colour(0xff1b1b1b)); g.drawEllipse(cx - R, cy - R, 2 * R, 2 * R, 1.0f);
    const float lr = R * 0.52f;
    if (getToggleState()) {
        g.setColour(pal::amber.withAlpha(0.35f)); g.fillEllipse(cx - lr * 1.8f, cy - lr * 1.8f, lr * 3.6f, lr * 3.6f);
        g.setGradientFill(juce::ColourGradient(Colour(0xfffff7cf), cx - lr * 0.3f, cy - lr * 0.4f, pal::amber.darker(0.4f), cx + lr, cy + lr, true));
    } else {
        g.setColour(Colour(0xff4d4630));
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
    const bool hasArtwork = filled;
    artButton.setVisible(hasArtwork);
    appliedArtworkChoice = -1; panelCache={};
    for (auto* c : std::initializer_list<juce::Component*>{&powerBtn, &leftBtn, &rightBtn, &removeBtn, &soloBtn}) c->setVisible(filled);
    if (filled) {
        const auto& mi = info(type);
        for (int j = 0; j < mi.numKnobs; ++j) {
            const Knob k = mi.knobs[j];
            auto* param = proc.apvts.getParameter(SkellierProcessor::knobId(index, j));
            auto cell = std::make_unique<KnobCell>(k.label, *param, [k](double v) { return knobText(k, (float) v); },
                                                   capColour(mi.cat), pointerColour(mi.cat), (double) defaultNorm(type, j),
                                                   k.steps > 1 ? 1.0 / (double) (k.steps - 1) : 0.0);
            addAndMakeVisible(*cell);
            cells.push_back(std::move(cell));
        }
    }
    needle = 0.0f; shownNeedle = -1.0f; lastMeterTime=0;outputDb=-60.f;
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
    if(choice==appliedArtworkChoice)return;
    appliedArtworkChoice=choice;nativeChoice=choice;panelCache={};
    const auto spec=originalSpec(type,choice);
    static const char* names[]={"GRAPHITE","SIGNATURE","SILVER","MIDNIGHT"};
    artButton.setButtonText("FINISH");artButton.setTooltip(juce::String("Metal finish: ")+names[choice]+" (click to cycle)");
    for(auto& c:cells){
        c->setNativeFace(true,originalLayout(type,preferredWidth()).sideReadouts);c->setLightFace(spec.light);
        c->slider.getProperties().set("cap",(int)spec.cap.getARGB());
        c->slider.getProperties().set("ptr",(int)Colour(0xff191510).getARGB());
        c->slider.repaint();
    }
    resized();repaint();
}

bool SlotComponent::artworkLayoutIsValid() const {
    if(type==Empty)return cells.empty();
    if(cells.size()!=static_cast<size_t>(info(type).numKnobs))return false;
    const auto body=getLocalBounds().withTrimmedTop(100).withTrimmedBottom(46);
    for(size_t i=0;i<cells.size();++i){
        const auto& c=*cells[i];
        if(!body.contains(c.getBounds())||!c.getLocalBounds().contains(c.slider.getBounds())||c.getBounds().intersects(vuRect)||c.getBounds().intersects(footRect)||c.getBounds().intersects(originalLayout(type,getWidth()).detail))return false;
        for(size_t j=0;j<i;++j)if(c.getBounds().intersects(cells[j]->getBounds()))return false;
    }
    return true;
}

bool SlotComponent::exerciseSpriteBindings() {
    if(cells.empty())return true;
    bool ok=true;
    for(size_t i=0;i<cells.size();++i){
        auto* p=proc.apvts.getParameter(SkellierProcessor::knobId(index,static_cast<int>(i)));
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
    if(type!=Flanger||vuRect.isEmpty())return;
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

void SlotComponent::resized() {
    sectionRects.clear();vuRect={};panelBounds={};
    const int w=getWidth(),h=getHeight();footRect={10,h-43,w-20,32};
    if(type==Empty)return;
    powerBtn.setBounds(16,h-39,25,25);soloBtn.setBounds(49,h-38,27,23);
    leftBtn.setBounds(82,h-38,21,23);rightBtn.setBounds(106,h-38,21,23);
    artButton.setBounds(130,h-38,62,23);removeBtn.setBounds(w-35,h-38,20,23);
    const auto layout=originalLayout(type,w);vuRect=layout.meter;
    for(size_t i=0;i<cells.size();++i)cells[i]->setBounds(layout.cells[i]);
}

void SlotComponent::paintEmpty(Graphics& g, Rectangle<float> b) {
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff1b1d20),0.0f,Colour(0xff111214),b.getHeight()));
    g.fillRect(b);
    paintBrushed(g,b,0.35f);
    auto inner=b.reduced(13.0f,18.0f);
    g.setColour(Colour(0xff07080a).withAlpha(.42f));
    g.fillRoundedRectangle(inner,4.0f);
    g.setColour(Colour(0xff393d43).withAlpha(.52f));
    g.drawRoundedRectangle(inner.reduced(.5f),4.0f,1.0f);
    for(int i=0;i<4;++i)
        paintScrew(g,i%2?b.getWidth()-9.0f:9.0f,i<2?9.0f:b.getHeight()-9.0f,4.0f,0.7f*(index+i));
    const float cx=b.getCentreX(),cy=b.getCentreY();
    g.setColour(dragOver?pal::amber:Colour(0xff71767c).withAlpha(.45f));
    g.drawLine(cx-11.0f,cy,cx+11.0f,cy,1.5f);
    g.drawLine(cx,cy-11.0f,cx,cy+11.0f,1.5f);
    g.setFont(font(10.0f,true));
    g.drawText(dragOver?"DROP MODULE HERE":"EMPTY SLOT",
               Rectangle<float>(10.0f,cy+23.0f,b.getWidth()-20.0f,18.0f),
               juce::Justification::centred,false);
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
        paintMonogram(pg,Rectangle<float>(w*.5f-9,15,18,25),ink);
        pg.setColour(ink);pg.setFont(font(12,true).withExtraKerningFactor(.20f));
        pg.drawText("SKELL",18,43,w-36,16,juce::Justification::centred,false);
        pg.setFont(juce::Font(juce::FontOptions("Georgia",24.f,juce::Font::bold|juce::Font::italic)));
        pg.drawFittedText(juce::String(displayName(type)).fromFirstOccurrenceOf(" ",false,false).toUpperCase(),
            Rectangle<int>(14,60,w-28,29),juce::Justification::centred,1);
        pg.setColour(ink.withAlpha(.38f));
        const int titleWidth=juce::jlimit(70,w-40,juce::String(displayName(type)).fromFirstOccurrenceOf(" ",false,false).length()*13+20);
        pg.drawHorizontalLine(75,20,float(w-titleWidth)*.5f);
        pg.drawHorizontalLine(75,float(w+titleWidth)*.5f,w-20);
        const auto layout=originalLayout(type,w);
        if(!layout.detail.isEmpty())paintTapeDetail(pg,layout.detail.toFloat());
        // Side-mounted value pills provide the separation; no extra grid lines.
        pg.setColour(ink.withAlpha(.24f));pg.drawHorizontalLine(h-46,20,w-20);
        pg.setColour(ink.withAlpha(.68f));pg.setFont(font(8.5f,true));
        pg.drawText(slotLetter(index)+"  /  "+p.series,14,h-57,w-28,10,juce::Justification::centred,false);
    }
    g.drawImage(panelCache,getLocalBounds().toFloat());
    if(type==Flanger&&!vuRect.isEmpty()){
        paintVU(g,vuRect.toFloat(),outputDb,true);
        g.setColour(pal::vuInk);g.setFont(font(10,true));
        g.drawText(outputDb<=-59.f?"-inf":juce::String(outputDb,1)+" dBFS",vuRect.withTrimmedTop(vuRect.getHeight()-23).reduced(10,4),juce::Justification::centredRight,false);
    }else if(mi.hasVU&&!vuRect.isEmpty()){
        paintVU(g,vuRect.toFloat(),needle);
        g.setColour(pal::vuInk);g.setFont(font(10,true));
        g.drawText(juce::String(needle<.05f?0.f:-needle,1)+" dB",vuRect.withTrimmedTop(vuRect.getHeight()-23).reduced(10,4),juce::Justification::centredRight,false);
    }
    if(!powerBtn.getToggleState()){g.setColour(juce::Colours::black.withAlpha(.22f));g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(3),5);}
}

void SlotComponent::paintFaceplate(Graphics& g,Rectangle<float>) {paintNativeFace(g);}

void SlotComponent::paint(Graphics& g) {
    auto b = getLocalBounds().toFloat();
    if (type == Empty) paintEmpty(g, b); else paintFaceplate(g, b);
    if (dragOver) { g.setColour(pal::amber); g.drawRect(b.reduced(2.0f), 2.0f); }
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
    if (s.startsWith("mod:")) proc.addModule(s.substring(4).getIntValue(), index);
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
        const int w=slot->preferredWidth();
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
    auto r=getLocalBounds().toFloat().reduced(.7f);
    const bool active=selected,hover=isMouseOver();
    g.setGradientFill(juce::ColourGradient::vertical(Colour(active?0xff3c3224:hover?0xff302b23:0xff28241e),r.getY(),Colour(active?0xff251e16:0xff151310),r.getBottom()));
    g.fillRoundedRectangle(r,5);
    g.setColour(active?pal::amber:Colour(0xff645b4b).withAlpha(.6f));g.drawRoundedRectangle(r,5,active?1.4f:.7f);
    auto sw=r.reduced(6).removeFromLeft(39);
    // Cache icon at fixed logical dimensions (not the whole tile or its hover state).
    static std::array<juce::Image,skl::kNumRackTypes> minis;
    auto& mini=minis[static_cast<size_t>(type)];
    if(!mini.isValid()){
        mini=juce::Image(juce::Image::ARGB,78,66,true);Graphics mg(mini);
        mg.addTransform(juce::AffineTransform::scale(2.f));paintOriginalMini(mg,Rectangle<float>(0,0,39,33),type);
    }
    g.drawImage(mini,sw);
    auto text=r.reduced(6).withTrimmedLeft(48);
    g.setColour(pal::ink);g.setFont(font(14,true));
    g.drawFittedText(juce::String(displayName(type)),text.removeFromTop(19).toNearestInt(),juce::Justification::centredLeft,1);
    g.setColour(pal::inkDim);g.setFont(font(11.f));
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
    search.setFont(font(13.5f));search.setIndents(29,6);search.getProperties().set("skellSearch",true);
    search.onTextChange = [this] { refresh(); };
    addAndMakeVisible(search);
    tileViewport.setViewedComponent(&tileContent, false);
    tileViewport.setScrollBarsShown(true, false);
    tileViewport.setScrollBarThickness(10);
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
    const int w=juce::jmax(80,tileViewport.getWidth()-12);
    for (auto* t : tiles) {
        const auto& mi = info(t->type);
        const bool show = (category < 0 || (int) mi.cat == category) &&
                          (q.isEmpty() || (juce::String(displayName(t->type)) + " " + mi.sub).toLowerCase().contains(q));
        t->setVisible(show);
        if(show){t->setBounds(0,n*52,w,50);++n;}
    }
    tileContent.setSize(w,n*52);
}

void LibraryPanel::resized() {
    const int w=getWidth();search.setBounds(15,42,w-30,32);
    const int bw=(w-30-12)/5;
    for(int k=0;k<5;++k)cats[k].setBounds(15+k*(bw+3),83,bw,27);
    tileViewport.setBounds(13,121,w-26,juce::jmax(60,getHeight()-202));
    infoLabel.setBounds(15,getHeight()-72,w-30,58);
    refresh();
}

void LibraryPanel::paint(Graphics& g) {
    auto r=getLocalBounds().toFloat().reduced(.5f);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff302c26),r.getY(),Colour(0xff191714),r.getBottom()));
    g.fillRoundedRectangle(r,7);
    {juce::Graphics::ScopedSaveState save(g);juce::Path p;p.addRoundedRectangle(r.getX(),r.getY(),r.getWidth(),r.getHeight(),7);g.reduceClipRegion(p);paintBrushed(g,r,.43f);}
    g.setColour(Colour(0xff837765));g.drawRoundedRectangle(r,7,1.f);
    g.setColour(Colour(0xff090807));g.drawRoundedRectangle(r.reduced(2),6,1.2f);
    g.setColour(pal::ink);g.setFont(font(14,true).withExtraKerningFactor(.12f));
    g.drawText("MODULE LIBRARY",r.withHeight(39),juce::Justification::centred,false);
    auto list=r.reduced(11).withY(119).withHeight(getHeight()-200);
    g.setColour(Colour(0xff100e0c));g.fillRoundedRectangle(list,5);
    g.setColour(Colour(0xff51483d));g.drawRoundedRectangle(list,5,.7f);
    for(int k=0;k<4;++k)paintScrew(g,k%2?r.getRight()-9:r.getX()+9,k<2?r.getY()+9:r.getBottom()-9,4.3f,k*.5f);
}

void PathStrip::paint(Graphics& g) {
    const int h=getHeight();g.setColour(pal::ink);g.setFont(font(12,true).withExtraKerningFactor(.12f));
    g.drawText("SIGNAL PATH",24,0,120,h,juce::Justification::centredLeft,false);
    g.setColour(Colour(0xff8b7d66).withAlpha(.65f));g.drawVerticalLine(152,7,h-7);
    const bool bypass=proc.apvts.getRawParameterValue("bypass")->load()>.5f;
    const int solo=proc.soloSlot.load();
    int count=0;for(int s=0;s<kNumSlots;++s)if(proc.slotType(s)!=Empty)++count;
    constexpr int port=55,arrowWidth=20;int x=174;
    const int remaining=getWidth()-x-18-2*port-arrowWidth*(count+1);
    const int cell=juce::jmax(35,remaining/juce::jmax(1,count));
    auto key=[&](const juce::String& text,int width,bool lit,bool model){
        auto r=Rectangle<float>(float(x),4.f,float(width),float(h-8));
        g.setColour(juce::Colours::black.withAlpha(.6f));g.fillRoundedRectangle(r.translated(0,1),5);
        g.setGradientFill(juce::ColourGradient::vertical(Colour(lit?0xff393127:0xff26221d),r.getY(),Colour(0xff171410),r.getBottom()));g.fillRoundedRectangle(r,5);
        g.setColour(lit?Colour(0xffc0a379):Colour(0xff575043));g.drawRoundedRectangle(r,5,lit?1.f:.75f);
        if(model&&lit){g.setColour(Colour(0xffe8c994));g.fillEllipse(r.getX()+5,r.getCentreY()-1.8f,3.6f,3.6f);}
        g.setColour(lit?pal::ink:pal::inkDim);g.setFont(font(count>6?10.5f:13.f));
        g.drawFittedText(text,r.reduced(model?9.f:3.f,1.f).toNearestInt(),juce::Justification::centred,1);x+=width;
    };
    auto arrow=[&]{g.setColour(pal::inkDim);juce::Path p;const float cy=h*.5f,cx=x+arrowWidth*.5f;
        p.startNewSubPath(cx-2.5f,cy-4);p.lineTo(cx+1.5f,cy);p.lineTo(cx-2.5f,cy+4);g.strokePath(p,juce::PathStrokeType(1.2f));x+=arrowWidth;};
    key("IN",port,true,false);
    for(int s=0;s<kNumSlots;++s){const int t=proc.slotType(s);if(t==Empty)continue;arrow();
        key(juce::String(displayName(t)),cell,!bypass&&proc.slotOn(s)&&(solo<0||solo==s),true);}
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
    lcd.setText("SKELL AUDIO   /   ANALOG COLLECTION");
    addAndMakeVisible(lcd); addAndMakeVisible(meters); addAndMakeVisible(inKnob); addAndMakeVisible(outKnob);
    stripsLabel.setText("DREAM STRIPS", juce::dontSendNotification);
    stripsLabel.setFont(font(12.0f, true));
    stripsLabel.setColour(juce::Label::textColourId, pal::inkDim);
    addChildComponent(stripDrawer);
    addAndMakeVisible(stripsToggle);
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
    viewport.setViewedComponent(&strip, false);
    viewport.setScrollBarsShown(false, true);
    viewport.setScrollBarThickness(8);
    addAndMakeVisible(viewport);
    addAndMakeVisible(pathStrip);
    stripDrawer.toFront(false);
    for (int s = 0; s < kNumSlots; ++s) { proc.apvts.addParameterListener(SkellierProcessor::typeId(s), this); proc.apvts.addParameterListener(SkellierProcessor::extraId(s), this); proc.apvts.addParameterListener(SkellierProcessor::onId(s), this); }
    proc.apvts.addParameterListener("bypass",this);
    startTimerHz(30);
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
    const int solo=proc.soloSlot.load();
    if(solo!=shownSolo){shownSolo=solo;pathStrip.repaint();}
    const bool bypass=proc.apvts.getRawParameterValue("bypass")->load()>.5f;
    auto toDb = [](float g) { return g > 1.0e-5f ? 20.0f * std::log10(g) : -90.0f; };
    inShown = juce::jmax(toDb(proc.takeInputPeak()), inShown - 2.0f);
    outShown = juce::jmax(toDb(proc.takeOutputPeak()), outShown - 2.0f);
    meters.setLevels(inShown, outShown);
    for (int s = 0; s < kNumSlots; ++s) {
        strip.slots[(size_t) s]->setMeter(proc.gainReduction(s));
        if(proc.slotType(s)==Flanger){
            const float peak=proc.takeJetPeak(s);
            strip.slots[(size_t)s]->setOutputPeak(!bypass&&proc.slotOn(s)&&(solo<0||solo==s)?peak:0.f);
        }
        strip.slots[(size_t) s]->syncSolo();
        strip.slots[(size_t) s]->syncArtwork();
    }
    undoBtn.setEnabled(proc.canUndo());
    redoBtn.setEnabled(proc.canRedo());
}

void RackUI::resized() {
    prevPreset.setBounds(335,16,30,36);presetBox.setBounds(370,16,208,36);nextPreset.setBounds(583,16,30,36);
    inKnob.setBounds(644,8,127,54);meters.setBounds(777,18,306,32);outKnob.setBounds(1100,8,137,54);
    lcd.setBounds(24,67,300,28);
    snapA.setBounds(335,66,30,27);snapB.setBounds(368,66,30,27);snapCopy.setBounds(402,66,49,27);
    undoBtn.setBounds(466,66,51,27);redoBtn.setBounds(520,66,51,27);clearBtn.setBounds(574,66,51,27);
    stripsToggle.setBounds(642,66,69,27);bypassBtn.setBounds(719,66,76,27);
    for(int k=0;k<5;++k)scaleBtn[k].setBounds(1005+k*47,66,43,27);
    stripDrawer.setBounds(12,104,1236,49);
    stripsLabel.setBounds(10,9,104,30);defaultStrip.setBounds(116,9,82,30);
    for(int k=0;k<stripButtons.size();++k)stripButtons[k]->setBounds(203+k*110,9,105,30);
    stripName.setBounds(892,9,192,30);saveStripBtn.setBounds(1090,9,132,30);
    library.setBounds(12,106,246,666);
    viewport.setBounds(270,106,964,666);
    pathStrip.setBounds(25,780,1210,32);
}

void RackUI::paint(Graphics& g) {
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff2b2823),0.f,Colour(0xff151310),float(kH)));
    g.fillAll();paintBrushed(g,getLocalBounds().toFloat(),.37f);
    g.setColour(Colour(0xff777064).withAlpha(.5f));g.drawHorizontalLine(61,20,1240);
    g.drawHorizontalLine(100,12,1248);
    paintMonogram(g,Rectangle<float>(25,15,24,35),pal::ink);
    g.setColour(pal::ink);g.setFont(font(25,true).withExtraKerningFactor(.085f));
    g.drawFittedText("SKELLIERE RACK",Rectangle<int>(63,10,258,43),juce::Justification::centredLeft,1);
    g.setColour(Colour(0xffa0927b).withAlpha(.45f));
    g.drawVerticalLine(322,13,54);g.drawVerticalLine(628,13,54);
    const auto frame=Rectangle<float>(262,102,986,674);
    g.setColour(Colour(0xff0d0c0a));g.fillRoundedRectangle(frame,8);
    g.setColour(Colour(0xff94846c));g.drawRoundedRectangle(frame.reduced(.8f),8,1.4f);
    g.setColour(Colour(0xff352d22));g.drawRoundedRectangle(frame.reduced(3),5,1.7f);
    paintScrew(g,1242,114,4.2f,.6f);paintScrew(g,1242,765,4.2f,1.5f);
    const auto foot=Rectangle<float>(12,778,1236,36);
    g.setGradientFill(juce::ColourGradient::vertical(Colour(0xff343029),foot.getY(),Colour(0xff1b1814),foot.getBottom()));g.fillRoundedRectangle(foot,6);
    g.setColour(Colour(0xff7d6f5c));g.drawRoundedRectangle(foot.reduced(.7f),6,1);
    g.setColour(Colour(0xff080706));g.drawRoundedRectangle(foot.reduced(2),5,.8f);
    paintScrew(g,20,796,3.6f,.8f);paintScrew(g,1240,796,3.6f,1.4f);
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

