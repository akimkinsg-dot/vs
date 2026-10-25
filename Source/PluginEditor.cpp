#include "PluginEditor.h"

VibeSparkAudioProcessorEditor::VibeSparkAudioProcessorEditor (VibeSparkAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (640, 480);

    addAndMakeVisible (vibePad);
    vibePad.setPosition (*proc.apvts.getRawParameterValue ("vibeX"),
                         *proc.apvts.getRawParameterValue ("vibeY"));
    
    vibePad.onPadChanged = [this] (float x, float y)
    {
        if (auto* px = proc.apvts.getParameter ("vibeX")) px->setValueNotifyingHost (x);
        if (auto* py = proc.apvts.getParameter ("vibeY")) py->setValueNotifyingHost (y);
    };

    vibePad.onPadDragEnded = [this]
    {
        proc.requestSpark();
        chordStrip.setChords (proc.chordNames, proc.progressionName);
    };

    proc.requestSpark();

    addAndMakeVisible (chordStrip);
    chordStrip.setChords (proc.chordNames, proc.progressionName);
    chordStrip.onStartDragAll = [this] { exportAndDragAllMidi(); };
    chordStrip.onStartDragSingle = [this] (int idx) { exportAndDragSingleMidi (idx); };
    chordStrip.onClickPreviewChord = [this] (int idx) { previewChordClicked (idx); };

    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter ("key")))
        keyBox.addItemList (c->choices, 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (loopButton);
    addAndMakeVisible (portButton);

    keyAtt  = std::make_unique<CA> (proc.apvts, "key", keyBox);
    loopAtt = std::make_unique<BA> (proc.apvts, "loop", loopButton);
    portAtt = std::make_unique<BA> (proc.apvts, "port", portButton);

    startTimerHz (30);
}

VibeSparkAudioProcessorEditor::~VibeSparkAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VibeSparkAudioProcessorEditor::exportAndDragAllMidi()
{
    auto prog = proc.getCurrentProgression();
    if (prog.n == 0) return;

    juce::MidiFile file;
    file.setTicksPerQuarterNote (960);
    file.addTrack (prog.toMidiMessageSequence (proc.hostBeatsPerBar.load()));

    juce::File tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory);
    juce::File tempMidi = tempDir.getChildFile ("VibeSpark_Progression.mid");
    tempMidi.deleteFile();

    juce::FileOutputStream stream (tempMidi);
    if (stream.openedOk())
    {
        file.writeTo (stream);
        stream.flush();
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ tempMidi.getFullPathName() }, false, this);
    }
}

void VibeSparkAudioProcessorEditor::exportAndDragSingleMidi (int index)
{
    auto prog = proc.getCurrentProgression();
    if (prog.n == 0) return;

    juce::MidiFile file;
    file.setTicksPerQuarterNote (960);
    file.addTrack (prog.singleChordToMidiSequence (index, proc.hostBeatsPerBar.load()));

    juce::File tempDir = juce::File::getSpecialLocation (juce::File::tempDirectory);
    juce::File tempMidi = tempDir.getChildFile ("VibeSpark_Chord_" + juce::String (index + 1) + ".mid");
    tempMidi.deleteFile();

    juce::FileOutputStream stream (tempMidi);
    if (stream.openedOk())
    {
        file.writeTo (stream);
        stream.flush();
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ tempMidi.getFullPathName() }, false, this);
    }
}

void VibeSparkAudioProcessorEditor::previewChordClicked (int index)
{
    juce::MidiBuffer dummyMidi;
    proc.previewChord (index, dummyMidi);
}

void VibeSparkAudioProcessorEditor::timerCallback()
{
    const double newBpm = proc.hostBpm.load();
    const bool newPlaying = proc.hostPlaying.load();
    const float newBeat = proc.beatInBar.load();
    const bool changed = std::abs (newBpm - bpm) > 0.01 || newPlaying != playing
                      || (int) newBeat != (int) beat;
    bpm = newBpm; playing = newPlaying; beat = newBeat;

    const float vx = *proc.apvts.getRawParameterValue ("vibeX");
    accent = ui::cold.interpolatedWith (ui::warm, vx);
    chordStrip.setAccent (accent);
    chordStrip.setActive (proc.currentBar.load());
    if (changed) repaint (bpmArea);
}

void VibeSparkAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (24);
    auto header = r.removeFromTop (50);
    bpmArea = header.removeFromRight (200);
    titleArea = header;
    r.removeFromTop (8);

    padArea = r.removeFromTop (230);
    r.removeFromTop (12);
    chordStrip.setBounds (r.removeFromTop (90));
    r.removeFromTop (12);
    footerArea = r;

    vibePad.setBounds (padArea);

    auto f = footerArea.withSizeKeepingCentre (footerArea.getWidth(), 30);
    keyBox.setBounds (f.removeFromLeft (160));
    f.removeFromLeft (20);
    loopButton.setBounds (f.removeFromLeft (90));
    portButton.setBounds (f.removeFromLeft (170));
}

void VibeSparkAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);
    juce::ColourGradient wash (ui::cold.withAlpha (0.06f), 0.0f, 0.0f,
                               ui::warm.withAlpha (0.04f), (float) getWidth(), (float) getHeight(), false);
    g.setGradientFill (wash);
    g.fillAll();

    auto t = titleArea;
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold).withKerningFactor (0.28f)));
    g.drawText ("VIBESPARK", t.removeFromTop (32), juce::Justification::centredLeft);
    g.setColour (ui::dim);
    g.setFont (ui::font (10.0f));
    g.drawText ("EMPATHETIC PROCEDURAL ENGINE - R&B / TRAPSOUL", t, juce::Justification::centredLeft);

    auto b = bpmArea.toFloat(); // Wait, let's keep clean: b = bpmArea.toFloat();
    // Let's rewrite just the b assignment cleanly below
}
