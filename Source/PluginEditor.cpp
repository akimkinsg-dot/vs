#include "PluginEditor.h"

VibeSparkEditor::VibeSparkEditor(VibeSparkProcessor& p) : AudioProcessorEditor(&p), proc(p), pad(p), drag(p) {
  setSize(520, 672);
  keyBox.addItemList(juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B","Free (Random Key)"}, 1);
  voiBox.addItemList(juce::StringArray{"Close Voicing","Open Voicing"}, 1);
  octBox.addItemList(juce::StringArray{"Oct: -1","Oct: 0","Oct: +1"}, 1);

  keyBox.setSelectedItemIndex(proc.key, juce::dontSendNotification);
  voiBox.setSelectedItemIndex(proc.open, juce::dontSendNotification);
  octBox.setSelectedItemIndex(proc.octaveShift + 1, juce::dontSendNotification);

  keyBox.onChange = [this] { proc.setKey(keyBox.getSelectedItemIndex()); };
  voiBox.onChange = [this] { proc.open = voiBox.getSelectedItemIndex(); proc.revoice(); };
  octBox.onChange = [this] { proc.octaveShift = octBox.getSelectedItemIndex() - 1; };

  resBox.addItemList(juce::StringArray{"End: Home","End: Hang","End: Twist"}, 1);
  resBox.setSelectedItemIndex(proc.res.load(), juce::dontSendNotification);
  resBox.onChange = [this] { proc.res = resBox.getSelectedItemIndex(); proc.padChanged(); };
  initCtl(cNost, "Nostalgia", proc.nost.load(), [this](float v) { proc.nost = v; proc.padChanged(); });
  initCtl(cAir, "Air", proc.air.load(), [this](float v) { proc.air = v; proc.padChanged(); });
  initCtl(cTear, "Tear", proc.tear.load(), [this](float v) { proc.tear = v; proc.padChanged(); });
  initCtl(cHuman, "Human", proc.human.load(), [this](float v) { proc.human = v; });

  genBtn.setButtonText("Generate Vibe"); genBtn.onClick = [this] { proc.generate(); };
  contBtn.setButtonText("Evolve Loop"); contBtn.onClick = [this] { proc.evolve(); };

  preBtn.setClickingTogglesState(true);
  preBtn.setToggleState(proc.preview.load(), juce::dontSendNotification);
  preBtn.setButtonText(proc.preview.load() ? "Stop" : "Preview");
  preBtn.onClick = [this] {
    bool on = preBtn.getToggleState();
    proc.setPreview(on);
    preBtn.setButtonText(on ? "Stop" : "Preview");
  };

  nobassBtn.setButtonText("No Bass (808 Safe)");
  nobassBtn.setClickingTogglesState(true);
  nobassBtn.setToggleState(proc.noBass.load(), juce::dontSendNotification);
  nobassBtn.onClick = [this] { proc.noBass = nobassBtn.getToggleState(); };

  strumBtn.setButtonText("Strum");
  strumBtn.setClickingTogglesState(true);
  strumBtn.setToggleState(proc.strum.load(), juce::dontSendNotification);
  strumBtn.onClick = [this] { proc.strum = strumBtn.getToggleState(); };

  label.setJustificationType(juce::Justification::centred); label.setFont(14.0f);
  label.setColour(juce::Label::textColourId, juce::Colours::white);

  for (juce::Component* c : std::initializer_list<juce::Component*>{&keyBox, &voiBox, &octBox, &resBox, &pad, &genBtn, &contBtn, &preBtn, &nobassBtn, &strumBtn, &label, &drag})
    addAndMakeVisible(c);
  startTimerHz(4); timerCallback();
}

VibeSparkEditor::~VibeSparkEditor() {
  stopTimer();
}

void VibeSparkEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colour(0xff14101f));
  g.setColour(juce::Colour(0xffb08cff)); g.setFont(18.0f);
  g.drawText("VIBESPARK // Mood Pad", 14, 8, 360, 24, juce::Justification::left);
}

void VibeSparkEditor::resized() {
  auto r = getLocalBounds().reduced(14); r.removeFromTop(28);
  auto row = r.removeFromTop(28); int w3 = row.getWidth() / 3;
  keyBox.setBounds(row.removeFromLeft(w3).reduced(2, 0));
  voiBox.setBounds(row.removeFromLeft(w3).reduced(2, 0));
  octBox.setBounds(row.reduced(2, 0));

  r.removeFromTop(6); pad.setBounds(r.removeFromTop(MoodPad::heightFor(r.getWidth())));

  r.removeFromTop(6); row = r.removeFromTop(26); int wt = row.getWidth() / 3;
  nobassBtn.setBounds(row.removeFromLeft(wt).reduced(2, 0));
  strumBtn.setBounds(row.removeFromLeft(wt).reduced(2, 0));
  resBox.setBounds(row.reduced(2, 0));

  r.removeFromTop(6); row = r.removeFromTop(22); placeCtl(cNost, row.removeFromLeft(row.getWidth() / 2)); placeCtl(cAir, row);
  r.removeFromTop(4); row = r.removeFromTop(22); placeCtl(cTear, row.removeFromLeft(row.getWidth() / 2)); placeCtl(cHuman, row);

  r.removeFromTop(6); row = r.removeFromTop(30); int bw = row.getWidth() / 3;
  genBtn.setBounds(row.removeFromLeft(bw).reduced(2, 0));
  contBtn.setBounds(row.removeFromLeft(bw).reduced(2, 0));
  preBtn.setBounds(row.reduced(2, 0));

  r.removeFromTop(6); label.setBounds(r.removeFromTop(32));
  r.removeFromTop(6); drag.setBounds(r);
}

void VibeSparkEditor::initCtl(Ctl& c, const char* nm, float v, std::function<void(float)> cb) {
  c.name.setText(nm, juce::dontSendNotification); c.name.setName(nm); c.name.setFont(12.0f);
  c.name.setColour(juce::Label::textColourId, juce::Colour(0xffb08cff));
  c.s.setSliderStyle(juce::Slider::LinearHorizontal);
  c.s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
  c.s.setRange(0.0, 100.0, 1.0); c.s.setTextValueSuffix("%");
  c.s.setValue((double) v * 100.0, juce::dontSendNotification);
  c.s.onValueChange = [&c, cb] { cb((float) (c.s.getValue() / 100.0)); };
  addAndMakeVisible(c.name); addAndMakeVisible(c.s);
}

void VibeSparkEditor::placeCtl(Ctl& c, juce::Rectangle<int> a) {
  a.reduce(2, 0); c.name.setBounds(a.removeFromLeft(58)); c.s.setBounds(a);
}

void VibeSparkEditor::syncFromProc() {
  cNost.s.setValue(proc.nost.load() * 100.0, juce::dontSendNotification);
  cAir.s.setValue(proc.air.load() * 100.0, juce::dontSendNotification);
  cTear.s.setValue(proc.tear.load() * 100.0, juce::dontSendNotification);
  cHuman.s.setValue(proc.human.load() * 100.0, juce::dontSendNotification);
  resBox.setSelectedItemIndex(proc.res.load(), juce::dontSendNotification);
  strumBtn.setToggleState(proc.strum.load(), juce::dontSendNotification);
}

void VibeSparkEditor::timerCallback() {
  label.setText(proc.getNames(), juce::dontSendNotification);
  if (proc.moodDirty.exchange(false)) syncFromProc();
  pad.repaint();
}

juce::AudioProcessorEditor* VibeSparkProcessor::createEditor() { return new VibeSparkEditor(*this); }
