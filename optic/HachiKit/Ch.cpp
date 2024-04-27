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
    SetParam(PARAM_ATTACK, attack, false);
    SetParam(PARAM_DECAY, decay, false);
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
    return param < Ch::PARAM_COUNT ? params[param] : 0.0f;
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

float Ch::SetParam(uint8_t param, float value, bool scaled) {
    if (param < Ch::PARAM_COUNT) {
        if (scaled) {
            switch (param) {
                case PARAM_ATTACK: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    // update accordingly
                    return params[param];
                case PARAM_DECAY: 
                    params[param] = Utility::ScaleFloat(value, 0.01, 5, Parameter::EXPONENTIAL);
                    // update accordingly
                    return params[param];
            }
        } else {
            params[param] = value;
        }
    }

    return value;
}
