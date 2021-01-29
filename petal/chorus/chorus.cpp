#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Chorus<4>  ch; //defaults to 2 delay lines

bool  effectOn;
float wet;

void Controls()
{
    hw.ProcessAllControls();

    //knobs
    float spread = hw.knob[2].Process();

    ch.SetPan(.5f - .25f * spread, 0);
    ch.SetPan(.4f - .4f * spread, 1);
    ch.SetPan(.5f + .25f * spread, 2);
    ch.SetPan(.6f + .4f * spread, 3);

    ch.SetLfoFreqAll(hw.knob[3].Process());
    ch.SetLfoDepthAll(hw.knob[4].Process());

    float delay = hw.knob[5].Process();

    ch.SetDelay(delay, 0);
    ch.SetDelay(delay * .5f, 1);
    ch.SetDelay(delay, 2);
    ch.SetDelay(delay * .5f, 3);

    //footswitch
    effectOn ^= hw.switches[0].RisingEdge();

    //encoder
    wet += hw.encoder.Increment() * .05f;
    wet = fclamp(wet, 0.f, 1.f);

    wet = hw.encoder.RisingEdge() ? .9f : wet;
}

void AudioCallback(float **in, float **out, size_t size)
{
    Controls();

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];

        if(effectOn)
        {
            ch.Process(in[0][i], out[0][i], out[1][i]);
            out[0][i] = out[0][i] * wet + in[0][i] * (1.f - wet);
            out[1][i] = out[1][i] * wet + in[1][i] * (1.f - wet);
        }
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    ch.Init(sample_rate);

    effectOn = true;
    wet      = .9f;

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
