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
    else {
        const int size=juce::jmax(1,juce::jmin(getWidth()-4,getHeight()-32,144));
        const int top=juce::jmax(0,(getHeight()-size-30)/2);
        slider.setBounds((getWidth()-size)/2,top,size,size);
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
        const auto readout=getLocalBounds().removeFromLeft(getWidth()/2).reduced(5,0);
        g.setColour(lightFace_?Colour(0xff20262b):pal::ink);g.setFont(font(12,true));
        g.drawFittedText(label_,readout.withHeight(getHeight()/2),juce::Justification::bottomLeft,1);
        g.setColour(lightFace_?Colour(0xff41494e):pal::inkDim);g.setFont(font(11));
        g.drawFittedText(text_,readout.withTrimmedTop(getHeight()/2),juce::Justification::topLeft,1);
        return;
    }
    const int y=juce::jmin(getHeight()-28,slider.getBottom()+2);
    g.setColour(lightFace_?Colour(0xff17211e):Colour(0xfff4eee1));g.setFont(font(12.5f,true));
    g.drawFittedText(label_,Rectangle<int>(2,y,getWidth()-4,14),juce::Justification::centred,1);
    g.setColour(lightFace_?Colour(0xff34423c):Colour(0xffd0d3c8));g.setFont(font(11.5f));
    g.drawFittedText(text_,Rectangle<int>(2,y+14,getWidth()-4,14),juce::Justification::centred,1);
}

// =================================================================== small displays
void LcdDisplay::paint(Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(Colour(0xff1a1b1d)); g.fillRoundedRectangle(r, 4.0f);
    auto in = r.reduced(2.0f);
    g.setColour(pal::lcdBg); g.fillRoundedRectangle(in, 3.0f);
    g.setColour(pal::lcdInk.withAlpha(0.06f));
    for (float y = in.getY(); y < in.getBottom(); y += 3.0f) g.drawHorizontalLine((int) y, in.getX(), in.getRight());
    g.setColour(juce::Colours::black.withAlpha(0.8f)); g.drawRoundedRectangle(in, 3.0f, 1.5f);
    g.setColour(pal::lcdInk.withAlpha(0.35f)); g.setFont(font(13.0f, true));
    g.drawText(text, in.reduced(10.0f, 0.0f), juce::Justification::centredLeft, true);
    g.setColour(pal::lcdInk); g.drawText(text, in.reduced(10.0f, 0.0f).translated(0.0f, -0.4f), juce::Justification::centredLeft, true);
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
            const Colour c = pos > 0.9f ? pal::warn : pos > 0.72f ? pal::amber : Colour(0xff5ec48a);
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
    for (auto* b : {&leftBtn, &rightBtn, &removeBtn}) b->getProperties().set("plain", true);
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
    needle = 0.0f; shownNeedle = -1.0f; lastMeterTime=0;
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
    artButton.setButtonText(names[choice]);
    for(auto& c:cells){
        c->setNativeFace(true);c->setLightFace(spec.light);
        c->slider.getProperties().set("cap",(int)spec.cap.getARGB());
        c->slider.getProperties().set("ptr",(int)(spec.light?juce::Colours::white:juce::Colours::black).getARGB());
        c->slider.repaint();
    }
    resized();repaint();
}

bool SlotComponent::artworkLayoutIsValid() const {
    if(type==Empty)return cells.empty();
    if(cells.size()!=static_cast<size_t>(info(type).numKnobs))return false;
    const auto body=getLocalBounds().withTrimmedTop(78).withTrimmedBottom(48);
    for(size_t i=0;i<cells.size();++i){
        const auto& c=*cells[i];
        if(!body.contains(c.getBounds())||!c.getLocalBounds().contains(c.slider.getBounds())||c.getBounds().intersects(vuRect)||c.getBounds().intersects(footRect))return false;
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
    powerBtn.setBounds(12,9,27,27);removeBtn.setBounds(w-29,10,21,24);
    rightBtn.setBounds(w-54,10,21,24);leftBtn.setBounds(w-79,10,21,24);
    soloBtn.setBounds(12,h-40,30,26);artButton.setBounds(48,h-40,w-60,26);
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
    const Colour ink=p.light?Colour(0xff202928):Colour(0xffeee9dc);
    if(!panelCache.isValid()||panelCache.getWidth()!=w||panelCache.getHeight()!=h){
        panelCache=juce::Image(juce::Image::ARGB,w,h,true);Graphics pg(panelCache);
        paintOriginalSurface(pg,getLocalBounds().toFloat(),type,nativeChoice);
        pg.setColour(Colour(0xff15191a));pg.fillRect(6,4,w-12,36);
        pg.setColour(Colour(0xffd2d9cc));pg.setFont(font(11,true));
        pg.drawText(slotLetter(index),40,8,21,28,juce::Justification::centred,false);
        pg.setColour(ink);pg.setFont(font(10,true));
        pg.drawText("SKELLIERE  /  MODEL "+juce::String(type).paddedLeft('0',2),18,42,w-36,16,juce::Justification::centred,false);
        pg.setFont(font(18,true));pg.drawFittedText(juce::String(displayName(type)).toUpperCase(),Rectangle<int>(12,57,w-24,25),juce::Justification::centred,1);
        pg.setColour(p.accent.withAlpha(.8f));pg.fillRect(18,mi.hasVU?80:91,w-36,2);
        pg.setColour(ink.withAlpha(.64f));pg.setFont(font(10,true));pg.drawText(p.series,16,517,w-32,15,juce::Justification::centred,false);
        pg.setColour(Colour(0xff15191a));pg.fillRect(6,h-45,w-12,39);
    }
    g.drawImageAt(panelCache,0,0);
    g.setColour(powerBtn.getToggleState()?Colour(0xffb7dfa3):Colour(0xff858a82));g.setFont(font(9,true));
    g.drawText(powerBtn.getToggleState()?"IN":"OFF",61,8,36,28,juce::Justification::centredLeft,false);
    if(mi.hasVU&&!vuRect.isEmpty()){
        paintVU(g,vuRect.toFloat(),needle);
        g.setColour(pal::vuInk);g.setFont(font(10,true));
        g.drawText(juce::String(needle<.05f?0.f:-needle,1)+" dB",vuRect.withTrimmedTop(vuRect.getHeight()-16).reduced(6,2),juce::Justification::centredRight,false);
    }
    if(!powerBtn.getToggleState()){g.setColour(juce::Colours::black.withAlpha(.19f));g.fillRect(6,40,w-12,h-85);}
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
    const auto& mi = info(type);
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(Colour(0xff0e0f11)); g.fillRoundedRectangle(r, 3.0f);
    auto sw = r.reduced(5.0f).removeFromLeft(34.0f);
    g.setGradientFill(juce::ColourGradient::vertical(pal::plateTop, sw.getY(), pal::plateBot, sw.getBottom()));
    g.fillRoundedRectangle(sw, 2.0f);
    paintBrushed(g, sw, 0.9f);
    paintOriginalMini(g,sw,type);
    auto text = r.reduced(5.0f).withTrimmedLeft(40.0f);
    g.setColour(pal::ink); g.setFont(font(12.0f, true));
    g.drawFittedText(juce::String(displayName(type)).toUpperCase(),text.removeFromTop(17.0f).toNearestInt(),juce::Justification::centredLeft,1);
    g.setColour(pal::inkDim); g.setFont(font(10.0f));
    g.drawFittedText(mi.sub,text.toNearestInt(),juce::Justification::centredLeft,1);
    g.setColour(selected ? pal::amber : Colour(0xff000000)); g.drawRoundedRectangle(r, 3.0f, selected ? 1.6f : 1.0f);
}

void ModuleTile::mouseDrag(const juce::MouseEvent& e) {
    if (e.getDistanceFromDragStart() < 6) return;
    if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor(this))
        if (!c->isDragAndDropActive()) c->startDragging("mod:" + juce::String(type), this);
}

LibraryPanel::LibraryPanel(SkellierProcessor& p) : proc(p) {
    static const char* names[5] = {"ALL", "DYN", "TONE", "SPACE", "UTIL"};
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
    search.setTextToShowWhenEmpty("Search modules", pal::inkDim);
    search.setFont(font(13.0f));
    search.onTextChange = [this] { refresh(); };
    addAndMakeVisible(search);
    tileViewport.setViewedComponent(&tileContent, false);
    tileViewport.setScrollBarsShown(true, false);
    tileViewport.setScrollBarThickness(10);
    addAndMakeVisible(tileViewport);
    for (int t = Eq; t < kNumRackTypes; ++t) {
        auto* tile = new ModuleTile(t, [this](int type) { select(type); }, [this](int type) { proc.addModule(type, -1); });
        tiles.add(tile);
        tileContent.addAndMakeVisible(tile);
    }
    infoLabel.setFont(font(12.5f));
    infoLabel.setColour(juce::Label::textColourId, pal::inkDim);
    infoLabel.setJustificationType(juce::Justification::topLeft);
    infoLabel.setMinimumHorizontalScale(1.0f);
    infoLabel.setText("25 original Skell effects. Double-click or drag a module into the rack.",juce::dontSendNotification);
    addAndMakeVisible(infoLabel);
    refresh();
}

void LibraryPanel::select(int type) {
    selectedType = type;
    for (auto* t : tiles) { t->selected = t->type == type; t->repaint(); }
    const auto& mi = info(type);
    const juce::String hint = (type == Eq || type == Air)
        ? "\n\nChoose a native faceplate at the bottom of its slot. All knobs remain active."
        : "\n\nDouble-click or drag into a slot.";
    infoLabel.setText(juce::String(displayName(type)).toUpperCase() + "\n" + mi.desc + hint, juce::dontSendNotification);
}

void LibraryPanel::refresh() {
    const auto q = search.getText().trim().toLowerCase();
    int n = 0;
    const int w=juce::jmax(80,tileViewport.getWidth()-12);
    for (auto* t : tiles) {
        const auto& mi = info(t->type);
        const bool show = (category < 0 || (int) mi.cat == category) &&
                          (q.isEmpty() || (juce::String(displayName(type)) + " " + mi.sub).toLowerCase().contains(q));
        t->setVisible(show);
        if(show){t->setBounds(0,n*47,w,43);++n;}
    }
    tileContent.setSize(w,n*47);
}

void LibraryPanel::resized() {
    const int w = getWidth();
    search.setBounds(10, 30, w - 20, 26);
    const int bw = (w - 20 - 12) / 5;
    for (int i = 0; i < 5; ++i) cats[i].setBounds(10 + i * (bw + 3), 61, bw, 24);
    tileViewport.setBounds(10, 92, w - 20, juce::jmax(60, getHeight() - 226));
    refresh();
    infoLabel.setBounds(10, getHeight() - 124, w - 20, 114);
}

void LibraryPanel::paint(Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient::vertical(pal::barTop, 0.0f, pal::barBot, r.getHeight()));
    g.fillRoundedRectangle(r, 4.0f);
    paintBrushed(g, r);
    g.setColour(Colour(0xff000000)); g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);
    g.setColour(juce::Colours::white.withAlpha(0.14f)); g.drawHorizontalLine(1, 6.0f, r.getWidth() - 6.0f);
    g.setColour(pal::inkDim); g.setFont(font(12.0f, true));
    g.drawText("MODULE LIBRARY", Rectangle<float>(12.0f, 8.0f, r.getWidth() - 24.0f, 18.0f), juce::Justification::centredLeft, false);
}

void PathStrip::paint(Graphics& g) {
    g.setColour(pal::inkDim);g.setFont(font(11,true));
    g.drawText("SIGNAL PATH",4,0,96,getHeight(),juce::Justification::centredLeft,false);
    int count=0;for(int s=0;s<kNumSlots;++s)if(proc.slotType(s)!=Empty)++count;
    int x=104;const int remaining=getWidth()-104-2*36-12*(count+1);
    const int cell=juce::jmax(38,juce::jmin(172,remaining/juce::jmax(1,count)));
    auto key=[&](juce::String text,int width,bool lit){
        auto r=Rectangle<int>(x,5,width,getHeight()-10);
        g.setColour(Colour(0xff101315));g.fillRoundedRectangle(r.toFloat(),3);
        g.setColour(lit?pal::ink:pal::inkDim);g.setFont(font(10.5f,true));
        g.drawFittedText(text,r.reduced(4,1),juce::Justification::centred,1);x+=width;
    };
    auto arrow=[&]{g.setColour(pal::inkDim);g.drawText(">",x,0,12,getHeight(),juce::Justification::centred,false);x+=12;};
    key("IN",36,true);
    for(int s=0;s<kNumSlots;++s){const int t=proc.slotType(s);if(t==Empty)continue;
        arrow();key(slotLetter(s)+" "+juce::String(displayName(t)).toUpperCase(),cell,proc.slotOn(s));}
    arrow();key("OUT",36,true);
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
    undoBtn.onClick = [this] { proc.undo(); say("Undo"); };
    redoBtn.onClick = [this] { proc.redo(); say("Redo"); };
    clearBtn.onClick = [this] { proc.clearRack(); say("Rack cleared. Undo brings it back."); };
    bypassBtn.setClickingTogglesState(true);
    bypassAtt = std::make_unique<juce::ButtonParameterAttachment>(*proc.apvts.getParameter("bypass"), bypassBtn);
    static const char* zt[4] = {"80%", "90%", "100%", "125%"};
    static const float zv[4] = {0.8f, 0.9f, 1.0f, 1.25f};
    for (int i = 0; i < 4; ++i) {
        scaleBtn[i].setButtonText(zt[i]);
        scaleBtn[i].setClickingTogglesState(true);
        scaleBtn[i].setRadioGroupId(42);
        scaleBtn[i].getProperties().set("plain", true);
        scaleBtn[i].onClick = [this, i] { if (onScale) onScale(zv[i]); };
        addAndMakeVisible(scaleBtn[i]);
    }
    scaleBtn[2].setToggleState(true, juce::dontSendNotification);
    lcd.setText("SKELLIERE RACK  /  25 SKELL EFFECTS");
    addAndMakeVisible(lcd); addAndMakeVisible(meters); addAndMakeVisible(inKnob); addAndMakeVisible(outKnob);
    stripsLabel.setText("DREAM STRIPS", juce::dontSendNotification);
    stripsLabel.setFont(font(12.0f, true));
    stripsLabel.setColour(juce::Label::textColourId, pal::inkDim);
    addAndMakeVisible(stripsLabel);
    defaultStrip.onClick = [this] { curPreset = 0; presetBox.setSelectedItemIndex(0, juce::sendNotificationSync); };
    addAndMakeVisible(defaultStrip);
    stripName.setTextToShowWhenEmpty("Name this rack", pal::inkDim);
    stripName.setFont(font(13.0f));
    addAndMakeVisible(stripName);
    saveStripBtn.onClick = [this] {
        const auto name = stripName.getText().trim();
        if (name.isEmpty()) { say("Type a name for the strip first."); return; }
        if (proc.saveStrip(name)) { stripName.clear(); rebuildStripButtons(); say("Saved dream strip: " + name); }
        else say("Could not save the strip. Check folder permissions.");
    };
    addAndMakeVisible(saveStripBtn);
    rebuildStripButtons();
    addAndMakeVisible(library);
    viewport.setViewedComponent(&strip, false);
    viewport.setScrollBarsShown(false, true);
    viewport.setScrollBarThickness(12);
    addAndMakeVisible(viewport);
    addAndMakeVisible(pathStrip);
    for (int s = 0; s < kNumSlots; ++s) { proc.apvts.addParameterListener(SkellierProcessor::typeId(s), this); proc.apvts.addParameterListener(SkellierProcessor::extraId(s), this); proc.apvts.addParameterListener(SkellierProcessor::onId(s), this); }
    startTimerHz(30);
}

RackUI::~RackUI() {
    stopTimer();
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
        addAndMakeVisible(b);
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
    auto toDb = [](float g) { return g > 1.0e-5f ? 20.0f * std::log10(g) : -90.0f; };
    inShown = juce::jmax(toDb(proc.takeInputPeak()), inShown - 2.0f);
    outShown = juce::jmax(toDb(proc.takeOutputPeak()), outShown - 2.0f);
    meters.setLevels(inShown, outShown);
    for (int s = 0; s < kNumSlots; ++s) {
        strip.slots[(size_t) s]->setMeter(proc.gainReduction(s));
        strip.slots[(size_t) s]->syncSolo();
        strip.slots[(size_t) s]->syncArtwork();
    }
    undoBtn.setEnabled(proc.canUndo());
    redoBtn.setEnabled(proc.canRedo());
}

void RackUI::resized() {
    prevPreset.setBounds(290, 18, 28, 32);
    presetBox.setBounds(322, 18, 180, 32);
    nextPreset.setBounds(506, 18, 28, 32);
    snapA.setBounds(552, 18, 34, 32);
    snapCopy.setBounds(590, 18, 54, 32);
    snapB.setBounds(648, 18, 34, 32);
    undoBtn.setBounds(704, 18, 60, 32);
    redoBtn.setBounds(768, 18, 60, 32);
    clearBtn.setBounds(832, 18, 66, 32);
    bypassBtn.setBounds(944, 18, 92, 32);
    for (int i = 0; i < 4; ++i) scaleBtn[i].setBounds(1044 + i * 48, 18, 44, 32);
    lcd.setBounds(24, 72, 600, 40);
    inKnob.setBounds(644, 68, 130, 48);
    meters.setBounds(780, 72, 300, 40);
    outKnob.setBounds(1096, 68, 130, 48);
    stripsLabel.setBounds(24, 130, 120, 32);
    defaultStrip.setBounds(150, 134, 90, 32);
    for (int i = 0; i < stripButtons.size(); ++i) stripButtons[i]->setBounds(246 + i * 104, 134, 100, 32);
    stripName.setBounds(1000, 134, 140, 32);
    saveStripBtn.setBounds(1146, 134, 92, 32);
    library.setBounds(12, 174, 224, 600);
    viewport.setBounds(260, 174, 972, 600);
    pathStrip.setBounds(12, 780, 1236, 32);
}

void RackUI::paint(Graphics& g) {
    g.setGradientFill(juce::ColourGradient::vertical(pal::chassis2, 0.0f, pal::chassis, (float) kH));
    g.fillAll();
    auto bar = [&](Rectangle<int> r) {
        auto f = r.toFloat();
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.fillRoundedRectangle(f.translated(0.0f, 2.0f), 4.0f);
        g.setGradientFill(juce::ColourGradient::vertical(pal::barTop, f.getY(), pal::barBot, f.getBottom()));
        g.fillRoundedRectangle(f, 4.0f);
        paintBrushed(g, f);
        g.setColour(Colour(0xff000000)); g.drawRoundedRectangle(f.reduced(0.5f), 4.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.14f)); g.drawHorizontalLine(r.getY() + 1, f.getX() + 5.0f, f.getRight() - 5.0f);
    };
    bar({12, 10, 1236, 48}); bar({12, 64, 1236, 56}); bar({12, 126, 1236, 40});
    g.setFont(font(23,true));g.setColour(juce::Colours::black.withAlpha(.6f));
    g.drawFittedText("SKELLIERE RACK",Rectangle<int>(29,14,250,28),juce::Justification::centredLeft,1);
    g.setColour(pal::ink);g.drawFittedText("SKELLIERE RACK",Rectangle<int>(28,13,250,28),juce::Justification::centredLeft,1);
    g.setColour(Colour(0xffb8dca3));g.setFont(font(9,true));
    g.drawText("SKELL AUDIO  /  ANALOG COLLECTION",29,41,245,12,juce::Justification::centredLeft,false);

    const Rectangle<int> frame(244, 174, 1004, 600);
    g.setColour(Colour(0xff030304)); g.fillRoundedRectangle(frame.toFloat(), 5.0f);
    for (int side = 0; side < 2; ++side) {
        auto c = Rectangle<float>(side ? 1232.0f : 244.0f, 174.0f, 16.0f, 600.0f);
        g.setGradientFill(juce::ColourGradient(Colour(0xff0b0c0d), c.getX(), 0.0f, Colour(0xff35383d), c.getRight(), 0.0f, false));
        g.fillRect(c);
        paintBrushed(g, c);
        g.setColour(juce::Colours::white.withAlpha(0.12f)); g.drawVerticalLine((int) (side ? c.getX() : c.getRight() - 1), c.getY(), c.getBottom());
        paintScrew(g, c.getCentreX(), c.getY() + 14.0f, 4.0f, 0.4f);
        paintScrew(g, c.getCentreX(), c.getBottom() - 14.0f, 4.0f, 1.1f);
    }
    for (int rail = 0; rail < 2; ++rail) {
        auto r = Rectangle<float>(260.0f, rail ? 752.0f : 174.0f, 972.0f, 22.0f);
        g.setGradientFill(juce::ColourGradient::vertical(rail ? Colour(0xff23262a) : Colour(0xff464a51), r.getY(), rail ? Colour(0xff464a51) : Colour(0xff23262a), r.getBottom()));
        g.fillRect(r);
        paintBrushed(g, r);
        for (float x = r.getX() + 17.0f; x < r.getRight() - 8.0f; x += 34.0f) {
            g.setColour(Colour(0xff020202)); g.fillEllipse(x - 3.4f, r.getCentreY() - 3.4f, 6.8f, 6.8f);
            g.setColour(juce::Colours::white.withAlpha(0.22f)); g.drawEllipse(x - 3.9f, r.getCentreY() - 3.9f, 7.8f, 7.8f, 0.8f);
        }
    }
    g.setColour(Colour(0xff050506)); g.fillRect(Rectangle<int>(260, 196, 972, 556));
    // The rack rail is hardware framing; do not paint development text over
    // the bottom of the viewport or the actual module footers.
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

