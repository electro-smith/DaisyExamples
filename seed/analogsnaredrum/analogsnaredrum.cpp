#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
AnalogSnareDrum sd;
Metro tick;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = out[1][i] = sd.Process(false, tick.Process(), .1f, .003f, .7f, .4f, .4f);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	
	tick.Init(2.f, sample_rate);

	sd.Init(sample_rate);
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}
