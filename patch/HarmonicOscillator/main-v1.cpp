/*
  Harmonic Oscillator Synth - v1.0
  ---------------------------------
  A 4-voice harmonic oscillator using the Daisy Patch and DaisySP's VariableShapeOscillator.

  Features:
  - Selectable harmonic ratio systems (Just Intonation, Harmonic Series, Subharmonics, etc.)
  - Smooth morphing between waveforms
  - CV pitch input with 1V/oct scaling
  - Wide frequency range from LFOs to full pitch

  Controls:
  - CTRL_1: Pitch offset over a 3-octave range (scaled by CTRL_4)
  - CTRL_2: Harmonic ratio selection (morphs through ratio array)
  - CTRL_3: Wave shape morph (sine → triangle → square)
  - CTRL_4: Sets the maximum frequency/octave range (from ~0.2Hz to 220Hz)
  - Encoder Press: Cycle through harmonic ratio systems
  - CV_1: 1V/oct pitch input
*/

#include "daisy_patch.h"
#include "daisysp.h"
#include <string>
#include <cmath>

#define MAX_FREQ 220.0f // Maximum frequency when CTRL_4 is at max
#define MIN_FREQ 0.1f  // Minimum frequency when CTRL_4 is at min (LFO rate)

using namespace daisy;
using namespace daisysp;

float f = 0.0f; // TEST
float oscFreqs[4] = {0};

DaisyPatch patch;
VariableShapeOscillator oscillators[4]; // Using VariableShapeOscillator

// Expanded just intonation ratio systems (at least 12 per set)
static const float kCommonChords[12] = {
    0.5f, 0.666f, 0.75f, 0.833f, 1.0f, 1.125f, 1.25f, 1.333f, 1.5f, 1.667f, 1.875f, 2.0f
};

static const float kHarmonicSeries[12] = {
    1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f
};

static const float kSubharmonics[12] = {
    0.5f, 0.666f, 0.75f, 0.833f, 1.0f, 1.2f, 1.333f, 1.5f, 1.666f, 2.0f, 2.5f, 3.0f
};

static const float kFullRatios[12] = {
    0.5f, 0.666f, 0.75f, 0.833f, 1.0f, 1.125f, 1.25f, 1.333f, 1.5f, 1.667f, 1.875f, 2.0f
};

// 3-limit (Pythagorean tuning) - powers of 2 and 3 only
static const float kLimit3[12] = {
    1.0f, 3.0f/2.0f, 4.0f/3.0f, 9.0f/8.0f, 27.0f/16.0f, 16.0f/9.0f, 2.0f, 8.0f/9.0f, 32.0f/27.0f, 243.0f/128.0f, 81.0f/64.0f, 128.0f/81.0f
};

// 5-limit tuning - includes prime 5 intervals (pure major/minor thirds)
static const float kLimit5[12] = {
    1.0f, 5.0f/4.0f, 6.0f/5.0f, 4.0f/3.0f, 3.0f/2.0f, 5.0f/3.0f, 8.0f/5.0f, 9.0f/8.0f, 15.0f/8.0f, 10.0f/9.0f, 16.0f/15.0f, 2.0f
};

// Tides-style: unison center, harmonics above, subharmonics below
static const float kTidesStyle[12] = {
    1.0f, 3.0f/2.0f, 2.0f, 3.0f, 0.75f, 0.666f, 0.5f, 0.333f, 1.25f, 1.5f, 2.25f, 2.5f
};

int mode = 0; // Track encoder mode selection

void InitOscillators() {
    for (int i = 0; i < 4; i++) {
        oscillators[i].Init(patch.AudioSampleRate());
        oscillators[i].SetPW(0.5f);
    }
}

void DisplayCurrentMode() {
    patch.display.Fill(false);
    std::string modeName;
    switch (mode) {
        case 0: modeName = "Common Chords"; break;
        case 1: modeName = "Harmonic Series"; break;
        case 2: modeName = "Subharmonics"; break;
        case 3: modeName = "Full Ratios"; break;
        case 4: modeName = "Limit 3"; break;
        case 5: modeName = "Limit 5"; break;
        default: modeName = "Tides Style"; break;
    }
    patch.display.SetCursor(0, 0);
    patch.display.WriteString(modeName.c_str(), Font_7x10, true);

    char buffer[32];
    for (int i = 0; i < 4; i++) {
        snprintf(buffer, sizeof(buffer), "Out %d: %d Hz", i + 1, (int)(oscFreqs[i]));
        patch.display.SetCursor(0, 12 + (i * 10));
        patch.display.WriteString(buffer, Font_7x10, true);
    }

    snprintf(buffer, sizeof(buffer), "Knob1: %.2f", patch.GetKnobValue(DaisyPatch::CTRL_1));
    patch.display.SetCursor(0, 54);
    patch.display.WriteString(buffer, Font_7x10, true);

    patch.display.Update();
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    patch.ProcessAllControls();

    float octaveShift = patch.GetKnobValue(DaisyPatch::CTRL_4);
    float maxFreq = MIN_FREQ * powf(2.0f, octaveShift * log2f(MAX_FREQ / MIN_FREQ));

    float pitchOffset = patch.GetKnobValue(DaisyPatch::CTRL_1);
    float baseFreq = maxFreq * powf(0.015625f, (1.0f - pitchOffset));

    // TEMP: ignore CV to debug knob behavior
    // float cv = patch.controls[4].Value() * 5.0f;
    // float cvFactor = powf(2.0f, cv);
    // baseFreq *= cvFactor;

    f = baseFreq; // display for debugging

    const float* ratioSet;
    int numRatios;
    switch (mode) {
        case 0: ratioSet = kCommonChords; numRatios = 12; break;
        case 1: ratioSet = kHarmonicSeries; numRatios = 12; break;
        case 2: ratioSet = kSubharmonics; numRatios = 12; break;
        case 3: ratioSet = kFullRatios; numRatios = 12; break;
        case 4: ratioSet = kLimit3; numRatios = 12; break;
        case 5: ratioSet = kLimit5; numRatios = 12; break;
        default: ratioSet = kTidesStyle; numRatios = 12; break;
    }

    int ratioIndex = powf(patch.GetKnobValue(DaisyPatch::CTRL_2), 2.0f) * (numRatios - 1);
    ratioIndex = fminf(fmaxf(ratioIndex, 0), numRatios - 1);

    for (int i = 0; i < 4; i++) {
        int rIdx = (ratioIndex + i) % numRatios;
        float freq = baseFreq * ratioSet[rIdx];
        oscillators[i].SetSync(true);
        oscillators[i].SetFreq(freq);
        oscillators[i].SetWaveshape(patch.GetKnobValue(DaisyPatch::CTRL_3));
        oscFreqs[i] = freq;
    }

    for (size_t i = 0; i < size; i++) {
        for (int j = 0; j < 4; j++) {
            out[j][i] = oscillators[j].Process();
        }
    }
}

int main(void) {
    patch.Init();
    patch.StartAdc();
    InitOscillators();
    patch.StartAudio(AudioCallback);

    while (1) {
        patch.ProcessDigitalControls();
        if (patch.encoder.RisingEdge()) {
            mode = (mode + 1) % 7;
            DisplayCurrentMode();
        }
    }
}
