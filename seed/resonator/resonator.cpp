#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Resonator res;
Metro tick;

float f;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{		
		float sig = res.Process(f, .5f, .5f, .5f, tick.Process());
		out[0][i] = out[1][i] = sig;
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	//kMaxNumModes = 24
	res.Init(.015, 24);

	tick.Init(1.f, sample_rate);

	f = 440.f / sample_rate;

	hw.StartAudio(AudioCallback);
	while(1) {}
}