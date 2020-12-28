#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed          seed;
static HarmonicOscillator harm;
float                     f;

static void AudioCallback(float *in, float *out, size_t size)
{
    float amplitudes[16];
    for(int i = 0; i < 16; i++)
    {
        amplitudes[i] = 0.f;
    }
    amplitudes[15] = 1.f;

    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = out[i + 1] = harm.Process(8, f, amplitudes);
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    seed.Configure();
    seed.Init();
    sample_rate = seed.AudioSampleRate();
    f           = 10.f / sample_rate;

    harm.Init(16);

    // start callback
    seed.StartAudio(AudioCallback);

    while(1) {}
}
