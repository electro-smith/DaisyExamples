#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Resonator res;
Metro     tick;

float frac;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float t_sig = tick.Process();
        if(t_sig)
        {
            res.SetFreq(rand() * frac * 770.f + 110.f); //110 - 880
            res.SetStructure(rand() * frac);
            res.SetBrightness(rand() * frac * .7f);
            res.SetDamping(rand() * frac * .7f);

            tick.SetFreq(rand() * frac * 7.f + 1);
        }
        float sig = res.Process(t_sig);
        out[0][i] = out[1][i] = sig;
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    res.Init(.015, 24, sample_rate);
    res.SetStructure(-7.f);

    tick.Init(1.f, sample_rate);

    frac = 1.f / RAND_MAX;

    hw.StartAudio(AudioCallback);
    while(1) {}
}