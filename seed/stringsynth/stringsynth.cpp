#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
StringSynthOscillator string;

float f;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		float amplitudes[7] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
		out[0][i] = out[1][i] = string.Process(f, amplitudes,1.f);
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();
	f = 440.f / sample_rate;

	string.Init();

	hw.StartAudio(AudioCallback);
	while(1) {}
}
