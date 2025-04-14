#include "ClickSource.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;


void ClickSource::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 1500, 191, 116);
}

void ClickSource::Init(std::string slot, float sample_rate, float hpfFreq, float lpfFreq, float mod) {

    this->slot = slot;

    noise.Init();

    lpfEnv.Init(sample_rate);
    lpfEnv.SetMax(1);
    lpfEnv.SetMin(0);
    lpfEnv.SetCurve(-20);
    lpfEnv.SetTime(ADENV_SEG_ATTACK, 0.001);
    lpfEnv.SetTime(ADENV_SEG_DECAY, 0.09);

    hpf.Init(sample_rate);
    hpf.SetRes(0.2);
    hpf.SetDrive(.002);
    SetParam(PARAM_HPF, hpfFreq);

    lpf.Init(sample_rate);
    lpf.SetRes(0.2);
    lpf.SetDrive(.002);
    SetParam(PARAM_LPF, lpfFreq);
    SetParam(PARAM_LPF_MOD, mod);
}

float ClickSource::Process() {
    // noise goes through a hpf, then through an lpf modulated by the lpf env.
    // lpf env also controls amp of noise
    float lpfEnvSignal = lpfEnv.Process();
    lpf.SetFreq(1000 + parameters[PARAM_LPF_MOD].GetScaledValue() * lpfEnvSignal);
    hpf.Process(noise.Process());
    lpf.Process(hpf.High());
    signal = lpf.Low() * lpfEnvSignal;
    return signal;
}

void ClickSource::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        lpfEnv.Trigger();
    }
}

float ClickSource::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string ClickSource::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_LPF_MOD:
            case PARAM_HPF:
            case PARAM_LPF:
                return std::to_string((int)GetParam(param));
        }
    }
   return "";
 }

float ClickSource::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < ClickSource::PARAM_COUNT) {
        switch (param) {
            case PARAM_LPF_MOD:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0, 2000, Parameter::EXPONENTIAL));
                break;
            case PARAM_HPF:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 3000, Parameter::EXPONENTIAL));
                hpf.SetFreq(scaled);
                break;
            case PARAM_LPF:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 3000, Parameter::EXPONENTIAL));
                lpf.SetFreq(scaled);
                break;
        }
    }

    return scaled;    
}

void ClickSource::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void ClickSource::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_LPF_MOD:
                parameters[param].SetScaledValue(scaled);
                break;
            case PARAM_HPF:
                parameters[param].SetScaledValue(scaled);
                hpf.SetFreq(scaled);
                break;
            case PARAM_LPF:
                parameters[param].SetScaledValue(scaled);
                lpf.SetFreq(scaled);
                break;
        }
    }
}
