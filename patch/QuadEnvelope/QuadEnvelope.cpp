#include "daisy_patch.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

int curveTimeMode;

struct envStruct
{
    AdEnv     env;
    Parameter attackParam;
    Parameter decayParam;
    Parameter curveParam;
    float     envSig;
};

envStruct envelopes[2];
void      ProcessControls();

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    //Process control inputs
    ProcessControls();

    for(size_t i = 0; i < size; i++)
    {
        //Get the next envelope samples
        envelopes[0].envSig = envelopes[0].env.Process();
        envelopes[1].envSig = envelopes[1].env.Process();

        for(size_t chn = 0; chn < 2; chn++)
        {
            //The envelopes effect the outputs in pairs
            out[chn * 2][i]     = in[chn * 2][i] * envelopes[chn].envSig;
            out[chn * 2 + 1][i] = in[chn * 2 + 1][i] * envelopes[chn].envSig;
        }
    }
}

void InitEnvelopes(float samplerate)
{
    for(int i = 0; i < 2; i++)
    {
        //envelope values and Init
        envelopes[i].env.Init(samplerate);
        envelopes[i].env.SetMax(1);
        envelopes[i].env.SetMin(0);
        envelopes[i].env.SetCurve(0);
    }

    //envelope parameters (control inputs)
    envelopes[0].attackParam.Init(
        hw.controls[0], .01, 2, Parameter::EXPONENTIAL);
    envelopes[0].decayParam.Init(
        hw.controls[1], .01, 2, Parameter::EXPONENTIAL);
    envelopes[0].curveParam.Init(hw.controls[0], -10, 10, Parameter::LINEAR);

    envelopes[1].attackParam.Init(
        hw.controls[2], .01, 2, Parameter::EXPONENTIAL);
    envelopes[1].decayParam.Init(
        hw.controls[3], .01, 2, Parameter::EXPONENTIAL);
    envelopes[1].curveParam.Init(hw.controls[2], -10, 10, Parameter::LINEAR);
}

void UpdateOled();

int main(void)
{
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    InitEnvelopes(samplerate);

    curveTimeMode = 0;

    UpdateOled();

    // Start the ADC and Audio Peripherals on the Hardware
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;)
    {
        //Send the latest envelope values to the CV outs
        hw.seed.dac.WriteValue(DacHandle::Channel::ONE,
                               envelopes[0].envSig * 4095);
        hw.seed.dac.WriteValue(DacHandle::Channel::TWO,
                               envelopes[1].envSig * 4095);
        hw.DelayMs(1);
    }
}

void UpdateOled()
{
    hw.display.Fill(false);

    hw.display.SetCursor(0, 0);
    std::string str  = "env1";
    char*       cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.SetCursor(70, 0);
    str = "env2";
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.SetCursor(0, 50);
    //curve or attack/decay mode
    if(curveTimeMode)
    {
        str = "curve";
    }
    else
    {
        str = "attack/decay";
    }
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.Update();
}

void ProcessEncoder()
{
    int edge = hw.encoder.RisingEdge();
    curveTimeMode += edge;
    curveTimeMode = curveTimeMode % 2;

    if(edge != 0)
    {
        UpdateOled();
    }
}

void ProcessKnobs()
{
    for(int i = 0; i < 2; i++)
    {
        if(curveTimeMode == 0)
        {
            envelopes[i].env.SetTime(ADENV_SEG_ATTACK,
                                     envelopes[i].attackParam.Process());
            envelopes[i].env.SetTime(ADENV_SEG_DECAY,
                                     envelopes[i].decayParam.Process());
        }
        else
        {
            envelopes[i].env.SetCurve(envelopes[i].curveParam.Process());
        }
    }
}

void ProcessGates()
{
    for(int i = 0; i < 2; i++)
    {
        if(hw.gate_input[i].Trig())
        {
            envelopes[i].env.Trigger();
        }
    }
}

void ProcessControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    ProcessEncoder();
    ProcessKnobs();
    ProcessGates();
}
