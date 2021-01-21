#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Chorus ch;

bool effectOn;

void AudioCallback(float **in, float **out, size_t size)
{
	hw.ProcessAllControls();
	
	ch.SetLfoFreq(hw.knob[2].Process() * 10.f + .03f);
	ch.SetLfoDepth(hw.knob[3].Process() * 6.5f);
	ch.SetDelay(hw.knob[4].Process() * 20.f);
	
	effectOn ^= hw.switches[0].RisingEdge();
	
	for (size_t i = 0; i < size; i++)
	{
		float sig = effectOn ? ch.Process(in[0][i]) : in[0][i];
		out[0][i] = out[1][i] = sig;
	}
}

int main(void)
{
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	ch.Init(sample_rate);
    
	effectOn = true;
	
	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {}
}
