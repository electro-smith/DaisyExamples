#include "Ch.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;


void Ch::Init(float sample_rate) {
    // Init(sample_rate, 0.001, 0.5);
    Init(sample_rate, 0.001, 0.5, NULL);
}

// void Ch::Init(float sample_rate, float attack, float decay) {

//     // env settings
//     env.Init(sample_rate);
//     env.SetMax(1);
//     env.SetMin(0);
//     env.SetCurve(-20);
//     SetParam(PARAM_ATTACK, attack, false);
//     SetParam(PARAM_DECAY, decay, false);
// }

void Ch::Init(float sample_rate, float attack, float decay, ISource *source) {

    this->source = source;

    // env settings
    env.Init(sample_rate);
    env.SetMax(1);
    env.SetMin(0);
    env.SetCurve(-20);
    SetParam(PARAM_ATTACK, attack);
    SetParam(PARAM_DECAY, decay);
}

float Ch::Process() {
    if (source == NULL) {
        return 0.0f;
    }

    float sig = source->Signal() * env.Process();
    return velocity * sig;
}

void Ch::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        env.Trigger();
    }
}

float Ch::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string Ch::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
            case PARAM_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
        }
    }
   return "";
 }

float Ch::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < Ch::PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                break;
            case PARAM_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                break;
        }
    }

    return scaled;    
}

void Ch::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void Ch::SetParam(uint8_t param, float value) {
    if (param < PARAM_COUNT) {
        parameters[param].SetScaledValue(value);
    }
}
