#include "Bd8.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void Bd8::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 78, 0.001, 4.0, 0.001, 0.1, 0.95);
}

void Bd8::Init(std::string slot, float sample_rate, float frequency, float ampAttack, float ampDecay, float pitchAttack, float pitchDecay, float modAmount) {

    this->slot = slot;

    // oscillator settings
    osc.Init(sample_rate);
    SetParam(PARAM_FREQUENCY, frequency);
    osc.SetWaveform(Oscillator::WAVE_SIN);

    // ampEnv settings
    ampEnv.Init(sample_rate);
    ampEnv.SetMax(1);
    ampEnv.SetMin(0);
    ampEnv.SetCurve(-100);
    SetParam(PARAM_AMP_ATTACK, ampAttack);
    SetParam(PARAM_AMP_DECAY, ampDecay);

    // pitchEnv settings
    pitchEnv.Init(sample_rate);
    pitchEnv.SetMax(1);
    pitchEnv.SetMin(0);
    pitchEnv.SetCurve(-100);
    SetParam(PARAM_PITCH_ATTACK, pitchAttack);
    SetParam(PARAM_PITCH_DECAY, pitchDecay);
    SetParam(PARAM_MOD_AMT, modAmount);
}

float Bd8::Process() {
    float psig = pitchEnv.Process();
    osc.SetFreq(parameters[PARAM_FREQUENCY].GetScaledValue() + parameters[PARAM_MOD_AMT].GetScaledValue() * psig);
    // osc.SetFreq(parameters[PARAM_FREQUENCY].GetScaledValue());
    return 2 * velocity * osc.Process() * ampEnv.Process();
}

void Bd8::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        osc.Reset();
        ampEnv.Trigger();
        pitchEnv.Trigger();
    }
}

float Bd8::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
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

float Bd8::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 5000, Parameter::EXPONENTIAL));
                break;
            case PARAM_AMP_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_AMP_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_PITCH_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                pitchEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_PITCH_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                pitchEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_MOD_AMT: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0, 2000, Parameter::EXPONENTIAL));
                break;
        }
    }

    return scaled;    
}

void Bd8::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void Bd8::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                parameters[param].SetScaledValue(scaled);
                break;
            case PARAM_AMP_ATTACK: 
                parameters[param].SetScaledValue(scaled);
                ampEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_AMP_DECAY: 
                parameters[param].SetScaledValue(scaled);
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_PITCH_ATTACK: 
                parameters[param].SetScaledValue(scaled);
                pitchEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_PITCH_DECAY: 
                parameters[param].SetScaledValue(scaled);
                pitchEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_MOD_AMT: 
                parameters[param].SetScaledValue(scaled);
                break;
        }
    }

}

