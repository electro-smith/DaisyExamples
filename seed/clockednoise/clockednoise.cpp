#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed    hw;
ClockedNoise noise;
Oscillator   lfo;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        noise.SetFreq(fabsf(lfo.Process()));
        out[0][i] = out[1][i] = noise.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    noise.Init(sample_rate);

    lfo.Init(sample_rate);
    lfo.SetAmp(sample_rate / 3.f);
    lfo.SetFreq(.125f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
