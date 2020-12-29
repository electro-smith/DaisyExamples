#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed          hw;
GrainletOscillator grainlet;
Oscillator         lfo;

static void AudioCallback(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        float sig = fabsf(lfo.Process());
		grainlet.SetShape(sig);
		
        out[i] = out[i + 1] = grainlet.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    grainlet.Init(sample_rate);
	grainlet.SetCarrierFreq(80.f);
	grainlet.SetFormantFreq(2000.f);
	grainlet.SetBleed(1.f);

    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(.125f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
