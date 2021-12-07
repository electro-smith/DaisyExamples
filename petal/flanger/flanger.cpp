#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

Flanger    flanger;
DaisyPetal hw;

bool  effectOn = true;
float wet;
float deltarget, del;
float lfotarget, lfo;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    deltarget = hw.knob[2].Process();
    flanger.SetFeedback(hw.knob[3].Process());
    float val = hw.knob[4].Process();
    flanger.SetLfoFreq(val * val * 10.f);
    lfotarget = hw.knob[5].Process();

    effectOn ^= hw.switches[0].RisingEdge();

    //encoder
    wet += hw.encoder.Increment() * .05f;
    wet = fclamp(wet, 0.f, 1.f);

    wet = hw.encoder.RisingEdge() ? .9f : wet;

    for(size_t i = 0; i < size; i++)
    {
        fonepole(del, deltarget, .0001f); //smooth at audio rate
        flanger.SetDelay(del);

        fonepole(lfo, lfotarget, .0001f); //smooth at audio rate
        flanger.SetLfoDepth(lfo);

        out[0][i] = out[1][i] = in[0][i];

        if(effectOn)
        {
            float sig = flanger.Process(in[0][i]);
            out[0][i] = out[1][i] = sig * wet + in[0][i] * (1.f - wet);
        }
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    deltarget = del = 0.f;
    lfotarget = lfo = 0.f;
    flanger.Init(sample_rate);

    wet = .9f;

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
