#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
HiHat<> hihat;
Metro tick;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++){
		out[0][i] = out[1][i] = hihat.Process(false, tick.Process(), .4f, .1f, .5f, .2f, .8f);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	hihat.Init(sample_rate);
	
	tick.Init(2.f, sample_rate);	
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}
