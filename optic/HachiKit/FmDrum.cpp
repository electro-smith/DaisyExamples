#include "FmDrum.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void FmDrum::Init(float sample_rate) {
    Init(sample_rate, 220, 3, 2, 0.001, 0.5, -50);
}

void FmDrum::Init(float sample_rate, float frequency, float ratio, float modAmount, float attack, float decay, float curve) {

    // 2-osc fm object
    fm.Init(sample_rate);
    SetParam(PARAM_FREQUENCY, frequency, false);
    SetParam(PARAM_RATIO, ratio, false);
    SetParam(PARAM_MOD_AMT, modAmount, false);

    ampEnv.Init(sample_rate);
    ampEnv.SetMax(1);
    ampEnv.SetMin(0);
    SetParam(PARAM_ATTACK, attack, false);
    SetParam(PARAM_DECAY, decay, false);
    SetParam(PARAM_ENV_CURVE, curve, false);
}

float FmDrum::Process() {
    return velocity * fm.Process() * ampEnv.Process();
}

void FmDrum::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        ampEnv.Trigger();
    }
}

float FmDrum::GetParam(uint8_t param) {
    return param < FmDrum::PARAM_COUNT ? params[param] : 0.0f;
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

float FmDrum::SetParam(uint8_t param, float value, bool scaled) {
    if (param < FmDrum::PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                params[param] = value;
                if (scaled) params[param] = Utility::ScaleFloat(value, 20, 5000, Parameter::EXPONENTIAL);
                fm.SetFrequency(params[param]);
                return params[param];
            case PARAM_RATIO: 
                params[param] = value;
                if (scaled) params[param] = Utility::ScaleFloat(value, 0.125, 8, Parameter::EXPONENTIAL);
                fm.SetRatio(params[param]);
                return params[param];
            case PARAM_MOD_AMT: 
                params[param] = value;
                if (scaled) params[param] = Utility::ScaleFloat(value, 0, 5, Parameter::EXPONENTIAL);
                fm.SetIndex(params[param]);
                return params[param];
            case PARAM_ATTACK: 
                params[param] = value;
                if (scaled) params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                ampEnv.SetTime(ADENV_SEG_ATTACK, params[param]);
                return params[param];
            case PARAM_DECAY: 
                params[param] = value;
                if (scaled) params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                ampEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                return params[param];
            case PARAM_ENV_CURVE: 
                params[param] = value;
                if (scaled) params[param] = Utility::ScaleFloat(value, -100, 100, Parameter::LINEAR);
                ampEnv.SetTime(ADENV_SEG_DECAY, params[param]);
                return params[param];
        }
        return params[param];
    }

    return value;
}
