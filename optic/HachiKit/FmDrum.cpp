#include "FmDrum.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void FmDrum::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 68, 3.3, 2.2, 0.001, 0.043, -50);
}

void FmDrum::Init(std::string slot, float sample_rate, float frequency, float ratio, float modAmount, float attack, float decay, float curve) {

    this->slot = slot;
    
    // 2-osc fm object
    fm.Init(sample_rate);
    SetParam(PARAM_FREQUENCY, frequency);
    SetParam(PARAM_RATIO, ratio);
    SetParam(PARAM_MOD_AMT, modAmount);

    ampEnv.Init(sample_rate);
    ampEnv.SetMax(1);
    ampEnv.SetMin(0);
    SetParam(PARAM_ATTACK, attack);
    SetParam(PARAM_DECAY, decay);
    SetParam(PARAM_ENV_CURVE, curve);
}

float FmDrum::Process() {
    return velocity * fm.Process() * ampEnv.Process();
}

void FmDrum::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        fm.Reset();
        ampEnv.Trigger();
    }
}

float FmDrum::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string FmDrum::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                return std::to_string((int)GetParam(param));// + "hz";
            case PARAM_RATIO: 
                return Utility::FloatToString2(GetParam(param));
            case PARAM_MOD_AMT: 
                return Utility::FloatToString2(GetParam(param));
            case PARAM_ATTACK: 
            case PARAM_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
            case PARAM_ENV_CURVE:
                return std::to_string((int)GetParam(param));
        }
    }
   return "";
 }

float FmDrum::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 5000, Parameter::EXPONENTIAL));
                fm.SetFrequency(scaled);
                break;
            case PARAM_RATIO: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.125, 8, Parameter::EXPONENTIAL));
                fm.SetRatio(scaled);
                break;
            case PARAM_MOD_AMT: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0, 5, Parameter::EXPONENTIAL));
                fm.SetIndex(scaled);
                break;
            case PARAM_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_ENV_CURVE: 
                scaled = raw;
                // TODO: set the curve
                break;
        }
    }

    return scaled;    
}

void FmDrum::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void FmDrum::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                parameters[param].SetScaledValue(scaled);
                fm.SetFrequency(scaled);
                break;
            case PARAM_RATIO: 
                parameters[param].SetScaledValue(scaled);
                fm.SetRatio(scaled);
                break;
            case PARAM_MOD_AMT: 
                parameters[param].SetScaledValue(scaled);
                fm.SetIndex(scaled);
                break;
            case PARAM_ATTACK: 
                parameters[param].SetScaledValue(scaled);
                ampEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_DECAY: 
                parameters[param].SetScaledValue(scaled);
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_ENV_CURVE: 
                parameters[param].SetScaledValue(scaled);
                // TODO: set the curve
                break;
        }
    }}
