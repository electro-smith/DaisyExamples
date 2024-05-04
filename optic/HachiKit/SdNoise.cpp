#include "SdNoise.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void SdNoise::Init(float sample_rate) {
    Init(sample_rate, 0.01, 0.1, -100.0f);
}

void SdNoise::Init(float sample_rate, float attack, float decay, float curve) {
    noise.Init();
    ampEnv.Init(sample_rate);
    ampEnv.SetMax(1);
    ampEnv.SetMin(0);
    SetParam(PARAM_ATTACK, attack);
    SetParam(PARAM_DECAY, decay);
    SetParam(PARAM_CURVE, curve);
}

float SdNoise::Process() {
    return velocity * noise.Process() * ampEnv.Process();
}

void SdNoise::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        ampEnv.Trigger();
    }
}

float SdNoise::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string SdNoise::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
            case PARAM_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
            case PARAM_CURVE: 
                return std::to_string((int)GetParam(param));
        }
    }
   return "";
 }

float SdNoise::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < SdNoise::PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_CURVE: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, -100, 100, Parameter::LINEAR));
                ampEnv.SetCurve(scaled);
                break;
        }
    }

    return scaled;    
}

void SdNoise::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void SdNoise::SetParam(uint8_t param, float value) {
    if (param < PARAM_COUNT) {
        parameters[param].SetScaledValue(value);
    }
}
