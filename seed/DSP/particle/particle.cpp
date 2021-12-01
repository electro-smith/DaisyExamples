#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
Particle   particle;
Oscillator lfo;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        particle.SetDensity(fabsf(lfo.Process()));
        out[0][i] = out[1][i] = particle.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    lfo.Init(sample_rate);
    lfo.SetAmp(.5f);
    lfo.SetFreq(.1f);

    particle.Init(sample_rate);
    particle.SetSpread(2.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
