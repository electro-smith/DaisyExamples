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

		out[0][i] = out[1][i] = bd.Process(t);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	
	bd.Init(sample_rate);
	bd.SetAccent(.1f);
	bd.SetFreq(50.f);
	bd.SetTone(.1f);
	bd.SetDecay(-0.1f);
	bd.SetAttackFmAmount(10.f);
	bd.SetSelfFmAmount(10.f);
	
	tick.Init(1.f, sample_rate);
	
	hw.StartAudio(AudioCallback);
	while(1) {}
}
