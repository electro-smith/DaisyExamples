#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
void AudioCallback(float **in, float **out, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
	}
}

int main(void)
{
	hw.Configure();
	hw.Init();
	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {}
}
