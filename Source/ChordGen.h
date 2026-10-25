#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>

// ============================================================================
//  CHORD TYPES
// ============================================================================
enum { MAJ7, MAJ9, M7, M9, M11, D9, D13, MAJ7S11, D7B9, SUS9, MMAJ7, SIX9, ADD9, M7B5,
       D7S11, M69, MAJ7SUS2 };
inline const std::vector<int> IV[] = {
  {0,4,7,11},{0,4,7,11,14},{0,3,7,10},{0,3,7,10,14},{0,3,7,10,14,17},
  {0,4,7,10,14},{0,4,10,14,21},{0,4,7,11,18},{0,4,7,10,13},{0,5,7,10,14},
  {0,3,7,11},{0,4,7,9,14},{0,4,7,14},{0,3,6,10},
  {0,4,7,10,18},{0,3,7,9,14},{0,2,7,11}};
inline const char* SUF[] = {"maj7","maj9","m7","m9","m11","9","13","maj7#11","7b9","9sus4","m(maj7)","6/9","add9","m7b5",
                            "7#11","m6/9","maj7sus2"};
inline const char* NOTE_NAMES[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

inline double tensOf(int t) {
  switch (t) {
    case MAJ7: case MAJ9: case ADD9: case SIX9: return 0.0;
    case M7: case M9: case M69: return 0.2;
    case MAJ7SUS2: return 0.25;
    case M11: return 0.3;
    case MAJ7S11: case SUS9: return 0.5;
    case D9: case D7S11: return 0.6;
    case D13: return 0.7;
    case MMAJ7: case M7B5: return 0.8;
    case D7B9: return 1.0;
    default: return 0.3;
  }
}

// ============================================================================
//  PROGRESSION BANK
//  Roots are semitones from the tonic (written in C). dk = darkness, tn = tension.
//  mn = minor-tonic progression (used for borrowed chords and cadences).
// ============================================================================
struct Ch { int r, t, b; };
struct Prog { Ch c[4]; float dk, tn; bool mn; };
inline Ch mk(int r, int t, int b = -1) { return Ch{r, t, b}; }
inline Prog mkP(Ch a, Ch b, Ch c, Ch d, float dk, float tn, bool mn = false) { return Prog{{a, b, c, d}, dk, tn, mn}; }

inline const std::vector<Prog> BANK = {
  // ---------- CANDLELIGHT: warm, safe, close (maj9 / m9, soft slash chords, gentle jazz turnarounds) ----------
  mkP(mk(0,MAJ9),  mk(9,M9),   mk(5,MAJ9),  mk(7,SUS9),            0.12f, 0.10f), // I  vi  IV  Vsus
  mkP(mk(0,MAJ7),  mk(4,M7),   mk(9,M9),    mk(5,MAJ9),            0.10f, 0.12f), // I  iii vi  IV
  mkP(mk(5,MAJ9),  mk(4,M9),   mk(2,M9),    mk(0,MAJ9),            0.12f, 0.08f), // IV iii ii  I  (bass walks down)
  mkP(mk(0,SIX9),  mk(5,MAJ9), mk(4,M7),    mk(2,M9),              0.16f, 0.12f),
  mkP(mk(0,MAJ9),  mk(5,MAJ9,9), mk(2,M9),  mk(7,SUS9),            0.18f, 0.15f),
  mkP(mk(0,MAJ7),  mk(9,M7),   mk(2,M9),    mk(7,D13),             0.20f, 0.20f), // I vi ii V  turnaround
  mkP(mk(2,M9),    mk(7,D13),  mk(0,MAJ9),  mk(5,MAJ9),            0.22f, 0.22f), // ii V I IV
  mkP(mk(0,MAJ9),  mk(10,D9),  mk(5,MAJ9),  mk(0,SIX9),            0.30f, 0.28f), // bVII9 colour
  mkP(mk(4,M9),    mk(9,M9),   mk(2,M9),    mk(7,SUS9),            0.25f, 0.15f),
  mkP(mk(9,M9),    mk(5,MAJ9), mk(0,MAJ9),  mk(7,SUS9),            0.30f, 0.15f),
  mkP(mk(5,MAJ7S11), mk(4,M7), mk(2,M9),    mk(0,MAJ9),            0.22f, 0.25f),
  mkP(mk(0,MAJ9),  mk(6,M7B5), mk(5,MAJ9),  mk(4,M9),              0.26f, 0.30f), // chromatic descent
  mkP(mk(5,MAJ9),  mk(0,MAJ9,4), mk(2,M9),  mk(7,SUS9),            0.08f, 0.08f),
  mkP(mk(0,ADD9),  mk(7,SUS9), mk(9,M9),    mk(5,MAJ7S11),         0.25f, 0.20f),
  mkP(mk(0,MAJ9),  mk(8,MAJ9), mk(5,MAJ9),  mk(7,SUS9),            0.32f, 0.25f), // bVI lift
  mkP(mk(0,MAJ9),  mk(5,M9),   mk(4,M7),    mk(9,M9),              0.33f, 0.32f), // iv bittersweet
  mkP(mk(2,M9),    mk(4,M7),   mk(5,MAJ9),  mk(7,SUS9),            0.18f, 0.22f), // bass climbs D E F G
  mkP(mk(9,M9),    mk(2,M9),   mk(7,D13),   mk(0,MAJ9),            0.28f, 0.30f),
  mkP(mk(0,MAJ9),  mk(9,M9),   mk(5,MAJ9),  mk(5,M9),              0.35f, 0.28f), // IV -> iv
  mkP(mk(0,MAJ9),  mk(5,MAJ9,0), mk(2,M9,0), mk(7,SUS9,0),         0.10f, 0.15f), // tonic pedal (ambient)

  // ---------- ETHEREAL: airy, lydian #11, floating, tritone-sub colour ----------
  mkP(mk(0,MAJ9),  mk(9,M9),   mk(5,MAJ7S11), mk(5,M9),            0.30f, 0.55f),
  mkP(mk(0,SIX9),  mk(8,MAJ7S11), mk(5,MAJ9), mk(7,SUS9),          0.25f, 0.60f),
  mkP(mk(5,MAJ7S11), mk(4,M9), mk(9,M11),   mk(7,SUS9),            0.30f, 0.50f),
  mkP(mk(0,MAJ7S11), mk(2,D7S11), mk(4,M9), mk(5,MAJ7S11),         0.33f, 0.65f), // lydian run
  mkP(mk(0,MAJ9),  mk(3,MAJ9), mk(5,MAJ7S11), mk(7,SUS9),          0.28f, 0.58f),
  mkP(mk(2,M9),    mk(1,D7S11), mk(0,MAJ9), mk(5,MAJ7S11),         0.22f, 0.62f), // ii bII7#11 I (tritone sub)
  mkP(mk(0,MAJ7S11), mk(6,M7B5), mk(5,MAJ9), mk(4,M9),             0.30f, 0.66f),
  mkP(mk(0,MAJ9),  mk(2,D13),  mk(5,MAJ7S11), mk(7,SUS9),          0.25f, 0.64f),
  mkP(mk(2,M9),    mk(7,D13),  mk(0,MAJ9),  mk(10,MAJ7S11),        0.30f, 0.55f),
  mkP(mk(5,MAJ7S11), mk(4,M9), mk(3,MAJ7S11), mk(2,M11),           0.35f, 0.62f), // chromatic bass F E Eb D
  mkP(mk(9,M9),    mk(5,MAJ7S11), mk(0,MAJ9), mk(7,SUS9),          0.30f, 0.50f),
  mkP(mk(0,MAJ7SUS2), mk(2,M11), mk(5,MAJ7SUS2), mk(7,SUS9),       0.15f, 0.48f), // ambient sus2
  mkP(mk(0,MAJ7SUS2), mk(5,MAJ7S11,0), mk(9,M9,0), mk(7,SUS9,0),   0.15f, 0.45f), // ambient pedal
  mkP(mk(0,M9),    mk(8,MAJ7S11), mk(3,MAJ9), mk(10,D13),          0.42f, 0.55f, true),
  mkP(mk(5,MAJ7S11), mk(7,SUS9), mk(4,M9),  mk(9,M9),              0.28f, 0.48f), // royal-road, extended
  mkP(mk(10,MAJ7S11), mk(9,M9), mk(8,MAJ7S11), mk(7,SUS9),         0.38f, 0.72f), // planing, shares D F A
  mkP(mk(0,MAJ7S11), mk(3,MAJ7S11), mk(5,MAJ7S11), mk(7,SUS9),     0.30f, 0.76f),

  // ---------- TOXIC: noir, minor, m(maj7), m7b5, V7b9 (dark but always smooth) ----------
  mkP(mk(0,MMAJ7), mk(8,MAJ7S11), mk(5,M11), mk(7,D7B9),           0.92f, 0.80f, true),
  mkP(mk(0,M9),    mk(2,M7B5), mk(7,D7B9),  mk(0,M11),             0.85f, 0.75f, true),
  mkP(mk(2,M7B5),  mk(7,D7B9), mk(0,M11),   mk(8,MAJ9),            0.80f, 0.75f, true),
  mkP(mk(0,M9),    mk(8,MAJ9), mk(3,MAJ9),  mk(7,D7B9),            0.80f, 0.68f, true),
  mkP(mk(0,MMAJ7), mk(5,M9),   mk(2,M7B5),  mk(7,D7B9),            0.88f, 0.82f, true),
  mkP(mk(0,M11),   mk(10,D13), mk(8,MAJ7S11), mk(7,D7B9),          0.85f, 0.78f, true), // bass C Bb Ab G
  mkP(mk(9,M11),   mk(5,MAJ7S11), mk(2,M9), mk(7,D7B9),            0.70f, 0.66f),
  mkP(mk(1,MAJ9),  mk(0,M11),  mk(8,MAJ7S11), mk(7,D7B9),          0.80f, 0.72f, true),
  mkP(mk(5,M11),   mk(1,D9),   mk(0,MMAJ7), mk(8,MAJ9),            0.85f, 0.82f, true), // bII9 tritone sub -> i
  mkP(mk(0,MMAJ7), mk(0,M7,10), mk(8,MAJ7S11), mk(7,D7B9),         0.90f, 0.78f, true),
  mkP(mk(0,M11),   mk(5,M9),   mk(10,D13),  mk(3,MAJ9),            0.78f, 0.58f, true),
  mkP(mk(0,M9),    mk(0,MMAJ7,11), mk(0,M7,10), mk(0,M69,9),       0.75f, 0.60f, true), // line cliche
  mkP(mk(8,MAJ7S11), mk(7,D7B9), mk(0,MMAJ7), mk(5,M9),            0.88f, 0.85f, true),
  mkP(mk(0,M9),    mk(1,MAJ9), mk(8,MAJ9),  mk(7,D7B9),            0.82f, 0.70f, true),
  mkP(mk(2,M7B5),  mk(1,D9),   mk(0,M11),   mk(5,M9),              0.78f, 0.80f, true),
  mkP(mk(0,M9),    mk(5,M11),  mk(2,M7B5),  mk(7,D7B9),            0.74f, 0.68f, true),
  mkP(mk(0,M11),   mk(3,MAJ7S11), mk(8,MAJ7S11), mk(7,D7B9),       0.72f, 0.62f, true),

  // ---------- 4 AM: lonely, heavy, spacious (m11 / m9, bVI, dorian IV, pedals) ----------
  mkP(mk(0,M11),   mk(8,MAJ9), mk(5,M9),    mk(7,SUS9),            0.80f, 0.35f, true),
  mkP(mk(0,M9),    mk(8,MAJ9), mk(5,M9),    mk(7,D9),              0.75f, 0.40f, true),
  mkP(mk(9,M11),   mk(2,M9),   mk(7,D13),   mk(0,MAJ9),            0.62f, 0.35f),
  mkP(mk(0,M9),    mk(3,MAJ9), mk(8,MAJ9),  mk(7,D9),              0.70f, 0.30f, true),
  mkP(mk(5,M9),    mk(0,M11),  mk(8,MAJ9),  mk(7,D7B9),            0.75f, 0.45f, true),
  mkP(mk(0,M11),   mk(5,M9),   mk(10,D9),   mk(8,MAJ9),            0.70f, 0.40f, true),
  mkP(mk(0,M9),    mk(10,D13), mk(8,MAJ9),  mk(7,D9),              0.68f, 0.46f, true),
  mkP(mk(0,M11),   mk(0,M9,10), mk(8,MAJ9), mk(5,M9),              0.80f, 0.40f, true),
  mkP(mk(8,MAJ9),  mk(7,D9),   mk(0,M9),    mk(5,M11),             0.70f, 0.40f, true),
  mkP(mk(8,MAJ9),  mk(5,M9),   mk(10,D9),   mk(3,MAJ9),            0.62f, 0.30f, true),
  mkP(mk(0,M9),    mk(5,D9),   mk(0,M11),   mk(10,MAJ9),           0.70f, 0.25f, true), // dorian vamp
  mkP(mk(9,M9),    mk(5,MAJ9), mk(4,M9),    mk(7,SUS9),            0.62f, 0.25f),
  mkP(mk(0,M11),   mk(8,MAJ9,0), mk(5,M11,0), mk(7,SUS9,0),        0.75f, 0.12f, true), // tonic pedal
  mkP(mk(5,M9),    mk(3,MAJ9), mk(8,MAJ9),  mk(10,D9),             0.72f, 0.35f, true),
  mkP(mk(0,M9),    mk(5,M9),   mk(0,M11),   mk(8,MAJ9),            0.85f, 0.15f, true), // minimal, heavy
  mkP(mk(2,M9),    mk(4,M7),   mk(9,M11),   mk(7,SUS9),            0.65f, 0.20f)
};

// ============================================================================
//  RHYTHM / FEEL
// ============================================================================
struct Hit { double start = 0, len = 0; int chord = 0; int sus = -1; double susLen = 0; bool topOnly = false; };

enum { RES_HOME, RES_HANG, RES_TWIST };
struct Feel {
  float nost = 0.0f;     // borrowed bittersweet chord (iv / bVI / dorian IV)
  int   res  = RES_HOME; // loop ending: home / hang on sus / twist (deceptive or tritone sub)
  float air  = 0.0f;     // chord arrives early + breath at loop end
  float tear = 0.0f;     // suspension in the top voice
  float human = 0.5f;    // micro-timing, dynamics
  float reg = 0.0f;      // register of the top line: -1 low .. +1 high
};

// ============================================================================
//  MOOD MAP: 4 corners, each opens into 8 shades (coordinates in the 680x440 view space)
// ============================================================================
struct Corner {
  const char* name; const char* tag; const char* desc; juce::uint32 col; float cx, cy;
  const char* axL; const char* axR; const char* axD; const char* axU;
  float dk0, dk1, tn0, tn1;   // where this corner lives on the darkness / tension plane
};
inline const Corner CORNERS[4] = {
  {"Flutter",    "Ethereal",    "attraction, anticipation", 0xff1D9E75, 130, 125, "earth", "weightless", "anticipation", "elation",  0.10f, 0.42f, 0.45f, 0.85f},
  {"Obsession",  "Toxic",       "jealousy, pain, breakdown", 0xffD85A30, 550, 125, "cold",  "heat",       "control",      "breakdown", 0.65f, 0.95f, 0.55f, 0.88f},
  {"Closeness",  "Candlelight", "tenderness, safety",        0xffBA7517, 130, 315, "now",   "memory",     "quiet",        "passionate", 0.05f, 0.38f, 0.05f, 0.40f},
  {"Longing",    "4 AM",        "loneliness, heaviness",     0xff7F77DD, 550, 315, "empty", "heavy",      "numb",         "pain",      0.60f, 0.92f, 0.08f, 0.50f}};

struct Shade { const char* name; int corner; float x, y; Feel f; bool strum; };
//                                                         nost  res        air   tear  human reg
inline const Shade SHADES[32] = {
  // Ethereal
  {"Butterflies",       0, 160, 105, {0.10f, RES_HANG,  0.60f, 0.55f, 0.65f,  0.6f}, true },
  {"Dawn After",        0, 380, 105, {0.10f, RES_HOME,  0.30f, 0.35f, 0.55f,  0.8f}, true },
  {"Dream of You",      0, 570, 105, {0.15f, RES_HANG,  0.55f, 0.40f, 0.60f,  0.5f}, true },
  {"Almost a Kiss",     0, 130, 185, {0.10f, RES_HANG,  0.45f, 0.85f, 0.65f,  0.4f}, true },
  {"Weightless",        0, 480, 185, {0.15f, RES_HANG,  0.40f, 0.40f, 0.50f,  0.6f}, true },
  {"Taxi in the Rain",  0, 260, 265, {0.40f, RES_HANG,  0.50f, 0.70f, 0.60f,  0.2f}, true },
  {"Neon Haze",         0, 520, 265, {0.20f, RES_HANG,  0.35f, 0.50f, 0.55f,  0.5f}, true },
  {"First Glance",      0, 150, 345, {0.05f, RES_HANG,  0.50f, 0.30f, 0.65f,  0.5f}, true },
  // Toxic
  {"Intrusive Thought", 1, 270, 105, {0.10f, RES_HOME,  0.10f, 0.20f, 0.35f, -0.4f}, false},
  {"Breakup Rage",      1, 540, 105, {0.20f, RES_TWIST, 0.20f, 0.30f, 0.25f, -0.1f}, false},
  {"Can't Let Go",      1, 220, 185, {0.50f, RES_TWIST, 0.50f, 0.60f, 0.50f, -0.2f}, false},
  {"Toxic Loop",        1, 480, 185, {0.15f, RES_TWIST, 0.25f, 0.40f, 0.45f, -0.2f}, false},
  {"Gaslighting",       1, 150, 265, {0.20f, RES_TWIST, 0.20f, 0.45f, 0.40f, -0.1f}, false},
  {"Jealous at 3 AM",   1, 400, 265, {0.20f, RES_TWIST, 0.30f, 0.80f, 0.40f, -0.1f}, false},
  {"Cold Shoulder",     1, 130, 345, {0.05f, RES_HOME,  0.10f, 0.20f, 0.30f, -0.3f}, false},
  {"Calm Revenge",      1, 330, 345, {0.30f, RES_TWIST, 0.35f, 0.50f, 0.55f, -0.2f}, false},
  // Candlelight
  {"Vow",               2, 190, 105, {0.15f, RES_HOME,  0.15f, 0.30f, 0.65f,  0.4f}, true },
  {"Sweet Goodbye",     2, 520, 105, {0.65f, RES_HANG,  0.35f, 0.60f, 0.65f,  0.2f}, true },
  {"First Touch",       2, 120, 185, {0.10f, RES_HOME,  0.40f, 0.25f, 0.75f,  0.3f}, true },
  {"Last Summer",       2, 440, 185, {0.65f, RES_HOME,  0.30f, 0.55f, 0.60f,  0.2f}, true },
  {"Slow Dance",        2, 260, 265, {0.10f, RES_HOME,  0.10f, 0.20f, 0.70f,  0.0f}, true },
  {"Sunday Morning",    2, 110, 345, {0.05f, RES_HOME,  0.15f, 0.15f, 0.65f, -0.3f}, true },
  {"Calm at Home",      2, 270, 345, {0.00f, RES_HOME,  0.10f, 0.10f, 0.60f, -0.2f}, true },
  {"Old Photograph",    2, 500, 345, {0.60f, RES_HOME,  0.25f, 0.55f, 0.65f, -0.1f}, true },
  // 4 AM
  {"Unsent Message",    3, 330, 105, {0.35f, RES_HANG,  0.50f, 0.80f, 0.70f, -0.1f}, false},
  {"Let Go",            3, 560, 105, {0.15f, RES_HOME,  0.35f, 0.60f, 0.65f, -0.3f}, true },
  {"Missed Call",       3, 300, 185, {0.45f, RES_HANG,  0.40f, 0.60f, 0.65f, -0.2f}, true },
  {"Forced Laughter",   3, 520, 185, {0.55f, RES_TWIST, 0.30f, 0.30f, 0.40f,  0.0f}, false},
  {"Rain on Glass",     3, 180, 265, {0.45f, RES_HANG,  0.40f, 0.45f, 0.70f, -0.2f}, true },
  {"City Lights",       3, 450, 265, {0.20f, RES_HANG,  0.55f, 0.45f, 0.60f,  0.4f}, true },
  {"Empty Apartment",   3, 130, 345, {0.50f, RES_HANG,  0.60f, 0.60f, 0.80f, -0.3f}, true },
  {"Endless Scroll",    3, 340, 345, {0.05f, RES_HANG,  0.20f, 0.20f, 0.50f, -0.5f}, false}};

// soft weights over the 8 shades of a corner: the dot can sit anywhere between them
inline void moodWeights(int corner, float x, float y, float* w) {
  const float inv = 1.0f / (2.0f * 85.0f * 85.0f);
  float sum = 0.0f;
  for (int j = 0; j < 8; ++j) {
    const Shade& s = SHADES[corner * 8 + j];
    const float dx = x - s.x, dy = y - s.y;
    w[j] = std::exp(-(dx * dx + dy * dy) * inv) + 1.0e-4f;
    sum += w[j];
  }
  for (int j = 0; j < 8; ++j) w[j] /= sum;
}

// ============================================================================
//  PAD RHYTHMS (every pattern ends on the 4th chord so the loop ending is always heard)
// ============================================================================
inline bool isMinorType(int ty) { return ty == M7 || ty == M9 || ty == M11 || ty == MMAJ7 || ty == M7B5 || ty == M69; }
inline bool isDomType(int ty) { return ty == D9 || ty == D13 || ty == D7B9 || ty == D7S11; }
struct PadPattern { int n; double s[4], l[4]; int cMap[4]; };

inline const PadPattern PAT_ALL4  = {4, {0.0, 4.0, 8.0, 12.0}, {4.0, 4.0, 4.0, 4.0}, {0, 1, 2, 3}};
inline const PadPattern PAT_SKIP1 = {3, {0.0, 8.0, 12.0, 0}, {8.0, 4.0, 4.0, 0}, {0, 2, 3, 0}};
inline const PadPattern PAT_SKIP2 = {3, {0.0, 8.0, 12.0, 0}, {8.0, 4.0, 4.0, 0}, {0, 1, 3, 0}};
inline const PadPattern PATTERNS_SLOW[3] = {PAT_SKIP1, PAT_ALL4, PAT_SKIP2};
inline const PadPattern PATTERNS_MID[3]  = {PAT_ALL4, PAT_SKIP1, PAT_ALL4};
inline const PadPattern PATTERNS_FAST[3] = {PAT_ALL4, PAT_ALL4, PAT_SKIP1};

// ============================================================================
//  GENERATOR
// ============================================================================
struct ChordGen {
  int root[4] = {0,0,0,0}, type[4] = {MAJ9,MAJ9,MAJ9,MAJ9}, bass[4] = {0,0,0,0};
  std::vector<int> voiced[4];
  std::vector<Hit> hits;
  Prog base{};
  int baseIdx = -1, effKey = 0;
  int topLine[4] = {72,72,72,72};
  float topCenter = 72.0f, tenseV = 0.4f;
  juce::String names;
  Feel feel;
  bool tearAt[4] = {false, false, false, false};

  static int pickIdx(juce::Random& rng, float dk, float tn, float sigma = 0.17f) {
    const size_t n = BANK.size();
    std::vector<double> w(n); double sum = 0;
    for (size_t i = 0; i < n; ++i) {
      const double dx = BANK[i].dk - dk, dy = BANK[i].tn - tn;
      w[i] = std::exp(-(dx * dx + dy * dy) / (2.0 * sigma * sigma)) + 0.002;
      sum += w[i];
    }
    double r = rng.nextDouble() * sum;
    for (size_t i = 0; i < n; ++i) { r -= w[i]; if (r <= 0.0) return (int) i; }
    return (int) n - 1;
  }

  void makeRhythm(juce::Random& rng, double bpm = 120.0) {
    hits.clear();
    const PadPattern* selectedPool = PATTERNS_MID;
    if (bpm < 85.0) selectedPool = PATTERNS_SLOW;
    else if (bpm > 115.0) selectedPool = PATTERNS_FAST;

    const PadPattern& p = selectedPool[rng.nextInt(3)];
    for (int k = 0; k < p.n; ++k) hits.push_back(Hit{ p.s[k], p.l[k], p.cMap[k] % 4 });

    // draws always in the same order so sliders do not reshuffle the progression
    const float ra = rng.nextFloat(), rb = rng.nextFloat(), t1 = rng.nextFloat(), t2 = rng.nextFloat();
    const int ka = 1 + rng.nextInt((int) hits.size() - 1);

    // AIR: chord arrives half a beat early
    if (ra < feel.air && hits[(size_t) ka - 1].len >= 3.0) {
      hits[(size_t) ka].start -= 0.5; hits[(size_t) ka].len += 0.5; hits[(size_t) ka - 1].len -= 0.5;
    }
    // AIR: breath before the loop restarts
    if (rb < feel.air * 0.6f && hits.back().len >= 4.0) hits.back().len -= 1.0;

    // TEAR: where the top voice is suspended (climax and arrival)
    for (bool& b : tearAt) b = false;
    tearAt[2] = t1 < feel.tear;
    tearAt[0] = t2 < feel.tear * 0.5f;
    tearify();
  }

  void tearify() {
    hits.erase(std::remove_if(hits.begin(), hits.end(), [](const Hit& h) { return h.topOnly; }), hits.end());
    std::vector<Hit> extra;
    for (auto& h : hits) {
      h.sus = -1; h.susLen = 0;
      if (h.chord < 0 || h.chord > 3 || !tearAt[h.chord] || h.len < 4.0) continue;
      const auto& v = voiced[h.chord];
      if (v.size() < 4) continue;
      const int top = v.back();
      int sus = -1;
      for (int add : {2, 1}) {
        const int cand = top + add; bool ok = cand <= 90;
        for (size_t i = 0; i + 1 < v.size(); ++i) {
          if (v[i] % 12 == cand % 12) ok = false;
          if (i > 0 && std::abs(v[i] - cand) == 1) ok = false;
        }
        if (ok) { sus = cand; break; }
      }
      if (sus < 0) continue;
      h.sus = sus; h.susLen = h.len >= 8.0 ? 2.0 : 1.0;
      Hit e; e.start = h.start + h.susLen; e.len = h.len - h.susLen; e.chord = h.chord; e.topOnly = true;
      extra.push_back(e);
    }
    for (auto& e : extra) hits.push_back(e);
    std::stable_sort(hits.begin(), hits.end(), [](const Hit& a, const Hit& b) { return a.start < b.start; });
  }

  // ek = effective key 0..11
  void apply(juce::Random& rng, int ek, float tense) {
    tenseV = tense;
    int r[4], t[4], b[4];
    for (int i = 0; i < 4; ++i) {
      r[i] = base.c[i].r; b[i] = base.c[i].b; t[i] = base.c[i].t;
      const float x = rng.nextFloat();
      if (t[i] == D7B9 && x < 0.4f * (1.0f - tense)) t[i] = D9;
      else if (t[i] == MAJ7S11 && x < 0.3f * (1.0f - tense)) t[i] = MAJ9;
      else if (t[i] == D7S11 && x < 0.3f * (1.0f - tense)) t[i] = D9;
      else if (t[i] == M11 && x < 0.3f * (1.0f - tense)) t[i] = M9;
    }
    const bool minorKey = base.mn;
    const float rn = rng.nextFloat(), rp = rng.nextFloat(), rq = rng.nextFloat();

    // NOSTALGIA: bittersweet borrowed chord. Major: IV -> iv (or bVI), vi -> bVI. Minor: iv -> dorian IV9.
    bool hasIv = false;
    for (int i = 0; i < 4; ++i) if (r[i] == 5 && (isMinorType(t[i]) || isDomType(t[i]))) hasIv = true;
    if (rn < feel.nost && !hasIv) {
      auto borrow = [&](int k) {
        if (!minorKey) {
          if (r[k] == 5 && !isMinorType(t[k]) && !isDomType(t[k])) {
            b[k] = -1;
            if (rp < 0.7f) t[k] = M9; else { r[k] = 8; t[k] = MAJ9; }
            return true;
          }
          if (r[k] == 9 && isMinorType(t[k])) { b[k] = -1; r[k] = 8; t[k] = MAJ9; return true; }
        } else if (r[k] == 5 && isMinorType(t[k])) { b[k] = -1; t[k] = D9; return true; }
        return false;
      };
      for (int k : {2, 1, 3}) if (borrow(k)) break;
    }

    // LOOP ENDING: home / hang / twist
    const bool domLast = isDomType(t[3]) || (r[3] == 7 && !isMinorType(t[3]) && t[3] != MAJ9 && t[3] != SIX9);
    if (feel.res == RES_HANG) {
      if (domLast) t[3] = SUS9;
    } else if (feel.res == RES_HOME) {
      if (t[3] == SUS9) t[3] = tense > 0.6f ? D7B9 : D9;
    } else {
      if (t[3] == SUS9 && r[3] == 7) t[3] = tense > 0.6f ? D7B9 : D9;
      if (rq < 0.85f) {
        if (isDomType(t[2])) { r[3] = minorKey ? 8 : 9; b[3] = -1; t[3] = minorKey ? MAJ9 : M9; }   // deceptive
        else if (isDomType(t[3]) && r[3] == 7) { r[3] = 1; b[3] = -1; t[3] = D9; }                // tritone sub
      }
    }

    for (int i = 0; i < 4; ++i) {
      root[i] = (r[i] + ek) % 12;
      bass[i] = ((b[i] < 0 ? r[i] : b[i]) + ek) % 12;
      type[i] = t[i];
    }
  }

  void planTop(juce::Random& rng) {
    std::vector<int> cands[4];
    for (int i = 0; i < 4; ++i) {
      const auto& iv = IV[type[i]];
      std::vector<int> pcs;
      for (size_t k = 1; k < iv.size(); ++k) {
        if (iv[k] % 12 == 7 && iv.size() >= 5) continue;
        if (iv[k] % 12 == 1) continue;
        pcs.push_back((root[i] + iv[k]) % 12);
      }
      for (int m = (int) topCenter - 4; m <= (int) topCenter + 4; ++m)
        for (int pc : pcs) if (m % 12 == pc) cands[i].push_back(m);
      if (cands[i].empty())
        for (int m = 64; m <= 76; ++m) for (int pc : pcs) if (m % 12 == pc) cands[i].push_back(m);
      if (cands[i].empty()) cands[i].push_back(72);
    }
    double bestS = 1e18; int bv[4] = {72, 72, 72, 72};
    for (int a : cands[0]) for (int b : cands[1]) for (int c : cands[2]) for (int d : cands[3]) {
      const int v[4] = {a, b, c, d};
      double s = rng.nextFloat();
      for (int i = 0; i < 4; ++i) {
        const int leap = std::abs(v[i] - v[(i + 1) % 4]);
        if (leap == 0) s += 0.8;
        else if (leap <= 2) s += 0.4;
        else if (leap <= 4) s += 2.0 + (leap - 2) * 0.5;
        else s += 4.0 + (leap - 4) * 2.5;
      }
      const int mx = std::max(std::max(v[0], v[1]), std::max(v[2], v[3]));
      if (v[2] == mx && v[0] < mx && v[1] < mx && v[3] < mx) s -= 3.0;
      else if (v[3] == mx) s += 2.0;
      if (v[3] < v[2]) s -= 1.0;
      if (v[0] == v[2]) s -= 1.5;
      s += std::abs((a + b + c + d) / 4.0 - topCenter) * 0.3;
      if (s < bestS) { bestS = s; for (int i = 0; i < 4; ++i) bv[i] = v[i]; }
    }
    for (int i = 0; i < 4; ++i) topLine[i] = bv[i];
  }

  double quality() const {
    static const double ideal[4] = {0.25, 0.5, 1.0, 0.5};
    double q = 0;
    for (int i = 0; i < 4; ++i) {
      const auto& a = voiced[i]; const auto& b = voiced[(i + 1) % 4];
      if (a.size() < 2 || b.size() < 2) return 1e9;
      const int bl = std::abs(a[0] - b[0]); if (bl > 5) q += (bl - 5) * 3;
      const int tl = std::abs(a.back() - b.back()); if (tl > 4) q += (tl - 4) * 2;
      q += 3.0 * std::abs(tensOf(type[i]) - ideal[i] * (0.4 + 0.6 * (double) tenseV));
    }
    return q;
  }

  void generate(juce::Random& rng, int key, float dark, float tense, bool open, double bpm = 120.0, bool differ = false) {
    const int avoid = differ ? baseIdx : -1;
    const int ek = (key == 12) ? rng.nextInt(12) : key;   // random key is chosen once, not per attempt
    ChordGen best; double bq = 1e18;
    for (int a = 0; a < 30; ++a) {
      ChordGen t; t.feel = feel; t.effKey = ek;
      t.baseIdx = pickIdx(rng, dark, tense); t.base = BANK[(size_t) t.baseIdx];
      t.topCenter = juce::jlimit(66.0f, 74.0f, 74.0f - 4.0f * dark + 2.5f * feel.reg);
      t.apply(rng, ek, tense); t.revoice(open, rng, true); t.makeRhythm(rng, bpm);
      double q = t.quality();
      if (t.baseIdx == avoid) q += 25;
      if (q < bq) { bq = q; best = t; }
    }
    *this = best;
  }

  void evolve(juce::Random& rng, int key, float tense, bool open, double bpm = 120.0) {
    const int ek = (key == 12) ? effKey : key;
    ChordGen best; double bq = 1e18; bool found = false;
    for (int a = 0; a < 12; ++a) {
      const int k = 1 + rng.nextInt(3);
      const Prog* q = nullptr;
      for (int tries = 0; tries < 8 && !q; ++tries) {          // only borrow from progressions of the same tonality
        const Prog& c = BANK[(size_t) pickIdx(rng, base.dk, base.tn, 0.2f)];
        if (c.mn == base.mn) q = &c;
      }
      if (!q) continue;
      const Ch nc = q->c[k]; const Ch old = base.c[k];
      if (nc.r == old.r && nc.t == old.t && nc.b == old.b) continue;
      Prog np = base; np.c[k] = nc;
      ChordGen t = *this; t.base = np; t.effKey = ek;
      t.apply(rng, ek, tense); t.revoice(open, rng, true); t.makeRhythm(rng, bpm);
      const double qq = t.quality();
      if (qq < bq) { bq = qq; best = t; found = true; }
    }
    if (found) *this = best;
  }

  void transpose(int d, bool open, juce::Random& rng) {
    effKey = (effKey + d) % 12;
    for (int i = 0; i < 4; ++i) {
      root[i] = (root[i] + d) % 12; bass[i] = (bass[i] + d) % 12;
      topLine[i] += d; if (topLine[i] > 77) topLine[i] -= 12;
    }
    revoice(open, rng);
  }

  static std::vector<int> pick(int rt, int t, const std::vector<int>& prev, juce::Random& rng, int top = -1) {
    const auto& iv = IV[t]; std::vector<int> pcs;
    for (size_t i = 1; i < iv.size(); ++i) { if (iv[i] % 12 == 7 && iv.size() >= 5) continue; pcs.push_back((rt + iv[i]) % 12); }
    std::vector<std::vector<int>> cand;
    for (int pc : pcs) { std::vector<int> a; for (int m = 59; m <= 78; ++m) if (m % 12 == pc) a.push_back(m); cand.push_back(a); }
    std::vector<int> best, cur; double bestCost = 1e9;
    std::function<void(size_t)> rec = [&](size_t k) {
      if (k == cand.size()) {
        auto s = cur; std::sort(s.begin(), s.end());
        double mean = 0; for (int v : s) mean += v; mean /= (double) s.size();
        double cost = rng.nextFloat() * 0.5;
        if (s.back() - s.front() > 14) cost += 100;
        if (top >= 0 && s.back() != top) cost += 400;
        for (size_t i = 0; i + 1 < s.size(); ++i) {
          const int gap = s[i + 1] - s[i];
          if (gap < 3 && s[i] < 66) cost += 25; else if (gap == 1) cost += 2;
        }
        if (!prev.empty()) {
          double pm = 0; for (int v : prev) pm += v; pm /= (double) prev.size();
          for (int v : s) { int d = 99; for (int q : prev) d = std::min(d, std::abs(v - q)); cost += d; }
          cost += std::abs(mean - pm) * 0.5;
        } else cost += std::abs(mean - 68) * 1.5;
        if (cost < bestCost) { bestCost = cost; best = s; }
        return;
      }
      for (int n : cand[k]) { cur.push_back(n); rec(k + 1); cur.pop_back(); }
    };
    rec(0);
    return best;
  }

  void revoice(bool open, juce::Random& rng, bool replan = false) {
    if (replan) planTop(rng);
    std::vector<int> prev; juce::StringArray nm; int prevBass = -1;
    for (int i = 0; i < 4; ++i) {
      auto up = pick(root[i], type[i], prev, rng, topLine[i]); prev = up;
      int bassNote = 31 + (((bass[i] - 31) % 12) + 12) % 12;
      if (prevBass >= 0) {
        int bestD = 99;
        for (int m = 31; m <= 43; ++m)
          if (m % 12 == bass[i] && std::abs(m - prevBass) < bestD) { bestD = std::abs(m - prevBass); bassNote = m; }
      }
      prevBass = bassNote;
      if (open && up.size() >= 4) {
        const int dn = up[up.size() - 2] - 12;
        if (dn >= 55) up[up.size() - 2] = dn;
      }
      std::sort(up.begin(), up.end());
      voiced[i].clear(); voiced[i].push_back(bassNote);
      for (int v : up) voiced[i].push_back(v);
      juce::String n = juce::String(NOTE_NAMES[root[i]]) + SUF[type[i]];
      if (bass[i] != root[i]) n += juce::String("/") + NOTE_NAMES[bass[i]];
      nm.add(n);
    }
    names = nm.joinIntoString("   |   ");
    tearify();
  }
};
