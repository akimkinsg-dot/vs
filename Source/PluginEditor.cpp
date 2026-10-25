#include "PluginEditor.h"

VibeSparkAudioProcessorEditor::VibeSparkAudioProcessorEditor (VibeSparkAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (640, 480);

    addAndMakeVisible (vibePad);
    vibePad.setPosition (*proc.apvts.getRawParameterValue ("vibeX"),
                         *proc.apvts.getRawParameterValue ("vibeY"));
    vibePad.setLayerState (proc.currentLayer.load(), proc.activeCorner.load());

    vibePad.onPadChanged = [this] (float x, float y)
    {
        if (auto* px = proc.apvts.getParameter ("vibeX")) px->setValueNotifyingHost (x);
        if (auto* py = proc.apvts.getParameter ("vibeY")) py->setValueNotifyingHost (y);
    };

    vibePad.onPadDragEnded = [this]
    {
        float x = *proc.apvts.getRawParameterValue ("vibeX");
        float y = *proc.apvts.getRawParameterValue ("vibeY");
        handlePadRelease (x, y);
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

void VibeSparkAudioProcessorEditor::handlePadRelease (float x, float y)
{
    int currentL = proc.currentLayer.load();
    int activeC = proc.activeCorner.load();

    if (currentL == 0)
    {
        // Check if pulled into one of the 4 corners (threshold > 0.85 or < 0.15)
        bool isTL = (x < 0.2f && y > 0.8f);
        bool isTR = (x > 0.8f && y > 0.8f);
        bool isBL = (x < 0.2f && y < 0.2f);
        bool isBR = (x > 0.8f && y < 0.2f);

        if (isTL || isTR || isBL || isBR)
        {
            int corner = isTL ? 0 : (isTR ? 1 : (isBL ? 2 : 3));
            proc.currentLayer.store (1);
            proc.activeCorner.store (corner);
            // Snap dot back to centre for the sub-layer exploration
            if (auto* px = proc.apvts.getParameter ("vibeX")) px->setValueNotifyingHost (0.5f);
            if (auto* py = proc.apvts.getParameter ("vibeY")) py->setValueNotifyingHost (0.5f);
        }
    }
    else
    {
        // In sub-layer: check if pulled towards the OPPOSITE direction to zoom out
        // Corner mapping: 0=TL(X<0.2,Y>0.8 -> opposite is BR), 1=TR(X>0.8,Y>0.8 -> opposite is BL), etc.
        bool zoomOut = false;
        if (activeC == 0 && x > 0.8f && y < 0.2f) zoomOut = true; // TL -> pull BR
        else if (activeC == 1 && x < 0.2f && y < 0.2f) zoomOut = true; // TR -> pull BL
        else if (activeC == 2 && x > 0.8f && y > 0.8f) zoomOut = true; // BL -> pull TR
        else if (activeC == 3 && x < 0.2f && y > 0.8f) zoomOut = true; // BR -> pull TL

        if (zoomOut)
        {
            proc.currentLayer.store (0);
            proc.activeCorner.store (-1);
            if (auto* px = proc.apvts.getParameter ("vibeX")) px->setValueNotifyingHost (0.5f);
            if (auto* py = proc.apvts.getParameter ("vibeY")) py->setValueNotifyingHost (0.5f);
        }
    }

    vibePad.setLayerState (proc.currentLayer.load(), proc.activeCorner.load());
    vibePad.setPosition (*proc.apvts.getRawParameterValue ("vibeX"),
                         *proc.apvts.getRawParameterValue ("vibeY"));

    proc.requestSpark();
    chordStrip.setChords (proc.chordNames, proc.progressionName);
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
    g.drawText ("MULTI-LAYER PSYCHOLOGICAL VIBE ENGINE", t, juce::Justification::centredLeft);

    auto b = bpmArea.toFloat();
    g.setColour (ui::panel);  g.fillRoundedRectangle (b, 10.0f);
    g.setColour (ui::line);   g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);
    auto inner = b.reduced (12.0f, 6.0f);

    auto left = inner.removeFromLeft (95.0f);
    g.setColour (ui::dim);  g.setFont (ui::font (9.0f));
    g.drawText ("HOST BPM", left.removeFromTop (12.0f).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (ui::text); g.setFont (ui::font (24.0f));
    g.drawText (juce::String (bpm, 1), left.toNearestInt(), juce::Justification::centredLeft);

    auto top = inner.removeFromTop (inner.getHeight() * 0.5f);
    auto led = top.removeFromLeft (12.0f).withSizeKeepingCentre (8.0f, 8.0f);
    g.setColour (playing ? ui::live : ui::dim);
    g.fillEllipse (led);
    g.setFont (ui::font (9.5f));
    g.drawText (playing ? "SYNC" : "FREE", top.toNearestInt(), juce::Justification::centredLeft);

    const int idx = (int) beat;
    auto dots = inner.withSizeKeepingCentre (4.0f * 14.0f, 8.0f).withX (inner.getX());
    for (int i = 0; i < 4; ++i)
    {
        auto d = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ dots.getX() + 5.0f + (float) i * 14.0f, dots.getCentreY() });
        g.setColour (i == idx ? accent : ui::line);
        g.fillEllipse (d);
    }
}
