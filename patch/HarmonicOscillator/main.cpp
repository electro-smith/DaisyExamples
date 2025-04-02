/*
  Harmonic Oscillator Synth - v2.0
  --------------------------------
  A 4-voice oscillator engine with CV and knob control, using MorphOscillator for waveform blending.

  Features:
  - 4 independent voices sent to Out 1–4
  - Smooth waveform morphing from sine → triangle → saw → square
  - Selectable harmonic ratio systems: just intonation, 3/5/7/11-limit, harmonic series, subharmonics, and Tides-style
  - CTRL_1: Pitch sweep (over 6 octaves, ending at max frequency)
  - CTRL_2: Ratio selector (offset through chosen harmonic set)
  - CTRL_3: Waveform morph control
  - CTRL_4: Sets the top of the pitch range (from LFO to ~440Hz)
  - Encoder press: Cycles through harmonic ratio systems
*/

#include "daisy_patch.h"
#include "daisysp.h"
#include "ratios.h"
#include "MorphOscillator.h"
#include <string>
#include <cmath>

#define MAX_FREQ 440.0f
#define MIN_FREQ 0.2f

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
MorphOscillator oscillators[4];
float oscFreqs[4] = {0};
int mode = 0;

void InitOscillators() {
    for (int i = 0; i < 4; i++) {
        oscillators[i].Init(patch.AudioSampleRate());
        oscillators[i].SetAmp(0.5f);
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
        case 6: modeName = "Limit 7"; break;
        case 7: modeName = "Limit 11"; break;
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

    const float* ratioSet;
    int numRatios;
    switch (mode) {
        case 0: ratioSet = kCommonChords; numRatios = 12; break;
        case 1: ratioSet = kHarmonicSeries; numRatios = 12; break;
        case 2: ratioSet = kSubharmonics; numRatios = 12; break;
        case 3: ratioSet = kFullRatios; numRatios = 12; break;
        case 4: ratioSet = kLimit3; numRatios = 12; break;
        case 5: ratioSet = kLimit5; numRatios = 12; break;
        case 6: ratioSet = kLimit7; numRatios = 12; break;
        case 7: ratioSet = kLimit11; numRatios = 12; break;
        default: ratioSet = kTidesStyle; numRatios = 12; break;
    }

    int ratioIndex = powf(patch.GetKnobValue(DaisyPatch::CTRL_2), 2.0f) * (numRatios - 1);
    ratioIndex = fminf(fmaxf(ratioIndex, 0), numRatios - 1);

    for (int i = 0; i < 4; i++) {
        int rIdx = (ratioIndex + i) % numRatios;
        float freq = baseFreq * ratioSet[rIdx];
        oscillators[i].SetFreq(freq);
        oscillators[i].SetShape(patch.GetKnobValue(DaisyPatch::CTRL_3));
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
            mode = (mode + 1) % 9;
            DisplayCurrentMode();
        }
    }
}
