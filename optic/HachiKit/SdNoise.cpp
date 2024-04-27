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
    SetParam(PARAM_ATTACK, attack, false);
    SetParam(PARAM_DECAY, decay, false);
    SetParam(PARAM_CURVE, curve, false);
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
    return param < PARAM_COUNT ? params[param] : 0.0f;
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

float SdNoise::SetParam(uint8_t param, float value, bool scaled) {
    if (param < SdNoise::PARAM_COUNT) {
        if (scaled) {
            switch (param) {
                case PARAM_ATTACK: 
                    params[param] = Utility::ScaleFloat(value, 0.001, 5, Parameter::EXPONENTIAL);
                    ampEnv.SetTime(ADENV_SEG_ATTACK, params[param]);
                    return params[param];
                case PARAM_DECAY: 
                    params[param] = Utility::ScaleFloat(value, 0.001, 5, Parameter::EXPONENTIAL);
                    ampEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                    return params[param];
                case PARAM_CURVE: 
                    params[param] = Utility::ScaleFloat(value, -100, 100, Parameter::LINEAR);
                    ampEnv.SetCurve(params[param]);
                    return params[param];
            }
        } else {
            params[param] = value;
            return value;
        }
    }

    return 0.0f;
}
