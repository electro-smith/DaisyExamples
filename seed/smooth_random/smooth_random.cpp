#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;

SmoothRandomGenerator smooth;
Oscillator osc;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		osc.SetFreq(smooth.Process() * 440.f);
		out[0][i] = out[1][i] = osc.Process();
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	
	smooth.Init(sample_rate);
	
	osc.Init(sample_rate);
	osc.SetFreq(440.f);

	hw.StartAudio(AudioCallback);
	while(1) {}
}
