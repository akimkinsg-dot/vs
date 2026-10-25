#include "PluginProcessor.h"
#include "PluginEditor.h"

VibeSparkProcessor::VibeSparkProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {
  synth.addSound(new PadSound());
  for (int i = 0; i < 16; ++i) synth.addVoice(new PadVoice());
  generate();
}

void VibeSparkProcessor::prepareToPlay(double sr, int) {
  sampleRate = sr; synth.setCurrentPlaybackSampleRate(sr); active.reserve(64); pending.reserve(64);
}

void VibeSparkProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals nd;
  buffer.clear(); midi.clear();
  const int n = buffer.getNumSamples();
  bool hostPlaying = false, havePpq = false; double ppq = 0, bpm = 120.0;
  if (auto* ph = getPlayHead())
    if (auto pos = ph->getPosition()) {
      hostPlaying = pos->getIsPlaying();
      if (auto p = pos->getPpqPosition()) { ppq = *p; havePpq = true; }
      if (auto b = pos->getBpm()) bpm = *b;
    }
  if (bpm < 20.0) bpm = 120.0;
  bool run = false;
  if (hostPlaying && havePpq) run = true;
  else if (preview.load()) { ppq = internalPpq; bpm = getCurrentBpm(); internalPpq += (double) n / sampleRate * (bpm / 60.0); run = true; }

  if (version.load() != snapVersion) {
    juce::SpinLock::ScopedTryLockType tl(lock);
    if (tl.isLocked()) { snap = state; snapVersion = version.load(); }
  }

  juce::MidiBuffer ev;
  if (run) {
    const double pps = bpm / 60.0 / sampleRate;
    const double p0 = ppq, p1 = p0 + (double) n * pps;
    auto off = [&](double t) { return juce::jlimit(0, n - 1, (int) std::floor((t - p0) / pps)); };
    auto release = [&](double upTo) {
      for (size_t i = 0; i < active.size();) {
        if (active[i].end <= upTo) {
          ev.addEvent(juce::MidiMessage::noteOff(1, active[i].note), off(active[i].end));
          active.erase(active.begin() + (long) i);
        } else ++i;
      }
    };
    auto cutAll = [&](int at) {
      for (auto& a : active) ev.addEvent(juce::MidiMessage::noteOff(1, a.note), at);
      active.clear(); pending.clear();
    };
    auto play = [&](const Hit& h, double t, int at) {
      if (h.chord < 0 || h.chord > 3) return;
      if (!h.topOnly) cutAll(at);
      const auto& c = snap.voiced[h.chord];
      const int sz = (int) c.size();
      if (sz < 2) return;
      const int octOffset = octaveShift.load() * 12;
      const bool skipB = noBass.load();
      const bool doStrum = strum.load();
      const float hm = human.load();
      const double msToPpq = bpm / 60000.0;

      for (int i = 0; i < sz; ++i) {
        const bool isTop = (i == sz - 1);
        if (skipB && i == 0) continue;
        if (h.topOnly && !isTop) continue;
        int note = c[(size_t) i];
        double endT = t + h.len;
        if (isTop && h.sus >= 0) { note = h.sus; endT = t + h.susLen; }
        else if (i > 0 && !isTop && !h.topOnly) endT -= (double) hm * 0.1 * audioRng.nextFloat();
        note += octOffset;
        if (note < 0 || note > 127) continue;

        double ms = h.topOnly ? (double) hm * 4.0 * audioRng.nextFloat()
                              : (doStrum ? 22.0 * i : 0.0) + (double) hm * (6.0 * i + 8.0 * audioRng.nextFloat());
        const int vel = calcVel(i, isTop, note, h.chord, hm, audioRng.nextFloat(), h.topOnly);

        active.push_back(Act{note, endT});
        if (ms < 0.5) ev.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), at);
        else pending.push_back(Pend{t + ms * msToPpq, note, (juce::uint8) vel});
      }
    };

    const bool jump = !wasRunning || std::abs(p0 - expectedPpq) > 0.25;
    const double base = std::floor(p0 / 16.0) * 16.0;
    if (jump) {
      cutAll(0);
      for (const auto& h : snap.hits) {
        const double t = base + h.start;
        if (t < p0 - 1e-9 && t + h.len > p0 + 1e-9) play(h, t, 0);
      }
    }
    const double w0 = jump ? p0 : expectedPpq;
    for (int cyc = 0; cyc < 2; ++cyc)
      for (const auto& h : snap.hits) {
        const double t = base + cyc * 16.0 + h.start;
        if (t >= w0 && t < p1) { release(t); play(h, t, off(t)); }
      }
    for (size_t i = 0; i < pending.size();) {
      if (pending[i].t < p1) {
        ev.addEvent(juce::MidiMessage::noteOn(1, pending[i].note, pending[i].vel), off(pending[i].t));
        pending.erase(pending.begin() + (long) i);
      } else ++i;
    }
    release(p1);
    expectedPpq = p1; wasRunning = true;
  } else if (wasRunning) {
    for (auto& a : active) ev.addEvent(juce::MidiMessage::noteOff(1, a.note), 0);
    active.clear(); pending.clear(); wasRunning = false;
  }
  synth.renderNextBlock(buffer, ev, 0, n);
  midi.swapWith(ev);
}

juce::File VibeSparkProcessor::writeMidiFile() {
  ChordGen copy; { juce::SpinLock::ScopedLockType l(lock); copy = state; }
  juce::MidiMessageSequence seq;
  const int octOffset = octaveShift.load() * 12;
  const bool skipB = noBass.load();
  const bool doStrum = strum.load();
  const float hm = human.load();

  for (const auto& h : copy.hits) {
    if (h.chord < 0 || h.chord > 3) continue;
    const auto& c = copy.voiced[h.chord];
    const int sz = (int) c.size();
    for (int j = 0; j < sz; ++j) {
      const bool isTop = (j == sz - 1);
      if (skipB && j == 0) continue;
      if (h.topOnly && !isTop) continue;
      int note = c[(size_t) j];
      double endBeat = h.start + h.len;
      if (isTop && h.sus >= 0) { note = h.sus; endBeat = h.start + h.susLen; }
      note += octOffset;
      if (note < 0 || note > 127) continue;
      const int tickOffset = (!h.topOnly && doStrum) ? j * 15 : 0;
      const int vel = calcVel(j, isTop, note, h.chord, hm, 0.5f, h.topOnly);
      seq.addEvent(juce::MidiMessage::noteOn(1, note, (juce::uint8) vel), h.start * 480.0 + tickOffset);
      seq.addEvent(juce::MidiMessage::noteOff(1, note), endBeat * 480.0 - 10.0);
    }
  }
  seq.addEvent(juce::MidiMessage::endOfTrack(), 7680.0);
  seq.updateMatchedPairs();
  juce::MidiFile mf; mf.setTicksPerQuarterNote(480); mf.addTrack(seq);
  auto f = juce::File::getSpecialLocation(juce::File::tempDirectory)
               .getChildFile("VibeSpark_Chords_" + juce::String(++fileCounter) + ".mid");
  f.deleteFile();
  juce::FileOutputStream out(f);
  if (out.openedOk()) mf.writeTo(out);
  return f;
}

juce::AudioProcessorEditor* VibeSparkProcessor::createEditor();
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VibeSparkProcessor(); }
