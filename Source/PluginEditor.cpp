#include "PluginEditor.h"

VibeSparkAudioProcessorEditor::VibeSparkAudioProcessorEditor (VibeSparkAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (620, 470);

    vibeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    vibeSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    vibeSlider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                    juce::MathConstants<float>::pi * 2.75f, true);
    vibeSlider.setDoubleClickReturnValue (true, 0.5);
    addAndMakeVisible (vibeSlider);

    spaceSlider.setSliderStyle (juce::Slider::LinearVertical);
    spaceSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    spaceSlider.setDoubleClickReturnValue (true, 0.5);
    addAndMakeVisible (spaceSlider);

    addAndMakeVisible (sparkButton);
    sparkButton.onClick = [this]
    {
        proc.requestSpark();
        chordStrip.setChords (proc.chordNames, proc.progressionName);
        sparkButton.flash();
    };

    addAndMakeVisible (chordStrip);
    chordStrip.setChords (proc.chordNames, proc.progressionName);
    chordStrip.onStartDrag = [this] { exportAndDragMidi(); };

    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("key")))
        keyBox.addItemList (c->choices, 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (loopButton);
    addAndMakeVisible (portButton);

    vibeAtt  = std::make_unique<SA> (proc.apvts, "vibe", vibeSlider);
    spaceAtt = std::make_unique<SA> (proc.apvts, "space", spaceSlider);
    keyAtt   = std::make_unique<CA> (proc.apvts, "key", keyBox);
    loopAtt  = std::make_unique<BA> (proc.apvts, "loop", loopButton);
    portAtt  = std::make_unique<BA> (proc.apvts, "port", portButton);

    startTimerHz (30);
}

VibeSparkAudioProcessorEditor::~VibeSparkAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VibeSparkAudioProcessorEditor::exportAndDragMidi()
{
    auto prog = proc.getCurrentProgression();
    if (prog.n == 0) return;

    juce::MidiFile file;
    file.setTicksPerQuarterNote (960);
    file.addTrack (prog.toMidiMessageSequence (proc.hostBeatsPerBar.load()));

    juce::File tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory);
    juce::File tempMidi = tempDir.getChildFile ("VibeSpark_Chords.mid");
    tempMidi.deleteFile();

    juce::FileOutputStream stream (tempMidi);
    if (stream.openedOk())
    {
        file.writeTo (stream);
        stream.flush();
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ tempMidi.getFullPathName() }, false, this);
    }
}

void VibeSparkAudioProcessorEditor::timerCallback()
{
    const double newBpm = proc.hostBpm.load();
    const bool newPlaying = proc.hostPlaying.load();
    const float newBeat = proc.beatInBar.load();
    const bool changed = std::abs (newBpm - bpm) > 0.01 || newPlaying != playing
                      || (int) newBeat != (int) beat;
    bpm = newBpm; playing = newPlaying; beat = newBeat;

    accent = laf.vibeColour ((float) vibeSlider.getValue());
    sparkButton.setAccent (accent);
    chordStrip.setAccent (accent);
    chordStrip.setActive (proc.currentBar.load());
    sparkButton.tick();
    if (changed) repaint (bpmArea);
}

void VibeSparkAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (28);
    auto header = r.removeFromTop (56);
    bpmArea = header.removeFromRight (210);
    titleArea = header;
    r.removeFromTop (8);

    auto mid = r.removeFromTop (230);
    knobArea  = mid.removeFromLeft (210);
    spaceArea = mid.removeFromRight (90);
    sparkArea = mid;

    r.removeFromTop (10);
    chordStrip.setBounds (r.removeFromTop (92));
    r.removeFromTop (14);
    footerArea = r;

    vibeSlider.setBounds (knobArea.withTrimmedBottom (26));
    spaceSlider.setBounds (spaceArea.withTrimmedTop (34).withTrimmedBottom (34).withSizeKeepingCentre (60, 160));
    sparkButton.setBounds (sparkArea.withTrimmedBottom (20).withSizeKeepingCentre (200, 200));

    auto f = footerArea.withSizeKeepingCentre (footerArea.getWidth(), 30);
    keyBox.setBounds (f.removeFromLeft (160));
    f.removeFromLeft (20);
    loopButton.setBounds (f.removeFromLeft (90));
    portButton.setBounds (f.removeFromLeft (170));
}

void VibeSparkAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);
    juce::ColourGradient wash (ui::cold.withAlpha (0.07f), 0.0f, 0.0f,
                               ui::warm.withAlpha (0.05f), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill (wash);
    g.fillAll();

    auto t = titleArea;
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold).withKerningFactor (0.28f)));
    g.drawText ("VIBESPARK", t.removeFromTop (34), juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (ui::font (10.5f));
    g.drawText ("MIDI CHORD ENGINE - R&B / TRAPSOUL", t, juce::Justification::centredLeft);

    auto b = bpmArea.toFloat();
    g.setColour (ui::panel);  g.fillRoundedRectangle (b, 12.0f);
    g.setColour (ui::line);   g.drawRoundedRectangle (b.reduced (0.5f), 12.0f, 1.0f);
    auto inner = b.reduced (14.0f, 8.0f);

    auto left = inner.removeFromLeft (100.0f);
    g.setColour (ui::dim);  g.setFont (ui::font (9.5f));
    g.drawText ("HOST BPM", left.removeFromTop (14.0f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (ui::text); g.setFont (ui::font (26.0f));
    g.drawText (juce::String (bpm, 1), left.toNearestInt(), juce::Justification::centredLeft);

    auto top = inner.removeFromTop (inner.getHeight() * 0.5f);
    auto led = top.removeFromLeft (14.0f).withSizeKeepingCentre (8.0f, 8.0f);
    g.setColour (playing ? ui::live : ui::dim);
    g.fillEllipse (led);
    g.setFont (ui::font (10.0f));
    g.drawText (playing ? "SYNC" : "FREE", top.toNearestInt(), juce::Justification::centredLeft);

    const int idx = (int) beat;
    auto dots = inner.withSizeKeepingCentre (4.0f * 16.0f, 10.0f).withX (inner.getX());
    for (int i = 0; i < 4; ++i)
    {
        auto d = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ dots.getX() + 6.0f + (float) i * 16.0f, dots.getCentreY() });
        g.setColour (i == idx ? accent : ui::line);
        g.fillEllipse (d);
    }

    auto cap = knobArea.withTrimmedTop (knobArea.getHeight() - 24);
    g.setFont (ui::font (10.5f));
    g.setColour (ui::cold); g.drawText ("COLD", cap, juce::Justification::centredLeft);
    g.setColour (ui::warm); g.drawText ("WARM", cap, juce::Justification::centredRight);
    g.setColour (ui::dim);  g.drawText ("VIBE", cap, juce::Justification::centred);

    g.setColour (ui::dim);
    g.drawText ("SPACE", spaceArea.withHeight (20), juce::Justification::centred);
    g.setFont (ui::font (9.5f));
    g.drawText ("WIDE",  spaceArea.withTrimmedTop (22).withHeight (14), juce::Justification::centred);
    g.drawText ("TIGHT", spaceArea.withTrimmedTop (spaceArea.getHeight() - 16), juce::Justification::centred);

    g.setFont (ui::font (10.0f, false));
    g.drawText (playing ? "lands on the next bar" : "transport stopped: plays instantly",
                sparkArea.withTrimmedTop (sparkArea.getHeight() - 18), juce::Justification::centred);
}
