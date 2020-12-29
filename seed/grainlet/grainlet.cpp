#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed          hw;
GrainletOscillator grainlet;
Oscillator         lfo;

float       f0, f1;
static void AudioCallback(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        float sig = (lfo.Process() + 1.f) * 0.5;
        out[i] = out[i + 1] = grainlet.Process(f0, f1, sig, 1.f);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    f0 = 80.f / sample_rate;
    f1 = 2000.f / sample_rate;
    grainlet.Init();

    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(.5f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
