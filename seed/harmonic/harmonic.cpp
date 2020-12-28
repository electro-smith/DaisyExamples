#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed          seed;
static HarmonicOscillator harm;

static void AudioCallback(float *in, float *out, size_t size)
{


    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = out[i + 1] = harm.Process();
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    seed.Configure();
    seed.Init();
    sample_rate = seed.AudioSampleRate();

    harm.Init(sample_rate, 16);
	harm.SetFreq(10.f);
	harm.SetFirstHarmIdx(8);

    float amplitudes[16];
    for(int i = 0; i < 16; i++)
    {
        amplitudes[i] = 0.f;
    }
    amplitudes[15] = 1.f;
	harm.SetAmplitudes(amplitudes);

    // start callback
    seed.StartAudio(AudioCallback);

    while(1) {}
}
