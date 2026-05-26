#include "MorphOscillator.h"

using namespace daisysp;

void MorphOscillator::Init(float sample_rate) {
    for(int i = 0; i < 4; i++) {
        osc[i].Init(sample_rate);
        osc[i].SetAmp(1.0f);
    }

    osc[0].SetWaveform(Oscillator::WAVE_SIN);
    osc[1].SetWaveform(Oscillator::WAVE_TRI);
    osc[2].SetWaveform(Oscillator::WAVE_SAW);
    osc[3].SetWaveform(Oscillator::WAVE_SQUARE);

    freq = 440.0f;
    shape = 0.0f;
}

void MorphOscillator::SetFreq(float f) {
    freq = f;
    for(int i = 0; i < 4; i++) {
        osc[i].SetFreq(freq);
    }
}

void MorphOscillator::SetShape(float s) {
    shape = fminf(fmaxf(s, 0.0f), 1.0f); // Clamp between 0 and 1
}

void MorphOscillator::SetAmp(float a) {
    for(int i = 0; i < 4; i++) {
        osc[i].SetAmp(a);
    }
}

float MorphOscillator::Process() {
    float morph = shape * 3.0f; // 0 - 3 selects 4 waveforms 
    int index = (int)morph;     // Get the index of the starting wave soemthing like 1.23
    float frac = morph - index; // Get the get the fraction of the next wave form 0.23

    float a = osc[index].Process(); // Get the value of the current wave form
    float b = osc[(index + 1 < 4) ? (index + 1) : 3].Process(); // get the next wave form

    return a * (1.0f - frac) + b * frac; // Mix the wave with a fraction of the next wave
}