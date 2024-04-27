#include "Blank.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void Blank::Init(float sample_rate) {
    Init(sample_rate, 220, 0.001, 0.5);
}

void Blank::Init(float sample_rate, float frequency, float attack, float decay) {

    // initialize audio objects
    SetParam(PARAM_FREQUENCY, frequency, false);
    SetParam(PARAM_ATTACK, attack, false);
    SetParam(PARAM_DECAY, decay, false);
}

float Blank::Process() {
    float sig = 0.0f;
    return velocity * sig;
}

void Blank::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        // trigger envelopes
    }
}

float Blank::GetParam(uint8_t param) {
    return param < Blank::PARAM_COUNT ? params[param] : 0.0f;
}

std::string Blank::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                return std::to_string((int)GetParam(param));// + "hz";
            case PARAM_ATTACK: 
            case PARAM_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
        }
    }
   return "";
 }

float Blank::SetParam(uint8_t param, float value, bool scaled) {
    if (param < Blank::PARAM_COUNT) {
        if (scaled) {
            switch (param) {
                case PARAM_FREQUENCY: 
                    params[param] = Utility::ScaleFloat(value, 20, 5000, Parameter::EXPONENTIAL);
                    // update accordingly
                    return params[param];
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
