#include "Tom.h"
#include "Utility.h"

using namespace daisy;
using namespace daisysp;


void Tom::Init(float sample_rate) {
    Init(sample_rate, 80);
}

void Tom::Init(float sample_rate, float frequency) {

    osc.Init(sample_rate);
    SetParam(PARAM_FREQUENCY, frequency);
    osc.SetWaveform(Oscillator::WAVE_POLYBLEP_TRI);

    vibratoOsc.Init(sample_rate);
    vibratoOsc.SetWaveform(Oscillator::WAVE_SIN);
    vibratoOsc.SetFreq(20);

    noise.Init();

    ampEnv.Init(sample_rate);
    ampEnv.SetMax(1);
    ampEnv.SetMin(0);
    ampEnv.SetCurve(-10);
    ampEnv.SetTime(ADENV_SEG_ATTACK, 0.001);
    SetParam(PARAM_AMP_DECAY, 1.1);
    SetParam(PARAM_PITCH_MOD, 60);

    lpfEnv.Init(sample_rate);
    lpfEnv.SetMax(1);
    lpfEnv.SetMin(0);
    lpfEnv.SetCurve(-20);
    lpfEnv.SetTime(ADENV_SEG_ATTACK, 0.001);
    lpfEnv.SetTime(ADENV_SEG_DECAY, 0.09);

    pitchEnv.Init(sample_rate);
    pitchEnv.SetMax(1);
    pitchEnv.SetMin(0);
    pitchEnv.SetCurve(-20);
    pitchEnv.SetTime(ADENV_SEG_ATTACK, 0.001);
    pitchEnv.SetTime(ADENV_SEG_DECAY, 0.25);

    hpf.Init(sample_rate);
    hpf.SetRes(0.2);
    hpf.SetDrive(.002);
    SetParam(PARAM_HPF, 1500);

    lpf.Init(sample_rate);
    lpf.SetRes(0.2);
    lpf.SetDrive(.002);
    SetParam(PARAM_LPF, 191);
    SetParam(PARAM_LPF_MOD, 116);


}

float Tom::Process() {

    // noise goes through a hpf, then through an lpf modulated by the lpf env.
    // lpf env also controls amp of noise
    float lpfEnvSignal = lpfEnv.Process();
    lpf.SetFreq(1000 + parameters[PARAM_LPF_MOD].GetScaledValue() * lpfEnvSignal);
    hpf.Process(noise.Process());
    lpf.Process(hpf.High());
    float noiseSignal = lpf.Low() * lpfEnvSignal;
    
    // sine osc freq is modulated by pitch env, amp by amp env
    float pitchEnvSignal = pitchEnv.Process();
    float ampEnvSignal = ampEnv.Process();
    // TODO: including vibrato seems to crash the Daisy if there are 3 toms
    // float vibratoSignal = vibratoOsc.Process();
    // osc.SetFreq(parameters[PARAM_FREQUENCY].GetScaledValue() + parameters[PARAM_PITCH_MOD].GetScaledValue() * pitchEnvSignal + 3 * vibratoSignal);
    osc.SetFreq(parameters[PARAM_FREQUENCY].GetScaledValue() + parameters[PARAM_PITCH_MOD].GetScaledValue() * pitchEnvSignal);
    float oscSignal = osc.Process() * ampEnvSignal;

    // TODO: figure out why this wasn't working
    // // // apply velocity scaling more strong to noise than osc
    // // float signal = noiseSignal * velocity + oscSignal * (0.4 + velocity * 0.6);
    // // return signal;
    return (noiseSignal + oscSignal) * velocity; 
}

void Tom::Trigger(float velocity) {
    this->velocity = Utility::Limit(velocity);
    if (this->velocity > 0) {
        ampEnv.Trigger();
        lpfEnv.Trigger();
        pitchEnv.Trigger();
        osc.Reset();
        vibratoOsc.Reset();
    }
}

float Tom::GetParam(uint8_t param) {
    return param < PARAM_COUNT ? parameters[param].GetScaledValue() : 0.0f;
}

std::string Tom::GetParamString(uint8_t param) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
            case PARAM_PITCH_MOD:
            case PARAM_LPF_MOD:
            case PARAM_HPF:
            case PARAM_LPF:
                return std::to_string((int)GetParam(param));
            case PARAM_AMP_DECAY: 
                return std::to_string((int)(GetParam(param) * 1000));// + "ms";                
        }
    }
   return "";
 }

float Tom::UpdateParam(uint8_t param, float raw) {
    float scaled = raw;
    if (param < Tom::PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 20, 1000, Parameter::EXPONENTIAL));
                break;
            case PARAM_AMP_DECAY: 
                scaled = parameters[param].Update(raw, Utility::ScaleFloat(raw, 0.01, 5, Parameter::EXPONENTIAL));
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_PITCH_MOD:
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

void Tom::ResetParams() {
    for (u8 param = 0; param < PARAM_COUNT; param++) {
        parameters[param].Reset();
    }
}

void Tom::SetParam(uint8_t param, float scaled) {
    if (param < PARAM_COUNT) {
        switch (param) {
            case PARAM_FREQUENCY: 
                parameters[param].SetScaledValue(scaled);
                break;
            case PARAM_AMP_DECAY: 
                parameters[param].SetScaledValue(scaled);
                ampEnv.SetTime(ADENV_SEG_DECAY, scaled);
                break;
            case PARAM_PITCH_MOD:
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
