#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Resonator res;
Metro tick;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{		
		float sig = res.Process(440.f, .5f, .5f, .5f, tick.Process());
		out[0][i] = out[1][i] = sig;
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	//kMaxNumModes = 24
	res.Init(.015, 24, sample_rate);

	tick.Init(1.f, sample_rate);

	hw.StartAudio(AudioCallback);
	while(1) {}
}