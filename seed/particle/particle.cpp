#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Particle particle;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		particle.Sync();
		out[0][i] = out[1][i] = particle.Process();
	}	
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	
	particle.Init(sample_rate);

	particle.SetFreq(1000.f);

    particle.SetResonance(1.f);

    particle.SetRandomFreq(1000.f);

    particle.SetDensity(1.f);

    particle.SetGain(1.f);

    particle.SetSpread(1.f);
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}
