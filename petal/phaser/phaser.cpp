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
int   numstages;

void Controls()
{
    hw.ProcessAllControls();

    wet = hw.knob[0].Process();

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
    numstages += hw.encoder.Increment();
    numstages = DSY_CLAMP(numstages, 1, 8);
    numstages = hw.encoder.RisingEdge() ? 4 : numstages;
    phaser.SetPoles(numstages);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
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
            out[0][i] = out[1][i]
                = phaser.Process(in[0][i]) * wet + in[0][i] * (1.f - wet);
        }
    }
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    phaser.Init(sample_rate);

    effectOn   = true;
    wet        = .9f;
    freqtarget = freq = 0.f;
    lfotarget = lfo = 0.f;
    numstages       = 4;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.DelayMs(6);

        hw.ClearLeds();
        hw.SetFootswitchLed((DaisyPetal::FootswitchLed)0, (float)effectOn);

        for(int i = 0; i < numstages; i++)
        {
            hw.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);
        }

        hw.UpdateLeds();
    }
}