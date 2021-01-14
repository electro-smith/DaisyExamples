#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
AnalogBassDrum bd;
Metro tick;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = out[1][i] = bd.Process(false, tick.Process(), .1f, .002f, .1f, -.5f, .7f, .8f);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	
	bd.Init(sample_rate);
	tick.Init(1.f, sample_rate);
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}
