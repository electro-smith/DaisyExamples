#include "Sd8.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

const std::string Sd8::paramNames[] = { "Freq",  "oDec", "nDec", "Mix", "oAtt", "nAtt" };


void Sd8::Init(float sample_rate) {
    Init(sample_rate, 110, 0.001, 0.5, 0.001, 0.5, 0.5);
}

void Sd8::Init(float sample_rate, float oscFrequency, float oscAttack, float oscDecay, float noiseAttack, float noiseDecay, float mix) {

    // oscillator settings
    osc.Init(sample_rate);
    SetParam(PARAM_OSC_FREQUENCY, oscFrequency, false);
    osc.SetWaveform(Oscillator::WAVE_SIN);

    // oscEnv settings
    oscEnv.Init(sample_rate);
    oscEnv.SetMax(1);
    oscEnv.SetMin(0);
    oscEnv.SetCurve(-20);
    SetParam(PARAM_OSC_ATTACK, oscAttack, false);
    SetParam(PARAM_OSC_DECAY, oscDecay, false);

    // noise
    noise.Init();

    // noiseEnv settings
    noiseEnv.Init(sample_rate);
    noiseEnv.SetMax(1);
    noiseEnv.SetMin(0);
    noiseEnv.SetCurve(-20);
    SetParam(PARAM_NOISE_ATTACK, noiseAttack, false);
    SetParam(PARAM_NOISE_DECAY, noiseDecay, false);
    SetParam(PARAM_MIX, mix, false);
}

float Sd8::Process() {
    float sig = (1 - params[PARAM_MIX]) * osc.Process() * oscEnv.Process();
    sig += params[PARAM_MIX] * noise.Process() * noiseEnv.Process();
    return velocity * sig; // / 2;
}

void Sd8::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        oscEnv.Trigger();
        noiseEnv.Trigger();
    }
}

float Sd8::GetParam(uint8_t param) {
    return param < Sd8::PARAM_COUNT ? params[param] : 0.0f;
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

float Sd8::SetParam(uint8_t param, float value, bool scaled) {
    if (param < Sd8::PARAM_COUNT) {
        if (scaled) {
            switch (param) {
                case PARAM_OSC_FREQUENCY: 
                    params[param] = Utility::ScaleFloat(value, 20, 5000, Parameter::EXPONENTIAL);
                    osc.SetFreq(params[param]);
                    return params[param];
                case PARAM_OSC_ATTACK: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    oscEnv.SetTime(ADENV_SEG_ATTACK, params[param]);
                    return params[param];
                case PARAM_OSC_DECAY: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    oscEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                    return params[param];
                case PARAM_NOISE_ATTACK: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    noiseEnv.SetTime(ADENV_SEG_ATTACK, params[param]);
                    return params[param];
                case PARAM_NOISE_DECAY: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    noiseEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                    return params[param];
                case PARAM_MIX: 
                    params[param] = Utility::LimitFloat(value, 0, 1);
                    return params[param];
            }
        } else {
            params[param] = value;
        }
    }

    return value;
}
