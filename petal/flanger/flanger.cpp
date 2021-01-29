#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

Flanger    flanger;
DaisyPetal hw;

bool effectOn = true;

void AudioCallback(float **in, float **out, size_t size)
{
    hw.ProcessAllControls();

    flanger.SetDelay(hw.knob[2].Process());
    flanger.SetFeedback(hw.knob[3].Process());
    flanger.SetLfoFreq(hw.knob[4].Process());
    flanger.SetLfoDepth(hw.knob[5].Process());

    effectOn ^= hw.switches[0].RisingEdge();

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = out[1][i] = effectOn ? flanger.Process(in[0][i]) : in[0][i];
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    flanger.Init(sample_rate);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.DelayMs(6);
        hw.ClearLeds();
        hw.SetFootswitchLed(hw.FOOTSWITCH_LED_1, effectOn);
        hw.UpdateLeds();
    }
}
