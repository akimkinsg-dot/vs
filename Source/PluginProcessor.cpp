#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout VibeSparkAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vibeX", 1 }, "Vibe X",
           NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "vibeY", 1 }, "Vibe Y",
           NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

    StringArray keys { "Random", "Last MIDI Note", "C", "C#/Db", "D", "D#/Eb", "E", "F",
                       "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keys, 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "loop", 1 }, "Loop", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { "port", 1 }, "Virtual MIDI Port", true));
    return l;
}

VibeSparkAudioProcessor::VibeSparkAudioProcessor()
    : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                      ),
      apvts (*this, nullptr, "VIBESPARK_STATE", createLayout())
{
    vibeXParam = apvts.getRawParameterValue ("vibeX");
    vibeYParam = apvts.getRawParameterValue ("vibeY");
    keyParam   = apvts.getRawParameterValue ("key");
    loopParam  = apvts.getRawParameterValue ("loop");
    portParam  = apvts.getRawParameterValue ("port");

    queue.reserve (2048);
    portSender = std::make_unique<MidiPortSender>();
}

VibeSparkAudioProcessor::~VibeSparkAudioProcessor() = default;

bool VibeSparkAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

bool VibeSparkAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
   #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
   #else
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
   #endif
}

void VibeSparkAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    samplePos = 0;
    queue.clear();
    sounding.fill (false);
    portNotes.fill (false);
    cycleActive = haveCycle = startedInTransport = wasPlaying = false;
    lastTransportPpq = -1.0;
}

void VibeSparkAudioProcessor::requestSpark()
{
    const int keySel = (int) keyParam->load();
    int keyPc = 0;
    if (keySel == 0)       keyPc = engine.randomKey();
    else if (keySel == 1)  { const int p = lastInputPc.load(); keyPc = p >= 0 ? p : engine.randomKey(); }
    else                   keyPc = keySel - 2;

    vs::Progression p;
    juce::StringArray names;
    juce::String name;
    engine.generate (vibeXParam->load(), vibeYParam->load(), keyPc,
                     hostBpm.load(), hostBeatsPerBar.load(), p, names, name);

    {
        const juce::SpinLock::ScopedLockType lock (handoffLock);
        handoff = p;
        handoffReady = true;
    }
    chordNames = names;
    progressionName = name;
}

void VibeSparkAudioProcessor::previewChord (int chordIndex, juce::MidiBuffer& midiOut)
{
    if (!haveActive) return;
    auto seq = active.singleChordToMidiSequence (chordIndex, hostBeatsPerBar.load());
    for (int i = 0; i < seq.getNumEvents(); ++i)
    {
        auto* ev = seq.getEventPointer (i);
        if (ev != nullptr)
        {
            midiOut.addEvent (ev->message, 0);
        }
    }
}

void VibeSparkAudioProcessor::pushEvent (juce::int64 pos, int status, int d1, int d2)
{
    queue.push_back (Pending { pos, seqCounter++, (juce::uint8) status, (juce::uint8) d1, (juce::uint8) d2 });
    queueDirty = true;
}

void VibeSparkAudioProcessor::sortQueue()
{
    if (! queueDirty) return;
    std::sort (queue.begin(), queue.end(), [] (const Pending& a, const Pending& b)
               { return a.pos != b.pos ? a.pos < b.pos : a.seq < b.seq; });
    queueDirty = false;
}

void VibeSparkAudioProcessor::killAfter (juce::int64 start)
{
    sortQueue();
    auto on = sounding;
    size_t keep = 0;
    for (size_t i = 0; i < queue.size(); ++i)
    {
        if (queue[i].pos >= start) continue;
        on[queue[i].d1] = (queue[i].status & 0xf0) == 0x90;
        queue[keep++] = queue[i];
    }
    queue.erase (queue.begin() + (long) keep, queue.end());
    for (int n = 0; n < 128; ++n)
        if (on[(size_t) n]) pushEvent (start, 0x80, n, 0);
}

void VibeSparkAudioProcessor::flushAll (juce::int64 at)
{
    queue.clear();
    for (int n = 0; n < 128; ++n)
        if (sounding[(size_t) n]) pushEvent (at, 0x80, n, 0);
}

void VibeSparkAudioProcessor::beginCycle (juce::int64 blockStart, bool playing, bool havePpq, double ppq,
                                          double lastBarPpq, double bpb, double bpm)
{
    double offsetSamples = 0.0;
    if (playing && havePpq)
    {
        double nextBar = lastBarPpq;
        while (nextBar < ppq - 1.0e-3) nextBar += bpb;
        offsetSamples = (nextBar - ppq) * 60.0 / bpm * currentSampleRate;
        startedInTransport = true;
    }
    else
        startedInTransport = false;

    const juce::int64 start = blockStart + (juce::int64) std::llround (offsetSamples);
    killAfter (start);
    nextCycleStart = start;
    cycleActive = true;
}

void VibeSparkAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    const double sr = currentSampleRate;
    const juce::int64 blockStart = samplePos;
    const juce::int64 blockEnd = blockStart + numSamples;
    const double nowMs = juce::Time::getMillisecondCounterHiRes();
    const double blockMs = 1000.0 * numSamples / sr;
    const bool loopOn = loopParam->load() > 0.5f;

    double bpm = hostBpm.load();
    double bpb = 4.0, ppq = 0.0, lastBar = 0.0;
    bool playing = false, havePpq = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (auto ts = pos->getTimeSignature()) bpb = ts->numerator * 4.0 / ts->denominator;
            if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
            if (auto lb = pos->getPpqPositionOfLastBarStart()) lastBar = *lb;
            else lastBar = std::floor (ppq / bpb) * bpb;
        }
    }
    bpm = juce::jlimit (20.0, 400.0, bpm);
    hostBpm.store (bpm);
    hostBeatsPerBar.store (bpb);
    hostPlaying.store (playing);

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) lastInputPc.store (m.getNoteNumber() % 12);
    }
    midi.clear();

    // Reset when transport starts from 0 or restarts
    bool transportRestarted = false;
    if (playing && havePpq)
    {
        if (!wasPlaying || ppq < lastTransportPpq || ppq < 0.1)
            transportRestarted = true;
        lastTransportPpq = ppq;
    }

    const bool risingEdge = !wasPlaying && playing;
    if (wasPlaying && !playing && startedInTransport)
    {
        flushAll (blockStart);
        cycleActive = haveCycle = startedInTransport = false;
    }
    wasPlaying = playing;

    bool newProgression = false;
    {
        const juce::SpinLock::ScopedTryLockType lock (handoffLock);
        if (lock.isLocked() && handoffReady)
        {
            active = handoff;
            handoffReady = false;
            haveActive = true;
            newProgression = true;
        }
    }

    if (newProgression || risingEdge || transportRestarted)
    {
        if (haveActive)
            beginCycle (blockStart, playing, havePpq, ppq, lastBar, bpb, bpm);
    }

    const double spb = 60.0 / bpm * sr;
    const juce::int64 cycleLen = (juce::int64) std::llround (active.lengthBeats * spb);
    while (cycleActive && nextCycleStart < blockEnd)
    {
        for (int i = 0; i < active.n; ++i)
        {
            const auto& e = active.ev[(size_t) i];
            const juce::int64 pos = nextCycleStart
                                  + (juce::int64) std::llround (e.beat * spb + e.ms * 0.001 * sr);
            pushEvent (pos, e.on ? 0x90 : 0x80, e.note, e.on ? e.vel : 0);
        }
        lastCycleStart = nextCycleStart;
        lastCycleLen = std::max<juce::int64> (1, cycleLen);
        haveCycle = true;
        nextCycleStart += cycleLen;
        if (! loopOn) cycleActive = false;
    }

    sortQueue();
    const bool portOn = portParam->load() > 0.5f && portSender->isActive();
    size_t keep = 0;
    for (size_t i = 0; i < queue.size(); ++i)
    {
        const Pending e = queue[i];
        if (e.pos >= blockEnd) { queue[keep++] = e; continue; }

        const int off = (int) juce::jlimit<juce::int64> (0, numSamples - 1, e.pos - blockStart);
        midi.addEvent (juce::MidiMessage (e.status, e.d1, e.d2), off);

        const bool isOn = (e.status & 0xf0) == 0x90;
        sounding[e.d1] = isOn;
        if (portOn)
        {
            portSender->push (nowMs + 1000.0 * off / sr + blockMs, e.status, e.d1, e.d2);
            portNotes[e.d1] = isOn;
        }
    }
    queue.erase (queue.begin() + (long) keep, queue.end());

    if (! portOn && portSender->isActive())
        for (int n = 0; n < 128; ++n)
            if (portNotes[(size_t) n])
            {
                portSender->push (nowMs + blockMs, 0x80, (juce::uint8) n, 0);
                portNotes[(size_t) n] = false;
            }

    if (playing && havePpq) uiBeat = std::max (0.0, ppq - lastBar);
    else                    uiBeat = std::fmod (uiBeat + numSamples / spb, bpb);
    beatInBar.store ((float) uiBeat);

    if (haveCycle)
    {
        const juce::int64 ref = blockStart + numSamples / 2;
        const juce::int64 rel = ref - lastCycleStart;
        if (rel >= lastCycleLen)  currentBar.store (-1);
        else if (rel >= 0)        currentBar.store (juce::jlimit (0, 3, (int) ((double) rel / ((double) lastCycleLen / 4.0))));
    }
    else
        currentBar.store (-1);

    samplePos = blockEnd;
}

juce::AudioProcessorEditor* VibeSparkAudioProcessor::createEditor()
{
    return new VibeSparkAudioProcessorEditor (*this);
}

void VibeSparkAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void VibeSparkAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibeSparkAudioProcessor();
}
