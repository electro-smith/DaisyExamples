#include "DigiClap.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;

void DigiClap::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 0.012f, 1.0f, 3000.0f);
}

void DigiClap::Init(std::string slot, float sample_rate, float spread, float decay, float frequency) {

    this->slot = slot;

    clockedNoise.Init(sample_rate);
    SetParam(PARAM_FREQUENCY, frequency);
    clockedNoise.SetFreq(frequency);

    env.Init(sample_rate);
    env.SetMax(1);
    env.SetMin(0);
    env.SetCurve(-20);
    env.SetTime(ADENV_SEG_ATTACK, 0.001);
    SetParam(PARAM_DECAY, decay);
    SetParam(PARAM_SPREAD, spread);
}

float DigiClap::Process() {
    if (!env.IsRunning()) {
        repeat++;
        if (repeat == REPEATS) {
            env.SetTime(ADENV_SEG_DECAY, GetParam(PARAM_DECAY));
        }
        if (repeat <= REPEATS) {
            env.Trigger();
        }
    }
    return velocity * clockedNoise.Process() * env.Process();
}

void DigiClap::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        repeat = 0;
        env.SetTime(ADENV_SEG_DECAY, GetParam(PARAM_SPREAD));
        env.Trigger();
    }
}

float DigiClap::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string DigiClap::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_SPREAD: 
            case PARAM_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
            case PARAM_FREQUENCY: 
                return std::to_string((int)(GetParam(param)));
        }
    }
   return "";
 }

float DigiClap::UpdateParam(uint8_t param, float raw) {
    return SetParam(param, raw, true);
}

void DigiClap::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void DigiClap::SetParam(uint8_t param, float scaled) {
    SetParam(param, scaled, false);
}

float DigiClap::SetParam(uint8_t param, float value, bool isRaw) {
    float scaled = value;
    if (param < DigiClap::PARAM_COUNT) {
        switch (param) {
            case PARAM_SPREAD:
                if (isRaw) {
                    scaled = parameters[param].Update(value, Utility::ScaleFloat(value, 0.001, 0.050, Parameter::LINEAR));
                } else {
                    parameters[param].SetScaledValue(value);
                }
                return scaled;
            case PARAM_DECAY: 
                if (isRaw) {
                    scaled = parameters[param].Update(value, Utility::ScaleFloat(value, 0.001, 5, Parameter::EXPONENTIAL));
                } else {
                    parameters[param].SetScaledValue(value);
                }
                return scaled;
            case PARAM_FREQUENCY: 
                if (isRaw) {
                    scaled = parameters[param].Update(value, Utility::ScaleFloat(value, 20, 15000, Parameter::EXPONENTIAL));
                } else {
                    parameters[param].SetScaledValue(value);
                }
                clockedNoise.SetFreq(scaled);
                return scaled;
        }
    }

    return 0.0f;
}