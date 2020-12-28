#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed          seed;
static HarmonicOscillator harm;
static Oscillator         lfo;

static void AudioCallback(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        float center = fabsf(lfo.Process()) * 15.f;
        float amplitudes[16];
        for(int i = 0; i < 16; i++)
        {
            float dist    = fabsf(center - (float)i) + 1.f;
            amplitudes[i] = 1.f / ((float)dist * 10.f);
        }
        harm.SetAmplitudes(amplitudes);

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
    harm.SetFreq(55.f);
    harm.SetFirstHarmIdx(1);

    lfo.Init(sample_rate);
    lfo.SetFreq(.05f);
    lfo.SetAmp(1.f);

    // start callback
    seed.StartAudio(AudioCallback);

    while(1) {}
}
