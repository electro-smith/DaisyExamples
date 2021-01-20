#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Chorus ch;

void AudioCallback(float **in, float **out, size_t size)
{
	hw.ProcessAllControls();
	
	ch.SetLfoFreq(hw.knob[2].Process());
	ch.SetLfoDepth(hw.knob[3].Process() * 60.f);
	ch.SetDelay(hw.knob[4].Process() * 60.f);
	
	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = out[1][i] = ch.Process(in[0][i]);
	}
}

int main(void)
{
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	ch.Init(sample_rate);
    
	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {}
}
