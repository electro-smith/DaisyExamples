#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
VOSIMOscillator vosim;
Oscillator lfo;

float f0,f1,f2;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
			
		float mod = lfo.Process();
		out[0][i] = out[1][i] = vosim.Process(f0, f1 * (1.f + mod), f2, mod);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	
	vosim.Init(sample_rate);

	f0 = 105.0f / sample_rate;
	f1 = 1390.7f / sample_rate;
	f2 = 817.2f / sample_rate;


	lfo.Init(sample_rate);
	lfo.SetAmp(1.f);
	lfo.SetFreq(.5f);

	hw.StartAudio(AudioCallback);
	while(1) {}
}
