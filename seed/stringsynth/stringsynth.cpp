#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
StringSynthOscillator string;

void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = out[1][i] = string.Process();
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	string.Init(sample_rate);
	float amp[7] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	string.SetAmplitudes(amp);
	string.SetGain(1.f);
	string.SetFreq(440.f);

	hw.StartAudio(AudioCallback);
	while(1) {}
}
