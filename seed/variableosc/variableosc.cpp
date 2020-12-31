#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
VariableShapeOscillator variosc;
Oscillator lfo, lfo2;

float m;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		float mod = lfo.Process();
		float mod2 = lfo2.Process();
		out[0][i] = out[1][i] = variosc.Process(true, m, m * (mod + 3), fabsf(mod) * .8f, mod2);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	m = 110.f / sample_rate;

	variosc.Init(sample_rate);

	lfo.Init(sample_rate);
	lfo.SetAmp(1.f);
	lfo.SetFreq(.25f);

	lfo2.Init(sample_rate);
	lfo2.SetAmp(1.f);
	lfo2.SetFreq(.125f);


	hw.StartAudio(AudioCallback);
	while(1) {}
}
