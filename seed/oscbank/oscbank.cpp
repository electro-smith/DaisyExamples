#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed      hw;
OscillatorBank osc[3];
AdEnv          env;
Metro          tick;


float freqs[] = {523.25f, 659.25f, 783.99f};

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(tick.Process())
        {
            env.Trigger();
        }

        float env_sig = env.Process();
        float osc_sig = 0.f;

        for(int i = 0; i < 3; i++)
        {
            osc[i].SetGain(env_sig);
            osc_sig += osc[i].Process();
        }

        out[0][i] = out[1][i] = osc_sig;
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    float amp[7] = {.15f, 0.15f, 0.0f, 0.33f, 0.0f, 0.15f, 0.15f};
    for(int i = 0; i < 3; i++)
    {
        osc[i].Init(sample_rate);
        osc[i].SetFreq(freqs[i] * .5f);
        osc[i].SetAmplitudes(amp);
    }

    tick.Init(5.f, sample_rate);

    // set adenv parameters
    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK, 0.01);
    env.SetTime(ADENV_SEG_DECAY, 0.35);
    env.SetMin(0.f);
    env.SetMax(.3f);
    env.SetCurve(0.f); // linear

    hw.StartAudio(AudioCallback);
    while(1) {}
}
