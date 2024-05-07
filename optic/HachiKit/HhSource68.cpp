#include "HhSource68.h"

using namespace daisy;
using namespace daisysp;

const float HhSource68::freqs606[OSC_COUNT] = { 245, 306, 365, 415, 437, 619 };
const float HhSource68::freqs808[OSC_COUNT] = { 204, 298, 366, 515, 540, 800 };
const float HhSource68::MIN_FREQ_FACTOR = 0.1;
const float HhSource68::MORPH_606_VALUE = 0.35;
const float HhSource68::MORPH_808_VALUE = 0.65;
const float HhSource68::HH_HPF_MAX = 12000;
const float HhSource68::HH_HPF_MIN = 100;
const float HhSource68::GAIN_MAX = 100;


void HhSource68::Init(float sample_rate) {
    Init(sample_rate, MORPH_808_VALUE);
}

void HhSource68::Init(float sample_rate, float morph) {
    
    oscs[0] = &osc0;
    oscs[1] = &osc1;
    oscs[2] = &osc2;
    oscs[3] = &osc3;
    oscs[4] = &osc4;
    oscs[5] = &osc5;

    for (int osc = 0; osc < OSC_COUNT; osc++) {
        oscs[osc]->Init(sample_rate);
        oscs[osc]->SetWaveform(Oscillator::WAVE_SQUARE);
        oscs[osc]->SetFreq(freqs808[osc]);
    }

    hpf.Init(sample_rate);
    hpf.SetRes(0);
    hpf.SetDrive(.002);
    hpf.SetFreq(2700);

    lpf.Init(sample_rate);
    lpf.SetRes(0);
    lpf.SetDrive(.002);
    lpf.SetFreq(5000);

    SetMorph(morph);
    signal = 0.0f;
}

float HhSource68::Process() {
    float sig = 0.0f;
    for (int osc = 0; osc < OSC_COUNT; osc++) {
        sig += oscs[osc]->Process();
    }
    // sig = sig / OSC_COUNT;

    hpf.Process(sig);
    lpf.Process(hpf.High());
    // signal = gain * lpf.Low();;
    signal = lpf.Low();;

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
        // freq = std::max(freq, freqs606[osc] * MIN_FREQ_FACTOR);  // make sure freqs don't go to zero (and mins aren't all the same)
        freq = std::max(freq, 200.0f);
        oscs[osc]->SetFreq(freq);
    }
}

float HhSource68::GetHpfFrequency() {
    return hpfFreq;
}

void HhSource68::SetHpfFrequency(float freq) {
    hpfFreq = freq;
    hpf.SetFreq(hpfFreq);    
    gain = Utility::LimitFloat(1.0f + freq * (HH_HPF_MAX - HH_HPF_MIN) * (GAIN_MAX - 1), 1.0f, GAIN_MAX);
}

float HhSource68::GetLpfFrequency() {
    return lpfFreq;
}

void HhSource68::SetLpfFrequency(float freq) {
    lpfFreq = freq;
    lpf.SetFreq(lpfFreq);    
}
