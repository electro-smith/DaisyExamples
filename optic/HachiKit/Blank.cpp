#include "Blank.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void Blank::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 220, 0.001, 0.5);
}

void Blank::Init(std::string slot, float sample_rate, float frequency, float attack, float decay) {

    this->slot = slot;
    
    // initialize audio objects
    SetParam(PARAM_FREQUENCY, frequency);
    SetParam(PARAM_ATTACK, attack);
    SetParam(PARAM_DECAY, decay);
}

float Blank::Process() {
    return 0.0f;
}

void Blank::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        // trigger envelopes
    }
}

float Blank::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
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

float Blank::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < Blank::PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 5000, Parameter::EXPONENTIAL));
                break;
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

void Blank::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void Blank::SetParam(uint8_t param, float value) {
    if (param < PARAM_COUNT) {
        parameters[param].SetScaledValue(value);
    }
}
