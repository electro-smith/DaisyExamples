#include "Cy.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;


const float Cy::HPF_MAX = 12000;
const float Cy::HPF_MIN = 100;
const float Cy::LPF_MAX = 18000;
const float Cy::LPF_MIN = 100;


void Cy::Init(std::string slot, float sample_rate) {
    Init(slot, sample_rate, 0.001, 3.5, NULL, 1700, 2300);
}

void Cy::Init(std::string slot, float sample_rate, float attack, float decay, HhSource68 *source, float hpfCutoff, float lpfCutoff) {

    this->slot = slot;

    // env settings
    env.Init(sample_rate);
    env.SetMax(1);
    env.SetMin(0);
    env.SetCurve(-4);
    SetParam(PARAM_ATTACK, attack);
    SetParam(PARAM_DECAY, decay);

    hpf.Init(sample_rate);
    hpf.SetRes(0.2);
    hpf.SetDrive(.002);
    SetParam(PARAM_HPF, hpfCutoff);

    lpf.Init(sample_rate);
    lpf.SetRes(0.2);
    lpf.SetDrive(.002);
    SetParam(PARAM_LPF, lpfCutoff);

    this->source = source;

}

float Cy::Process() {
    if (source == NULL) {
        return 0.0f;
    }

    float sig = source->Signal() * env.Process();
    hpf.Process(sig);
    lpf.Process(hpf.High());
    return velocity * lpf.Low();;
}

void Cy::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        env.Trigger();
    }
}

float Cy::GetParam(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
            case PARAM_DECAY: 
            case PARAM_HPF:
            case PARAM_LPF:
                return parameters[param].GetScaledValue();
        }
    }
    return 0.0f;
}

std::string Cy::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
            case PARAM_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";
            case PARAM_HPF:
            case PARAM_LPF:
                return std::to_string((int)GetParam(param));
        }
    }
   return "";
 }

float Cy::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < Cy::PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                env.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 15, Parameter::EXPONENTIAL));
                env.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_HPF:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, HPF_MIN, HPF_MAX, Parameter::EXPONENTIAL));
                hpf.SetFreq(scaled);
                break;
            case PARAM_LPF:
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, LPF_MIN, LPF_MAX, Parameter::EXPONENTIAL));
                lpf.SetFreq(scaled);
                break;
        }
    }

    return scaled;    
}

void Cy::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
    source->ResetParams();
}

void Cy::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_ATTACK: 
                parameters[param].SetScaledValue(scaled);
                env.SetTime(ADENV_SEG_ATTACK, scaled);
                break;
            case PARAM_DECAY: 
                parameters[param].SetScaledValue(scaled);
                env.SetTime(ADENV_SEG_DECAY, scaled);
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
    }}
