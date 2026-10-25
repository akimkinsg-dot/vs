#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <vector>

namespace vs
{
struct NoteEvt
{
    double beat = 0.0;
    float  ms   = 0.0f;
    juce::uint8 note = 60, vel = 90;
    bool on = true;
};

struct Progression
{
    static constexpr int kMaxEvents = 512;
    std::array<NoteEvt, kMaxEvents> ev {};
    int n = 0;
    double lengthBeats = 16.0;

    void add (double beat, float ms, int note, int vel, bool on)
    {
        if (n >= kMaxEvents) return;
        ev[(size_t) n++] = { beat, ms, (juce::uint8) note, (juce::uint8) vel, on };
    }

    juce::MidiMessageSequence toMidiMessageSequence (double bpb = 4.0) const
    {
        juce::ignoreUnused (bpb);
        juce::MidiMessageSequence seq;
        const double ppqScale = 960.0;
        for (int i = 0; i < n; ++i)
        {
            const auto& e = ev[(size_t) i];
            double timestamp = e.beat * ppqScale + (e.ms * 0.001 * 2.0 * ppqScale);
            juce::MidiMessage m = e.on ? juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel)
                                       : juce::MidiMessage::noteOff (1, e.note);
            seq.addEvent (m, timestamp);
        }
        seq.updateMatchedPairs();
        return seq;
    }

    juce::MidiMessageSequence singleChordToMidiSequence (int chordIndex, double bpb = 4.0) const
    {
        juce::MidiMessageSequence seq;
        const double ppqScale = 960.0;
        const double chordDuration = bpb;
        const double startTime = (double) chordIndex * chordDuration;
        const double endTime = startTime + chordDuration;

        for (int i = 0; i < n; ++i)
        {
            const auto& e = ev[(size_t) i];
            if (e.beat >= startTime && e.beat < endTime)
            {
                double relativeBeat = e.beat - startTime;
                double timestamp = relativeBeat * ppqScale + (e.ms * 0.001 * 2.0 * ppqScale);
                juce::MidiMessage m = e.on ? juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel)
                                           : juce::MidiMessage::noteOff (1, e.note);
                seq.addEvent (m, timestamp);
            }
        }
        seq.updateMatchedPairs();
        return seq;
    }
};

enum Quality { Maj7, Maj9, Add9, Maj7s11, Min7, Min9, Min11, MinAdd9,
               Dom9, Dom7sus, Sus4, Min6, Dom13, Maj13, QualityCount };

struct ChordType { const char* suffix; int n; std::array<int, 6> iv; };

inline const ChordType& chordType (int q)
{
    static const ChordType table[QualityCount] = {
        { "maj7",    4, { 0, 4, 7, 11, 0,  0  } },
        { "maj9",    5, { 0, 4, 7, 11, 14, 0  } },
        { "add9",    4, { 0, 4, 7, 14, 0,  0  } },
        { "maj7#11", 5, { 0, 4, 7, 11, 18, 0  } },
        { "m7",      4, { 0, 3, 7, 10, 0,  0  } },
        { "m9",      5, { 0, 3, 7, 10, 14, 0  } },
        { "m11",     6, { 0, 3, 7, 10, 14, 17 } },
        { "m(add9)", 4, { 0, 3, 7, 14, 0,  0  } },
        { "9",       5, { 0, 4, 7, 10, 14, 0  } },
        { "7sus4",   4, { 0, 5, 7, 10, 0,  0  } },
        { "sus4",    4, { 0, 5, 7, 14, 0,  0  } },
        { "m6",      4, { 0, 3, 7, 9,  0,  0  } },
        { "13",      6, { 0, 4, 7, 10, 14, 21 } },
        { "maj13",   6, { 0, 4, 7, 11, 14, 21 } }
    };
    return table[q];
}

struct Step { int semis; Quality q; };

struct ProgTemplate
{
    const char* name;
    float temp;
    int minChords;
    std::array<Step, 4> s;
};

inline const std::vector<ProgTemplate>& library()
{
    static const std::vector<ProgTemplate> lib = {
        { "2AM Floating",       0.05f, 2, {{ {0,Min9},  {8,Maj9},    {0,Min9},    {8,Maj9}    }} },
        { "Ethereal Drift",     0.12f, 3, {{ {0,Min11}, {8,Maj7s11}, {3,Sus4},    {0,Min11}   }} },
        { "Cinematic Haze",     0.20f, 2, {{ {0,Min11}, {5,Min9},    {0,Min11},   {5,Min9}    }} },
        { "Deep Space Vibe",    0.28f, 3, {{ {0,Min9},  {1,Maj7},    {10,Dom9},   {0,Min9}    }} },
        { "Velvet Drive",       0.38f, 2, {{ {0,Min9},  {5,Min9},    {0,Min9},    {5,Min9}    }} },
        { "Late Night Call",    0.45f, 3, {{ {0,Min9},  {5,Dom9},    {8,Maj9},    {0,Min9}    }} },
        { "Dorian Silk",        0.52f, 4, {{ {0,Min9},  {5,Dom13},   {0,Min11},   {10,Maj9}   }} },
        { "Golden Hour R&B",    0.60f, 3, {{ {0,Min9},  {3,Maj9},    {8,Maj7},    {0,Min9}    }} },
        { "Neo-Soul Elegance",  0.72f, 4, {{ {0,Maj13}, {5,Dom9},    {3,Maj9},    {8,Maj7s11} }} },
        { "Smooth Operator",    0.80f, 4, {{ {0,Maj9},  {9,Min9},    {2,Min11},   {7,Dom13}   }} },
        { "Sunday Butter",      0.88f, 3, {{ {0,Maj9},  {4,Min9},    {9,Min11},   {0,Maj9}    }} },
        { "Jazz-Hop Royalty",   0.95f, 4, {{ {2,Min11}, {7,Dom13},   {0,Maj13},   {9,Min9}    }} }
    };
    return lib;
}

class ChordEngine
{
public:
    ChordEngine() : rng (juce::Time::currentTimeMillis()) {}

    int randomKey() { return rng.nextInt (12); }

    static juce::String noteName (int pc)
    {
        static const char* n[] = { "C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B" };
        return n[((pc % 12) + 12) % 12];
    }

    void generate (float vibeX, float vibeY, int keyPc, double bpm, double beatsPerBar,
                   Progression& out, juce::StringArray& chordNames, juce::String& progName)
    {
        juce::ignoreUnused (bpm);
        vibeX = juce::jlimit (0.0f, 1.0f, vibeX);
        vibeY = juce::jlimit (0.0f, 1.0f, vibeY);
        keyPc = ((keyPc % 12) + 12) % 12;
        const double bpb = juce::jlimit (2.0, 8.0, beatsPerBar);

        out = Progression{};
        out.lengthBeats = 4.0 * bpb;
        chordNames.clear();

        const auto& lib = library();
        std::vector<double> w (lib.size());
        double total = 0.0;
        for (size_t i = 0; i < lib.size(); ++i)
        {
            const double d = (double) vibeX - (double) lib[i].temp;
            w[i] = std::exp (-d * d / (2.0 * 0.20 * 0.20)) + 0.01;
            if ((int) i == lastProgression) w[i] *= 0.05;
            total += w[i];
        }

        double r = rng.nextDouble() * total;
        int idx = (int) lib.size() - 1;
        for (size_t i = 0; i < lib.size(); ++i)
        {
            r -= w[i];
            if (r <= 0.0) { idx = (int) i; break; }
        }
        lastProgression = idx;
        auto steps = lib[(size_t) idx].s;
        const int numChords = lib[(size_t) idx].minChords;

        std::array<int, 4> rootPc {};
        std::array<Voicing, 4> vo;
        for (int i = 0; i < 4; ++i)
        {
            rootPc[(size_t) i] = (keyPc + steps[(size_t) i].semis) % 12;
            if (i < numChords)
                chordNames.add (noteName (rootPc[(size_t) i]) + chordType (steps[(size_t) i].q).suffix);
        }

        const Voicing* prev = nullptr;
        for (int pass = 0; pass < 2; ++pass)
        {
            for (int i = 0; i < 4; ++i)
            {
                vo[(size_t) i] = voice (rootPc[(size_t) i], chordType (steps[(size_t) i].q), prev, vibeY);
                prev = &vo[(size_t) i];
            }
            prev = &vo[3];
        }

        progName = juce::String (lib[(size_t) idx].name) + " - " + noteName (keyPc);

        const double barDuration = (4.0 * bpb) / (double) numChords;
        for (int i = 0; i < numChords; ++i)
        {
            const auto& v = vo[(size_t) i];
            const double chordStart = (double) i * barDuration;
            const double chordEnd = chordStart + barDuration;
            const int totalVoices = 1 + v.nUp;

            for (int j = 0; j < totalVoices; ++j)
            {
                const int note = (j == 0) ? v.bass : v.up[(size_t) j - 1];
                int vel = 78 + (j == totalVoices - 1 ? 10 : 0) + (rng.nextInt (10) - 5);
                if (j == 0) vel -= 6;
                vel = juce::jlimit (1, 127, vel);

                const float strum = (float) j * (2.0f + (1.0f - vibeY) * 3.0f);
                out.add (chordStart, strum, note, vel, true);
                out.add (chordEnd - 0.08, -2.0f, note, 0, false);
            }
        }
    }

private:
    struct Voicing { int bass = 36; int nUp = 0; std::array<int, 6> up {}; };

    static Voicing voice (int rootPc, const ChordType& ct, const Voicing* prev, float spaceY)
    {
        Voicing v;
        const int bassMin = (int) std::lround (juce::jmap (spaceY, 36.0f, 28.0f));
        const int bassMax = bassMin + 12;
        const int ref = prev != nullptr ? prev->bass : bassMin + 5;
        int bestD = INT_MAX;
        for (int n = bassMin; n <= bassMax; ++n)
            if (n % 12 == rootPc && std::abs (n - ref) < bestD) { bestD = std::abs (n - ref); v.bass = n; }

        std::array<int, 6> pcs {};
        int np = 0;
        for (int i = 1; i < ct.n; ++i)
        {
            if (ct.n >= 5 && ct.iv[(size_t) i] == 7) continue;
            pcs[(size_t) np++] = (rootPc + ct.iv[(size_t) i]) % 12;
        }

        const int lo = std::max ((int) std::lround (juce::jmap (spaceY, 56.0f, 48.0f)), v.bass + 7);
        const int hi = (int) std::lround (juce::jmap (spaceY, 70.0f, 85.0f));

        std::array<int, 6> cur {}, best {};
        double bestCost = 1.0e18;

        auto rec = [&] (auto&& self, int i) -> void
        {
            if (i == np)
            {
                double c = 0.0;
                if (prev != nullptr)
                {
                    for (int k = 0; k < np; ++k)
                    {
                        int md = INT_MAX;
                        for (int j = 0; j < prev->nUp; ++j)
                            md = std::min (md, std::abs (cur[(size_t) k] - prev->up[(size_t) j]));
                        c += md;
                    }
                }
                if (c < bestCost) { bestCost = c; best = cur; }
                return;
            }
            for (int n = lo; n <= hi; ++n)
                if (n % 12 == pcs[(size_t) i]) { cur[(size_t) i] = n; self (self, i + 1); }
        };
        rec (rec, 0);

        std::sort (best.begin(), best.begin() + np);
        v.up = best;
        v.nUp = np;
        return v;
    }

    juce::Random rng;
    int lastProgression = -1;
};
} // namespace vs
