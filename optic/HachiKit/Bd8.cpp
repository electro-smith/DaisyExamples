#include "Bd8.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void Bd8::Init(float sample_rate) {
    Init(sample_rate, 60, 0.001, 0.5, 0.001, 0.2, 0.1);
}

void Bd8::Init(float sample_rate, float frequency, float ampAttack, float ampDecay, float pitchAttack, float pitchDecay, float modAmount) {

    // oscillator settings
    osc.Init(sample_rate);
    SetParam(PARAM_FREQUENCY, frequency, false);
    osc.SetWaveform(Oscillator::WAVE_SIN);

    // ampEnv settings
    ampEnv.Init(sample_rate);
    ampEnv.SetMax(1);
    ampEnv.SetMin(0);
    ampEnv.SetCurve(-100);
    SetParam(PARAM_AMP_ATTACK, ampAttack, false);
    SetParam(PARAM_AMP_DECAY, ampDecay, false);

    // pitchEnv settings
    pitchEnv.Init(sample_rate);
    pitchEnv.SetMax(1);
    pitchEnv.SetMin(0);
    pitchEnv.SetCurve(-100);
    SetParam(PARAM_PITCH_ATTACK, pitchAttack, false);
    SetParam(PARAM_PITCH_DECAY, pitchDecay, false);
    SetParam(PARAM_MOD_AMT, modAmount, false);
}

float Bd8::Process() {
    float psig = pitchEnv.Process();
    osc.SetFreq(params[PARAM_FREQUENCY] + params[PARAM_MOD_AMT] * psig);
    return velocity * osc.Process() * ampEnv.Process();
}

void Bd8::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        ampEnv.Trigger();
        pitchEnv.Trigger();
    }
}

float Bd8::GetParam(uint8_t param) {
    return param < Bd8::PARAM_COUNT ? params[param] : 0.0f;
}

std::string Bd8::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
            case PARAM_MOD_AMT: 
                return std::to_string((int)GetParam(param));// + "hz";
            case PARAM_AMP_ATTACK: 
            case PARAM_AMP_DECAY: 
            case PARAM_PITCH_ATTACK: 
            case PARAM_PITCH_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
        }
    }
   return "";
 }

float Bd8::SetParam(uint8_t param, float value, bool scaled) {
    if (param < Bd8::PARAM_COUNT) {
        if (scaled) {
            switch (param) {
                case PARAM_FREQUENCY: 
                    params[param] = Utility::ScaleFloat(value, 20, 5000, Parameter::EXPONENTIAL);
                    return params[param];
                case PARAM_AMP_ATTACK: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    ampEnv.SetTime(ADENV_SEG_ATTACK, params[param]);
                    return params[param];
                case PARAM_AMP_DECAY: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    ampEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                    return params[param];
                case PARAM_PITCH_ATTACK: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    pitchEnv.SetTime(ADENV_SEG_ATTACK, params[param]);
                    return params[param];
                case PARAM_PITCH_DECAY: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    pitchEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                    return params[param];
                case PARAM_MOD_AMT: 
                    params[param] = Utility::ScaleFloat(value, 0, 2000, Parameter::EXPONENTIAL);
                    return params[param];
            }
        } else {
            params[param] = value;
        }
    }

    return value;
}
