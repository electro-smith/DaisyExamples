#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
HiHat<>   hihat;
Metro     tick;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        bool t = tick.Process();
        if(t)
        {
            hihat.SetDecay(random() / (float)RAND_MAX);
            hihat.SetSustain((random() / (float)RAND_MAX) > .8f);
            hihat.SetTone(random() / (float)RAND_MAX);
            hihat.SetNoisiness(random() / (float)RAND_MAX);
        }
        out[0][i] = out[1][i] = hihat.Process(t);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    hihat.Init(sample_rate);

    tick.Init(8.f, sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
