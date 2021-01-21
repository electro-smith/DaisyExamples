#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Chorus<>   ch; //defaults to 2 delay lines

bool  effectOn;
float wet;

void Controls()
{
    hw.ProcessAllControls();

    //knobs
    float spread = hw.knob[2].Process() * 1.f;

    ch.SetLfoFreq(hw.knob[3].Process() * 2.5f, 0);
    ch.SetLfoFreq(hw.knob[3].Value() * 2.5f + spread, 1);

    ch.SetLfoDepth(hw.knob[4].Process() * 6.5f, 0);
    ch.SetLfoDepth(hw.knob[4].Process() * 6.5f + 2.f * spread, 1);

    ch.SetDelay(hw.knob[5].Process() * 10.f, 0);
    ch.SetDelay(hw.knob[5].Value() * 10.f + 5.f * spread, 1);

    //footswitch
    effectOn ^= hw.switches[0].RisingEdge();

    //encoder
    wet += hw.encoder.Increment() * .05f;
    wet = fclamp(wet, 0.f, 1.f);

    wet = hw.encoder.RisingEdge() ? .5f : wet;
}

void AudioCallback(float **in, float **out, size_t size)
{
    Controls();

    for(size_t i = 0; i < size; i++)
    {
        float sig = effectOn ? ch.Process(in[0][i]) : in[0][i];
        out[0][i] = out[1][i] = sig * wet + in[0][i] * (1.f - wet);
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    ch.Init(sample_rate);

    effectOn = true;
    wet      = .5f;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.DelayMs(6);

        hw.ClearLeds();
        hw.SetFootswitchLed((DaisyPetal::FootswitchLed)0, (float)effectOn);

        int   wet_int  = (int)(wet * 8.f);
        float wet_frac = wet - wet_int;
        for(int i = 0; i < wet_int; i++)
        {
            hw.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);
        }

        if(wet_int < 8)
        {
            hw.SetRingLed((DaisyPetal::RingLed)wet_int, wet_frac, 0.f, 0.f);
        }

        hw.UpdateLeds();
    }
}
