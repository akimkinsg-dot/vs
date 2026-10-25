#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <vector>

namespace vs
{
//==============================================================================
// Output container (plain data, no heap: safe to copy on the audio thread)
struct NoteEvt
{
    double beat = 0.0;          // position in beats from start of the 4-bar cycle
    float  ms   = 0.0f;         // micro-timing offset (humanization / strum)
    juce::uint8 note = 60, vel = 90;
    bool on = true;
};

struct Progression
{
    static constexpr int kMaxEvents = 384;
    std::array<NoteEvt, kMaxEvents> ev {};
    int n = 0;
    double lengthBeats = 16.0;

    void add (double beat, float ms, int note, int vel, bool on)
    {
        if (n >= kMaxEvents) return;
        ev[(size_t) n++] = { beat, ms, (juce::uint8) note, (juce::uint8) vel, on };
    }
};

//==============================================================================
// Harmonic vocabulary
enum Quality { Maj7, Maj9, Add9, Maj7s11, Min7, Min9, Min11, MinAdd9,
               Dom9, Dom7sus, Sus4, Min6, QualityCount };

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
        { "sus4",    4, { 0, 5, 7, 14, 0,  0  } },   // sus4 with a soft 9th on top
        { "m6",      4, { 0, 3, 7, 9,  0,  0  } },
    };
    return table[q];
}

struct Step { int semis; Quality q; };   // semitones above the tonic

struct ProgTemplate
{
    const char* name;
    float temp;                 // 0 = cold / 2AM minor ... 1 = warm / soulful major
    std::array<Step, 4> s;
};

// Curated library. Cold/mid sets are relative to a MINOR tonic, warm sets to a MAJOR tonic.
inline const std::vector<ProgTemplate>& library()
{
    static const std::vector<ProgTemplate> lib = {
        // ---- cold: moody minor, sus, phrygian touches --------------------------
        { "2AM Drive",          0.08f, {{ {0,Min9},  {8,Maj7},   {3,Add9},    {10,Sus4}   }} },
        { "Empty Parking Lot",  0.15f, {{ {0,Min7},  {5,Min9},   {8,Maj9},    {7,Dom7sus} }} },
        { "Phrygian Haze",      0.05f, {{ {0,Min9},  {1,Maj7},   {10,Dom9},   {0,Min11}   }} },
        { "Rain On Glass",      0.12f, {{ {0,Min11}, {10,Maj9},  {8,Maj7s11}, {7,Dom7sus} }} },
        { "Late Text",          0.20f, {{ {0,Min7},  {8,Maj9},   {10,Sus4},   {3,Maj7}    }} },
        { "Velvet Dark",        0.22f, {{ {0,Min9},  {3,Maj7},   {5,Min9},    {7,Min7}    }} },
        { "Midnight Loop",      0.30f, {{ {0,Min9},  {5,Dom9},   {3,Maj7},    {10,Maj9}   }} },
        // ---- mid: dorian / modal interchange ------------------------------------
        { "Golden Hour Minor",  0.40f, {{ {0,Min9},  {5,Dom9},   {8,Maj9},    {10,Add9}   }} },
        { "Moody Lift",         0.45f, {{ {0,Min7},  {8,Maj7},   {3,Maj9},    {10,Dom9}   }} },
        { "Dorian Glow",        0.48f, {{ {0,Min9},  {5,Dom9},   {0,Min11},   {10,Maj9}   }} },
        { "Borrowed Sunlight",  0.52f, {{ {0,Min9},  {3,Maj9},   {8,Maj7},    {5,Min6}    }} },
        // ---- warm: 90s / 2000s soul ---------------------------------------------
        { "Peach Haze",         0.62f, {{ {0,Maj9},  {5,Dom9},   {3,Maj7},    {8,Maj7}    }} },
        { "Borrowed Warmth",    0.72f, {{ {0,Maj9},  {3,Maj7},   {5,Maj7},    {5,Min6}    }} },
        { "Silk Road",          0.78f, {{ {0,Maj9},  {9,Min9},   {5,Maj7s11}, {7,Dom7sus} }} },
        { "Smooth Operator",    0.80f, {{ {0,Maj7},  {5,Maj9},   {4,Min7},    {9,Min9}    }} },
        { "Sunday Soul",        0.85f, {{ {0,Maj9},  {4,Min7},   {9,Min9},    {2,Min9}    }} },
        { "Two-Five Heaven",    0.90f, {{ {2,Min9},  {7,Dom9},   {0,Maj9},    {9,Min9}    }} },
        { "Velour Sunday",      0.95f, {{ {5,Maj9},  {4,Min7},   {2,Min9},    {0,Maj9}    }} },
        { "Late 90s Slow Jam",  1.00f, {{ {0,Maj7},  {9,Min7},   {2,Min9},    {7,Dom9}    }} },
    };
    return lib;
}

// Weighted quality substitutions (the "Markov-ish" variation layer)
struct Sub { Quality from, to; int w; };
inline const std::vector<Sub>& substitutions()
{
    static const std::vector<Sub> subs = {
        { Maj7, Maj9, 3 }, { Maj7, Maj7s11, 1 }, { Maj9, Maj7, 1 }, { Maj9, Maj7s11, 1 },
        { Add9, Maj9, 2 }, { Add9, Maj7, 1 },
        { Min7, Min9, 3 }, { Min7, Min11, 1 }, { Min9, Min11, 2 }, { Min9, Min7, 1 }, { Min11, Min9, 1 },
        { Dom9, Dom7sus, 1 }, { Dom7sus, Dom9, 2 }, { Sus4, Dom7sus, 1 }, { Min6, Min9, 1 },
    };
    return subs;
}

//==============================================================================
class ChordEngine
{
public:
    // --- tweakable "feel" constants -------------------------------------------
    static constexpr float kPushProbability = 0.35f;  // 8th-note anticipation for chords 2-4
    static constexpr float kMutateProbability = 0.40f;
    static constexpr int   kBaseVelocity = 82;

    ChordEngine() : rng (juce::Time::currentTimeMillis()) {}

    int randomKey() { return rng.nextInt (12); }

    static juce::String noteName (int pc)
    {
        static const char* n[] = { "C","Db","D","Eb","E","F","F#","G","Ab","A","Bb","B" };
        return n[((pc % 12) + 12) % 12];
    }

    // Message thread only (allocates). The result is handed to the audio thread as plain data.
    void generate (float vibe, float space, int keyPc, double bpm, double beatsPerBar,
                   Progression& out, juce::StringArray& chordNames, juce::String& progName)
    {
        vibe  = juce::jlimit (0.0f, 1.0f, vibe);
        space = juce::jlimit (0.0f, 1.0f, space);
        keyPc = ((keyPc % 12) + 12) % 12;
        const double bpb = juce::jlimit (2.0, 8.0, beatsPerBar);

        out = Progression{};
        out.lengthBeats = 4.0 * bpb;
        chordNames.clear();

        // ---- 1. choose a progression by temperature affinity ------------------
        const auto& lib = library();
        std::vector<double> w (lib.size());
        double total = 0.0;
        for (size_t i = 0; i < lib.size(); ++i)
        {
            const double d = (double) vibe - (double) lib[i].temp;
            w[i] = std::exp (-d * d / (2.0 * 0.16 * 0.16)) + 0.01;
            if ((int) i == lastProgression) w[i] *= 0.1;
            total += w[i];
        }
        double r = rng.nextDouble() * total;
        int idx = (int) lib.size() - 1;
        for (size_t i = 0; i < lib.size(); ++i) { r -= w[i]; if (r <= 0.0) { idx = (int) i; break; } }
        lastProgression = idx;

        auto steps = lib[(size_t) idx].s;

        // ---- 2. weighted quality substitution on one of chords 2-4 -------------
        if (rng.nextFloat() < kMutateProbability)
        {
            const int k = 1 + rng.nextInt (3);
            int sum = 0;
            for (auto& s : substitutions()) if (s.from == steps[(size_t) k].q) sum += s.w;
            if (sum > 0)
            {
                int pick = rng.nextInt (sum);
                for (auto& s : substitutions())
                {
                    if (s.from != steps[(size_t) k].q) continue;
                    if (pick < s.w) { steps[(size_t) k].q = s.to; break; }
                    pick -= s.w;
                }
            }
        }

        // ---- 3. voice leading (two passes so chord 4 flows back into chord 1) ---
        std::array<int, 4> rootPc {};
        std::array<Voicing, 4> vo;
        for (size_t i = 0; i < 4; ++i)
        {
            rootPc[i] = (keyPc + steps[i].semis) % 12;
            chordNames.add (noteName (rootPc[i]) + chordType (steps[i].q).suffix);
        }
        const Voicing* prev = nullptr;
        for (int pass = 0; pass < 2; ++pass)
        {
            for (size_t i = 0; i < 4; ++i)
            {
                vo[i] = voice (rootPc[i], chordType (steps[i].q), prev, space);
                prev = &vo[i];
            }
            prev = &vo[3];
        }

        progName = juce::String (lib[(size_t) idx].name) + "  -  " + noteName (keyPc)
                   + (lib[(size_t) idx].temp < 0.6f ? " minor" : " major");

        // ---- 4. rhythm (adapts to host BPM) ---------------------------------------
        std::array<double, 3> pat { 0.0, 0.0, 0.0 };
        int nHits;
        if      (bpm <  85.0) { pat = { 0.0, 0.0, 0.0 }; nHits = 1; }
        else if (bpm < 118.0) { pat = { 0.0, 2.5, 0.0 }; nHits = 2; }
        else                  { pat = { 0.0, 1.5, 3.0 }; nHits = 3; }

        std::array<double, 3> hp {};
        int usable = 0;
        for (int k = 0; k < nHits; ++k)
            if (k == 0 || pat[(size_t) k] <= bpb - 1.0) hp[(size_t) usable++] = pat[(size_t) k];

        std::array<double, 4> startBeat {};
        for (size_t i = 0; i < 4; ++i)
        {
            startBeat[i] = (double) i * bpb;
            if (i > 0 && rng.nextFloat() < kPushProbability) startBeat[i] -= 0.5;
        }

        // ---- 5. events with humanization -------------------------------------------
        for (size_t i = 0; i < 4; ++i)
        {
            const auto& v = vo[i];
            const double barStart = (double) i * bpb;
            const double chordEnd = (i < 3) ? startBeat[i + 1] : 4.0 * bpb;
            const int total = 1 + v.nUp;

            for (int k = 0; k < usable; ++k)
            {
                const double hs = (k == 0) ? startBeat[i] : barStart + hp[(size_t) k];
                const double he = (k + 1 < usable) ? barStart + hp[(size_t) k + 1] : chordEnd;
                const int accent = (k == 0) ? 0 : -12;

                for (int j = 0; j < total; ++j)           // j == 0 is the bass, then upper voices ascending
                {
                    const int note = (j == 0) ? v.bass : v.up[(size_t) j - 1];
                    int vel = kBaseVelocity + accent + (rng.nextInt (17) - 8);
                    if (j == 0)          vel -= 6;
                    if (j == total - 1)  vel += 5;       // let the top note sing
                    vel = juce::jlimit (1, 127, vel);

                    const float strum = (float) j * (2.0f + rng.nextFloat() * 4.5f) + rng.nextFloat() * 3.0f;
                    out.add (hs, strum, note, vel, true);
                    out.add (he, -3.0f, note, 0, false);   // tiny gap so common tones re-articulate cleanly
                }
            }
        }
    }

private:
    struct Voicing { int bass = 36; int nUp = 0; std::array<int, 6> up {}; };

    static Voicing voice (int rootPc, const ChordType& ct, const Voicing* prev, float space)
    {
        Voicing v;

        // --- bass: nearest octave of the root to the previous bass, inside a register window
        const int bassMin = (int) std::lround (juce::jmap (space, 36.0f, 29.0f));
        const int bassMax = bassMin + 14;
        const int ref = prev != nullptr ? prev->bass : bassMin + 7;
        int bestD = INT_MAX;
        for (int n = bassMin; n <= bassMax; ++n)
            if (n % 12 == rootPc && std::abs (n - ref) < bestD) { bestD = std::abs (n - ref); v.bass = n; }

        // --- upper pitch classes (drop the 5th in big chords to keep colour tones)
        std::array<int, 6> pcs {};
        int np = 0;
        for (int i = 1; i < ct.n; ++i)
        {
            if (ct.n >= 5 && ct.iv[(size_t) i] == 7) continue;
            pcs[(size_t) np++] = (rootPc + ct.iv[(size_t) i]) % 12;
        }

        const int lo = std::max ((int) std::lround (juce::jmap (space, 57.0f, 52.0f)), v.bass + 8);
        const int hi = (int) std::lround (juce::jmap (space, 72.0f, 82.0f));
        const int targetSpan = (int) std::lround (juce::jmap (space, 9.0f, 22.0f));

        std::array<int, 6> cur {}, best {};
        double bestCost = 1.0e18;

        auto cost = [&]() -> double
        {
            std::array<int, 6> s = cur;
            std::sort (s.begin(), s.begin() + np);
            double c = 0.0;

            if (prev != nullptr)
            {
                for (int i = 0; i < np; ++i)
                {
                    int md = INT_MAX;
                    bool common = false;
                    for (int j = 0; j < prev->nUp; ++j)
                    {
                        const int d = std::abs (s[(size_t) i] - prev->up[(size_t) j]);
                        md = std::min (md, d);
                        if (d == 0) common = true;
                    }
                    c += md;
                    if (common) c -= 3.0;                       // hold common tones
                }
                c += 0.7 * std::abs (s[(size_t) np - 1] - prev->up[(size_t) prev->nUp - 1]); // smooth top line
            }
            else
            {
                double mean = 0.0;
                for (int i = 0; i < np; ++i) mean += s[(size_t) i];
                mean /= (double) np;
                c += std::abs (mean - 0.5 * (lo + hi)) * 0.5;
            }

            for (int i = 0; i + 1 < np; ++i)                    // avoid harsh clusters
            {
                const int gap = s[(size_t) i + 1] - s[(size_t) i];
                if (gap == 1) c += 10.0; else if (gap == 2) c += 1.5;
            }
            c += std::abs ((s[(size_t) np - 1] - s[0]) - targetSpan) * 0.5;   // Space control
            return c;
        };

        auto rec = [&] (auto&& self, int i) -> void
        {
            if (i == np)
            {
                const double c = cost();
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
