#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
Dust       dust;
Oscillator lfo;
void       AudioCallback(AudioHandle::InputBuffer  in,
                         AudioHandle::OutputBuffer out,
                         size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        dust.SetDensity(fabsf(lfo.Process()));
        out[0][i] = out[1][i] = dust.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    dust.Init();
    lfo.Init(sample_rate);
    lfo.SetFreq(.1f);
    lfo.SetAmp(1.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
