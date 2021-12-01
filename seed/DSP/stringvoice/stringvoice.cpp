#include "daisy_seed.h"
#include "daisysp.h"
#include <stdlib.h>

using namespace daisy;
using namespace daisysp;

DaisySeed   hw;
StringVoice str;
Oscillator  lfo;
Metro       tick;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        bool t = tick.Process();
        if(t)
        {
            float r = rand() * kRandFrac;
            str.SetFreq(r * 300.f + 100.f);
            str.SetDamping(.5f + r);
            str.SetSustain(r > 0.8f);
            tick.SetFreq(5.f * r + 1.f);
        }
        float sig = fabsf(lfo.Process());
        str.SetStructure(sig);
        str.SetBrightness(.1f + sig * .2f);

        out[0][i] = out[1][i] = str.Process(t);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    str.Init(sample_rate);
    str.SetAccent(1.f);

    tick.Init(1.f, sample_rate);

    lfo.Init(sample_rate);
    lfo.SetFreq(.2f);
    lfo.SetAmp(1.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}