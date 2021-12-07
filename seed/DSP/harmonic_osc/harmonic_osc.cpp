#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed              hw;
static HarmonicOscillator<16> harm;
static Oscillator             lfo;
static AdEnv                  env;

float scale[] = {55.f, 65.41f, 73.42f, 82.41f, 98.f, 110.f};
int   note    = 0;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        //retrig env on EOC and go to next note
        if(!env.GetCurrentSegment())
        {
            env.Trigger();
            harm.SetFreq(scale[note]);
            note = (note + 1) % 6;
        }

        //calculate the new amplitudes based on env value
        float center = env.Process();
        for(int i = 0; i < 16; i++)
        {
            float dist = fabsf(center - (float)i) + 1.f;
            harm.SetSingleAmp(1.f / ((float)dist * 10.f), i);
        }

        out[i] = out[i + 1] = harm.Process();
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();

    //init harmonic oscillator
    harm.Init(sample_rate);
    harm.SetFirstHarmIdx(1);

    //init envelope
    env.Init(sample_rate);
    env.SetTime(ADENV_SEG_ATTACK, 0.05f);
    env.SetTime(ADENV_SEG_DECAY, 0.35f);
    env.SetMin(0.0);
    env.SetMax(15.f);
    env.SetCurve(0); // linear

    // start callback
    hw.StartAudio(AudioCallback);

    while(1) {}
}
