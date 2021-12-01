#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

Oscillator osc;
Metro      tick;
AdEnv      env;
Tremolo    trem;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(tick.Process())
        {
            trem.SetFreq(10.f * rand() * kRandFrac);
            trem.SetDepth(.3f + rand() * kRandFrac);
            env.Trigger();
            osc.SetFreq(330.f * rand() * kRandFrac + 110.f);
        }

        out[0][i] = out[1][i] = trem.Process(osc.Process() * 1.f);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    trem.Init(sample_rate);

    osc.Init(sample_rate);
    osc.SetFreq(440.f);

    tick.Init(.5f, sample_rate);

    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK, 0.15);
    env.SetTime(ADENV_SEG_DECAY, 1.f);
    env.SetMin(0.0);
    env.SetMax(1.f);
    env.SetCurve(0); // linear
    env.Trigger();   //trigger right away

    hw.StartAudio(AudioCallback);
    while(1) {}
}
