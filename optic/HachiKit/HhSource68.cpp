#include "HhSource68.h"

using namespace daisy;
using namespace daisysp;

const float HhSource68::freqs606[OSC_COUNT] = { 245, 306, 365, 415, 437, 619 };
const float HhSource68::freqs808[OSC_COUNT] = { 204, 298, 366, 515, 540, 800 };
const float HhSource68::MIN_FREQ_FACTOR = 0.1;
const float HhSource68::MORPH_606_VALUE = 0.35;
const float HhSource68::MORPH_808_VALUE = 0.65;
const float HhSource68::HPF_MAX = 12000;
const float HhSource68::HPF_MIN = 100;
const float HhSource68::LPF_MAX = 18000;
const float HhSource68::LPF_MIN = 100;
const float HhSource68::GAIN_MAX = 100;


void HhSource68::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, MORPH_808_VALUE);
}

void HhSource68::Init(std::string slot, float sample_rate, float morph) {
    
    this->slot = slot;
    
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
    hpf.SetRes(0.05);
    hpf.SetDrive(.002);
    hpf.SetFreq(2700);

    lpf.Init(sample_rate);
    lpf.SetRes(0.05);
    lpf.SetDrive(.002);
    lpf.SetFreq(5000);

    SetMorph(morph);
    signal = 0.0f;
}

float HhSource68::Process() {
    float sig = cowSignal = lowCowSignal = 0.0f;
    for (int osc = 0; osc < OSC_COUNT; osc++) {
        float oscSignal = oscs[osc]->Process();
        sig += oscSignal;
        if (osc == 4 || osc == 5) {
            cowSignal += oscSignal;
        }
        if (osc == 3 || osc == 4) {
            lowCowSignal += oscSignal;
        }
    }
    // sig = sig / OSC_COUNT;

    hpf.Process(sig);
    lpf.Process(hpf.High());
    // signal = gain * lpf.Low();;
    signal = lpf.Low();;

    return signal;
}

void HhSource68::Trigger(float velocity) {
    // NOP
}

float HhSource68::Signal() {
    return signal;
}

float HhSource68::Cowbell(bool isLow) {
    return isLow ? lowCowSignal : cowSignal;
}

void HhSource68::SetMorph(float morph) {

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

float HhSource68::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string HhSource68::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_MORPH: 
                return std::to_string((int)GetParam(param * 100));
            case PARAM_HPF:
            case PARAM_LPF:
                return std::to_string((int)GetParam(param));// + "hz";
        }
    }
   return "";
 }

float HhSource68::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_MORPH: 
                scaled = parameters[param].Update(raw, Utility::Limit(raw));
                SetMorph(scaled);
                break;
            case PARAM_HPF:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, HPF_MIN, HPF_MAX, Parameter::EXPONENTIAL));
                hpf.SetFreq(scaled);
                break;
            case PARAM_LPF:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, LPF_MIN, LPF_MAX, Parameter::EXPONENTIAL));
                lpf.SetFreq(scaled);
                break;
        }
    }

    return scaled;    
}

void HhSource68::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void HhSource68::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_MORPH: 
                parameters[param].SetScaledValue(scaled);
                SetMorph(scaled);
                break;
            case PARAM_HPF:
                parameters[param].SetScaledValue(scaled);
                hpf.SetFreq(scaled);
                break;
            case PARAM_LPF:
                parameters[param].SetScaledValue(scaled);
                lpf.SetFreq(scaled);
                break;
        }
    }
}
