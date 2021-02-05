#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Phaser     phaser;

bool  effectOn;
float wet;

float freqtarget, freq;
float lfotarget, lfo;

void Controls()
{
    hw.ProcessAllControls();

    //knobs
    float k = hw.knob[2].Process();
    phaser.SetLfoFreq(k * k * 20.f);
    lfo  = hw.knob[3].Process();
    k    = hw.knob[4].Process();
    freq = k * k * 7000; //0 - 10 kHz, square curve
    phaser.SetFeedback(hw.knob[5].Process());

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
        fonepole(freq, freqtarget, .0001f); //smooth at audio rate
        phaser.SetFreq(freq);

        fonepole(lfo, lfotarget, .0001f); //smooth at audio rate
        phaser.SetLfoDepth(lfo);

        out[0][i] = in[0][i];
        out[1][i] = in[0][i];

        if(effectOn)
        {
            out[0][i] = out[1][i] = phaser.Process(in[0][i]);
        }
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    phaser.Init(sample_rate);

    effectOn   = true;
    wet        = .9f;
    freqtarget = freq = 0.f;
    lfotarget = lfo = 0.f;

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