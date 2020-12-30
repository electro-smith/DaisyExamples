#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
VariableShapeOscillator variosc;
Oscillator lfo;

float m;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		float mod = lfo.Process();
		out[0][i] = out[1][i] = variosc.Process(true, m, m * (1 + mod), .5f, .5f);
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
	lfo.SetFreq(1.f);

	hw.StartAudio(AudioCallback);
	while(1) {}
}
