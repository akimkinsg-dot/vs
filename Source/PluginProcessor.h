#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "ChordEngine.h"
#include <array>
#include <atomic>
#include <memory>
#include <vector>

class MidiPortSender : private juce::Thread
{
public:
    MidiPortSender() : juce::Thread ("VibeSpark MIDI port")
    {
       #if JUCE_MAC || JUCE_LINUX
        out = juce::MidiOutput::createNewDevice ("VibeSpark");
       #endif
        if (out != nullptr) startThread (juce::Thread::Priority::high);
    }

    ~MidiPortSender() override
    {
        stopThread (500);
        if (out != nullptr) out->sendMessageNow (juce::MidiMessage::allNotesOff (1));
    }

    bool isActive() const noexcept { return out != nullptr; }

    void push (double targetMs, juce::uint8 s, juce::uint8 d1, juce::uint8 d2) noexcept
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 + n2 == 0) return;
        items[(size_t) (n1 > 0 ? s1 : s2)] = { targetMs, { s, d1, d2 } };
        fifo.finishedWrite (1);
    }

private:
    struct Item { double ms; std::array<juce::uint8, 3> b; };

    void run() override
    {
        while (! threadShouldExit())
        {
            int s1, n1, s2, n2;
            fifo.prepareToRead (1, s1, n1, s2, n2);
            if (n1 + n2 == 0) { juce::Thread::sleep (1); continue; }

            const Item it = items[(size_t) (n1 > 0 ? s1 : s2)];
            const double wait = it.ms - juce::Time::getMillisecondCounterHiRes();
            if (wait > 1.5) { juce::Thread::sleep (1); continue; }
            while (juce::Time::getMillisecondCounterHiRes() < it.ms && ! threadShouldExit())
                juce::Thread::yield();

            out->sendMessageNow (juce::MidiMessage (it.b[0], it.b[1], it.b[2]));
            fifo.finishedRead (1);
        }
    }

    std::unique_ptr<juce::MidiOutput> out;
    juce::AbstractFifo fifo { 1024 };
    std::array<Item, 1024> items {};
};

class VibeSparkAudioProcessor : public juce::AudioProcessor
{
public:
    VibeSparkAudioProcessor();
    ~VibeSparkAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void requestSpark();
    void previewChord (int chordIndex, juce::MidiBuffer& midiOut);
    vs::Progression getCurrentProgression() const { return active; }

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<double> hostBpm { 120.0 };
    std::atomic<double> hostBeatsPerBar { 4.0 };
    std::atomic<bool>   hostPlaying { false };
    std::atomic<float>  beatInBar { 0.0f };
    std::atomic<int>    currentBar { -1 };

    juce::StringArray chordNames;
    juce::String progressionName;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Pending { juce::int64 pos; juce::uint32 seq; juce::uint8 status, d1, d2; };

    void pushEvent (juce::int64 pos, int status, int d1, int d2);
    void sortQueue();
    void killAfter (juce::int64 start);
    void flushAll (juce::int64 at);
    void beginCycle (juce::int64 blockStart, bool playing, bool havePpq, double ppq,
                     double lastBarPpq, double bpb, double bpm);

    vs::ChordEngine engine;
    std::atomic<int> lastInputPc { -1 };

    juce::SpinLock handoffLock;
    vs::Progression handoff;
    bool handoffReady = false;

    vs::Progression active;
    bool haveActive = false;
    std::vector<Pending> queue;
    bool queueDirty = false;
    juce::uint32 seqCounter = 0;
    std::array<bool, 128> sounding {}, portNotes {};

    double currentSampleRate = 44100.0;
    juce::int64 samplePos = 0;
    bool cycleActive = false, haveCycle = false, startedInTransport = false, wasPlaying = false;
    juce::int64 nextCycleStart = 0, lastCycleStart = 0, lastCycleLen = 1;
    double uiBeat = 0.0;
    double lastTransportPpq = -1.0;

    std::atomic<float>* vibeXParam = nullptr;
    std::atomic<float>* vibeYParam = nullptr;
    std::atomic<float>* keyParam = nullptr;
    std::atomic<float>* loopParam = nullptr;
    std::atomic<float>* portParam = nullptr;

    std::unique_ptr<MidiPortSender> portSender;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibeSparkAudioProcessor)
};
