#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

SmoothRandomGenerator smooth;
Oscillator            osc;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        osc.SetFreq((fabsf(smooth.Process()) * 330.f) + 110.f);
        out[0][i] = out[1][i] = osc.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    smooth.Init(sample_rate);
    smooth.SetFreq(.75f);

    osc.Init(sample_rate);
    osc.SetFreq(440.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
