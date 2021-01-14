#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
AnalogBassDrum bd;
Metro tick;

float r = 0.f;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		bool t = tick.Process();
		if(t){
			r = random() / (float)RAND_MAX;
			r *= 100.f;
			r -= 50.f;
		}
										//sus   trig  acc  f    ton  dec atfm  self_fm
		out[0][i] = out[1][i] = bd.Process(false, t, .1f, .001f, 1.f, 1.f, 1.f, 20.f);
		//decay -3 - 0
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
