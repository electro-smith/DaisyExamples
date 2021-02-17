#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Diffuser  diff;

Oscillator osc, lfo0, lfo1;
Metro      tick;

void AudioCallback(float **in, float **out, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        float sig0 = fabsf(lfo0.Process());
        float sig1 = fabsf(lfo1.Process());

        diff.SetTime(sig0);
        diff.SetAmount(sig1);

        out[0][i] = out[1][i] = diff.Process(tick.Process());
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    diff.Init();

    lfo0.Init(sample_rate);
    lfo0.SetFreq(.5f);
    lfo0.SetAmp(1.f);

    lfo1.Init(sample_rate);
    lfo1.SetFreq(.2f);
    lfo1.SetAmp(1.f);

    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SIN);
    osc.SetAmp(1.f);

    tick.Init(1.f, sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
