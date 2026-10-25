#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "ChordGen.h"

struct PadSound : juce::SynthesiserSound {
  bool appliesToNote(int) override { return true; }
  bool appliesToChannel(int) override { return true; }
};

struct PadVoice : juce::SynthesiserVoice {
  bool canPlaySound(juce::SynthesiserSound* s) override { return dynamic_cast<PadSound*>(s) != nullptr; }
  void startNote(int note, float vel, juce::SynthesiserSound*, int) override {
    inc = juce::MathConstants<double>::twoPi * juce::MidiMessage::getMidiNoteInHertz(note) / getSampleRate();
    phase = 0; level = vel * 0.1f;
    env.setSampleRate(getSampleRate());
    env.setParameters(juce::ADSR::Parameters{0.25f, 0.7f, 0.85f, 1.4f});
    env.noteOn();
  }
  void stopNote(float, bool allow) override { if (allow) env.noteOff(); else { env.reset(); clearCurrentNote(); } }
  void pitchWheelMoved(int) override {}
  void controllerMoved(int, int) override {}
  void renderNextBlock(juce::AudioBuffer<float>& b, int start, int num) override {
    if (!env.isActive()) return;
    while (--num >= 0) {
      float e = env.getNextSample();
      float s = (float)(std::sin(phase) + 0.35 * std::sin(2 * phase) + 0.15 * std::sin(3 * phase)) * level * e;
      for (int c = 0; c < b.getNumChannels(); ++c) b.addSample(c, start, s);
      phase += inc; ++start;
      if (!env.isActive()) { clearCurrentNote(); break; }
    }
  }
  juce::ADSR env; double phase = 0, inc = 0; float level = 0;
};

class VibeSparkProcessor : public juce::AudioProcessor {
public:
  VibeSparkProcessor();
  ~VibeSparkProcessor() override = default;

  const juce::String getName() const override { return "VibeSpark"; }
  void prepareToPlay(double sr, int) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return true; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 1.5; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String&) override {}
  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor* createEditor() override;

  void getStateInformation(juce::MemoryBlock& d) override {
    juce::MemoryOutputStream o(d, true);
    o.writeInt(key); o.writeInt(open); o.writeFloat(dark); o.writeFloat(tense);
    o.writeInt(octaveShift); o.writeBool(noBass); o.writeBool(strum);
    o.writeFloat(nost.load()); o.writeFloat(air.load()); o.writeFloat(tear.load()); o.writeFloat(human.load());
    o.writeInt(res.load()); o.writeFloat(reg.load());
  }
  void setStateInformation(const void* data, int size) override {
    if (size < 20) return;
    juce::MemoryInputStream in(data, (size_t) size, false);
    key = juce::jlimit(0, 12, in.readInt()); open = juce::jlimit(0, 1, in.readInt());
    dark = juce::jlimit(0.0f, 1.0f, in.readFloat()); tense = juce::jlimit(0.0f, 1.0f, in.readFloat());
    octaveShift = juce::jlimit(-1, 1, in.readInt()); noBass = in.readBool(); strum = in.readBool();
    if (in.getNumBytesRemaining() >= 20) {
      nost = juce::jlimit(0.0f, 1.0f, in.readFloat()); air = juce::jlimit(0.0f, 1.0f, in.readFloat());
      tear = juce::jlimit(0.0f, 1.0f, in.readFloat()); human = juce::jlimit(0.0f, 1.0f, in.readFloat());
      res = juce::jlimit(0, 2, in.readInt());
      if (in.getNumBytesRemaining() >= 4) reg = juce::jlimit(-1.0f, 1.0f, in.readFloat());
    }
    lastKey = key;
    generate();
  }

  double getCurrentBpm() const {
    if (auto* ph = const_cast<VibeSparkProcessor*>(this)->getPlayHead())
      if (auto pos = ph->getPosition())
        if (auto b = pos->getBpm())
          if (*b >= 20.0 && *b <= 300.0)
            return *b;
    return 120.0;
  }

  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override;

  void generate(bool newSeed = true) {
    if (newSeed) seed = uiRng.nextInt();
    juce::Random r(seed);
    juce::SpinLock::ScopedLockType l(lock);
    state.feel = currentFeel();
    state.generate(r, key, dark, tense, open != 0, getCurrentBpm(), newSeed); ++version;
  }
  void padChanged() { generate(false); }
  Feel currentFeel() const {
    Feel f; f.nost = nost.load(); f.res = res.load(); f.air = air.load(); f.tear = tear.load();
    f.human = human.load(); f.reg = reg.load();
    return f;
  }

  void setMood(int corner, float cx, float cy, bool newSeed) {
    if (corner < 0 || corner > 3) return;
    const Corner& c = CORNERS[corner];
    float w[8]; moodWeights(corner, cx, cy, w);
    float nn = 0, aa = 0, tt = 0, hh = 0, rr = 0, st = 0, rw[3] = {0, 0, 0};
    for (int j = 0; j < 8; ++j) {
      const Shade& s = SHADES[corner * 8 + j];
      nn += w[j] * s.f.nost; aa += w[j] * s.f.air; tt += w[j] * s.f.tear;
      hh += w[j] * s.f.human; rr += w[j] * s.f.reg;
      rw[juce::jlimit(0, 2, s.f.res)] += w[j];
      if (s.strum) st += w[j];
    }
    if (newSeed) resDraw = uiRng.nextFloat();
    int rs = RES_HOME; float acc = 0.0f;
    for (int k = 0; k < 3; ++k) { acc += rw[k]; rs = k; if (resDraw <= acc) break; }

    const float fx = juce::jlimit(0.0f, 1.0f, (cx - 70.0f) / 520.0f);
    const float fy = juce::jlimit(0.0f, 1.0f, (375.0f - cy) / 270.0f);
    dark = c.dk0 + fx * (c.dk1 - c.dk0);
    tense = c.tn0 + fy * (c.tn1 - c.tn0);
    nost = nn; air = aa; tear = tt; human = hh; reg = rr; res = rs; strum = st >= 0.5f;
    moodDirty = true;
    generate(newSeed);
  }

  static int calcVel(int i, bool isTop, int note, int chord, float hm, float rnd, bool soft) {
    static const int arc[4] = {0, 3, 7, -3};
    int v = (i == 0) ? 76 : ((isTop ? 52 : 60) + (note % 7));
    v += (int) std::lround(arc[chord] * hm) + (int) std::lround((rnd - 0.5f) * 10.0f * hm);
    if (soft) v -= 6;
    return juce::jlimit(36, 100, v);
  }
  void evolve() {
    juce::SpinLock::ScopedLockType l(lock);
    state.feel = currentFeel();
    state.evolve(uiRng, key, tense, open != 0, getCurrentBpm()); ++version;
  }
  void setKey(int nk) {
    lastKey = key; key = nk;
    if (nk == 12) {
      generate(true);
    } else {
      const int d = (((nk - lastKey) % 12) + 12) % 12;
      juce::SpinLock::ScopedLockType l(lock);
      state.transpose(d, open != 0, uiRng); ++version;
    }
  }
  void revoice() { juce::SpinLock::ScopedLockType l(lock); state.revoice(open != 0, uiRng); ++version; }
  void setPreview(bool on) { if (on) internalPpq = 0; preview = on; }
  juce::String getNames() { juce::SpinLock::ScopedLockType l(lock); return state.names; }

  juce::File writeMidiFile();

  std::atomic<int> key{0}, open{0}, octaveShift{0};
  std::atomic<float> dark{0.6f}, tense{0.4f};
  std::atomic<bool> preview{false}, noBass{false}, strum{false}, moodDirty{false};
  std::atomic<float> nost{0.0f}, air{0.0f}, tear{0.0f}, human{0.5f}, reg{0.0f};
  std::atomic<int> res{RES_HOME};

private:
  struct Act { int note; double end; };
  struct Pend { double t; int note; juce::uint8 vel; };
  std::vector<Pend> pending;
  juce::Synthesiser synth;
  juce::SpinLock lock;
  ChordGen state, snap;
  std::atomic<int> version{0};
  int snapVersion = -1, seed = 1, lastKey = 0;
  float resDraw = 0.5f;
  juce::Random uiRng, audioRng;
  std::vector<Act> active;
  double sampleRate = 44100.0, internalPpq = 0.0, expectedPpq = 0.0;
  bool wasRunning = false;
  int fileCounter = 0;
};
