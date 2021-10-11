#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed      hw;
AnalogBassDrum bd;
Metro          tick;


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        bool t = tick.Process();
        if(t)
        {
            bd.SetTone(.7f * random() / (float)RAND_MAX);
            bd.SetDecay(random() / (float)RAND_MAX);
            bd.SetSelfFmAmount(random() / (float)RAND_MAX);
        }

        out[0][i] = out[1][i] = bd.Process(t);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    bd.Init(sample_rate);
    bd.SetFreq(50.f);

    tick.Init(2.f, sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
