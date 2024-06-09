#include "Sd8.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

const std::string Sd8::paramNames[]  = { "oFrq", "oDcy", "nDcy", "Mix", "oAtk", "nAtk" };


void Sd8::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 153, 0.001, 1.212, 0.001, 0.971, 0.5);
}

void Sd8::Init(std::string slot, float sample_rate, float oscFrequency, float oscAttack, float oscDecay, float noiseAttack, float noiseDecay, float mix) {

    this->slot = slot;
    
    // oscillator settings
    osc.Init(sample_rate);
    SetParam(PARAM_OSC_FREQUENCY, oscFrequency);
    osc.SetWaveform(Oscillator::WAVE_SIN);

    // oscEnv settings
    oscEnv.Init(sample_rate);
    oscEnv.SetMax(1);
    oscEnv.SetMin(0);
    oscEnv.SetCurve(-20);
    SetParam(PARAM_OSC_ATTACK, oscAttack);
    SetParam(PARAM_OSC_DECAY, oscDecay);

    // noise
    noise.Init();

    // noiseEnv settings
    noiseEnv.Init(sample_rate);
    noiseEnv.SetMax(1);
    noiseEnv.SetMin(0);
    noiseEnv.SetCurve(-20);
    SetParam(PARAM_NOISE_ATTACK, noiseAttack);
    SetParam(PARAM_NOISE_DECAY, noiseDecay);
    SetParam(PARAM_MIX, mix);
}

float Sd8::Process() {
    float sig = (1 - parameters[PARAM_MIX].GetScaledValue()) * osc.Process() * oscEnv.Process();
    sig += parameters[PARAM_MIX].GetScaledValue() * noise.Process() * noiseEnv.Process();
    return velocity * sig * 10; 
}

void Sd8::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        osc.Reset();
        oscEnv.Trigger();
        noiseEnv.Trigger();
    }
}

float Sd8::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string Sd8::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_OSC_FREQUENCY: 
                return std::to_string((int)GetParam(param));// + "hz";
            case PARAM_OSC_ATTACK: 
            case PARAM_OSC_DECAY: 
            case PARAM_NOISE_ATTACK: 
            case PARAM_NOISE_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
            case PARAM_MIX:
                return std::to_string((int)(GetParam(param) * 100));
        }
    }
   return "";
 }

float Sd8::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_OSC_FREQUENCY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 5000, Parameter::EXPONENTIAL));
                osc.SetFreq(scaled);
                break;
            case PARAM_OSC_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                oscEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_OSC_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                oscEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_NOISE_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                noiseEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_NOISE_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                noiseEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_MIX: 
                scaled = parameters[param].Update(raw, Utility::LimitFloat(raw, 0, 1));
                break;
        }
    }

    return scaled;    
}

void Sd8::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void Sd8::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_OSC_FREQUENCY: 
                parameters[param].SetScaledValue(scaled);
                osc.SetFreq(scaled);
                break;
            case PARAM_OSC_ATTACK: 
                parameters[param].SetScaledValue(scaled);
                oscEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_OSC_DECAY: 
                parameters[param].SetScaledValue(scaled);
                oscEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_NOISE_ATTACK: 
                parameters[param].SetScaledValue(scaled);
                noiseEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_NOISE_DECAY: 
                parameters[param].SetScaledValue(scaled);
                noiseEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_MIX: 
                parameters[param].SetScaledValue(scaled);
                break;
        }
    }}
