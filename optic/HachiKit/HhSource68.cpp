#include "HhSource68.h"

using namespace daisy;
using namespace daisysp;

const float HhSource68::freqs606[OSC_COUNT] = { 245, 306, 365, 415, 437, 619 };
const float HhSource68::freqs808[OSC_COUNT] = { 204, 298, 366, 515, 540, 800 };
const float HhSource68::MIN_FREQ_FACTOR = 0.1;


void HhSource68::Init(float sample_rate) {
    Init(sample_rate, MORPH_808_VALUE);
}

void HhSource68::Init(float sample_rate, float morph) {
    
    for (int osc = 0; osc < OSC_COUNT; osc++) {
        oscs[osc].Init(sample_rate);
        oscs[osc].SetWaveform(Oscillator::WAVE_SQUARE);
        oscs[osc].SetFreq(freqs808[osc]);
    }

    SetMorph(morph);
    signal = 0.0f;
}

float HhSource68::Process() {
    float sig = 0.0f;
    for (int osc = 0; osc < OSC_COUNT; osc++) {
        sig += oscs[osc].Process();
    }
    sig /= OSC_COUNT;

    signal = sig;
    return signal;
}

float HhSource68::Signal() {
    return signal;
}

float HhSource68::GetMorph() {
    return this->morph;
}

void HhSource68::SetMorph(float morph) {

    this->morph = morph;

    float range68 = MORPH_808_VALUE - MORPH_606_VALUE; // assumes 8>6 and both betw 0 and 1
    float weight8 = (morph - MORPH_606_VALUE) * (1 / range68);
    float weight6 = 1 - weight8;

    for (int osc = 0; osc < OSC_COUNT; osc++) {
        float freq = weight6 * freqs606[osc] + weight8 * freqs808[osc];
        freq = std::max(freq, freqs606[osc] * MIN_FREQ_FACTOR);  // make sure freqs don't go to zero (and mins aren't all the same)
        oscs[osc].SetFreq(freq);
    }

}