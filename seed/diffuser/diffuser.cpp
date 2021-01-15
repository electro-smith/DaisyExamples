#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Diffuser  diff;
uint16_t  buffer[8192];

Oscillator osc, lfo;

void AudioCallback(float **in, float **out, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        osc.SetFreq((lfo.Process() + 1.f) * 300.f);
        out[0][i] = out[1][i] = diff.Process(.5f, .5f, osc.Process());
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    diff.Init(buffer);

    lfo.Init(sample_rate);
    lfo.SetFreq(1.f);
    lfo.SetAmp(1.f);

    osc.Init(sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
