#pragma once

struct Ratio {
    float value;
    const char* label;
};

// Example: 5-limit Just Intonation with symmetric structure
static const Ratio kLimit5Symmetrical[] = {
    { 0.5f,   "1:2 (Octave Down)" },
    { 0.666f, "2:3 (Perfect Fifth Down)" },
    { 0.75f,  "3:4 (Perfect Fourth Down)" },
    { 0.8f,   "4:5 (Major Third Down)" },
    { 0.833f, "5:6 (Minor Third Down)" },
    { 1.0f,   "1:1 (Unison)" },
    { 1.2f,   "6:5 (Minor Third Up)" },
    { 1.25f,  "5:4 (Major Third Up)" },
    { 1.333f, "4:3 (Perfect Fourth Up)" },
    { 1.5f,   "3:2 (Perfect Fifth Up)" },
    { 1.666f, "5:3 (Major Sixth Up)" },
    { 2.0f,   "2:1 (Octave Up)" }
};

static const int kLimit5SymmetricalSize = sizeof(kLimit5Symmetrical) / sizeof(Ratio);










// #pragma once

// // Common Chords (triads and simple intervals)
// static const float kCommonChords[12] = {
//     0.5f, 0.666f, 0.75f, 0.833f, 1.0f, 1.125f, 1.25f, 1.333f, 1.5f, 1.667f, 1.875f, 2.0f
// };

// // Harmonic Series (partials of a fundamental)
// static const float kHarmonicSeries[12] = {
//     1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f
// };

// // Subharmonics (inverted harmonic series)
// static const float kSubharmonics[12] = {
//     0.5f, 0.666f, 0.75f, 0.833f, 1.0f, 1.2f, 1.333f, 1.5f, 1.666f, 2.0f, 2.5f, 3.0f
// };

// // Extended Just Intonation Ratios
// static const float kFullRatios[12] = {
//     0.5f, 0.666f, 0.75f, 0.833f, 1.0f, 1.125f, 1.25f, 1.333f, 1.5f, 1.667f, 1.875f, 2.0f
// };

// // Pythagorean / 3-limit Tuning
// static const float kLimit3[12] = {
//     1.0f, 3.0f/2.0f, 4.0f/3.0f, 9.0f/8.0f, 27.0f/16.0f, 16.0f/9.0f,
//     2.0f, 8.0f/9.0f, 32.0f/27.0f, 243.0f/128.0f, 81.0f/64.0f, 128.0f/81.0f
// };

// // 5-limit Just Intonation (includes pure thirds)
// static const float kLimit5[12] = {
//     1.0f, 5.0f/4.0f, 6.0f/5.0f, 4.0f/3.0f, 3.0f/2.0f, 5.0f/3.0f,
//     8.0f/5.0f, 9.0f/8.0f, 15.0f/8.0f, 10.0f/9.0f, 16.0f/15.0f, 2.0f
// };

// // 7-limit Just Intonation (includes septimal intervals)
// static const float kLimit7[12] = {
//     1.0f, 7.0f/6.0f, 8.0f/7.0f, 7.0f/5.0f, 10.0f/7.0f, 7.0f/4.0f,
//     9.0f/7.0f, 14.0f/9.0f, 7.0f/3.0f, 21.0f/16.0f, 28.0f/27.0f, 2.0f
// };

// // 11-limit Just Intonation (includes more complex prime ratios)
// static const float kLimit11[12] = {
//     1.0f, 11.0f/10.0f, 11.0f/8.0f, 9.0f/11.0f, 11.0f/6.0f, 22.0f/15.0f,
//     33.0f/22.0f, 11.0f/9.0f, 16.0f/11.0f, 11.0f/7.0f, 11.0f/12.0f, 2.0f
// };

// // Tides-style: harmonics above and subharmonics below
// static const float kTidesStyle[12] = {
//     1.0f, 3.0f/2.0f, 2.0f, 3.0f, 0.75f, 0.666f,
//     0.5f, 0.333f, 1.25f, 1.5f, 2.25f, 2.5f
// };
