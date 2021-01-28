#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

Flanger flanger;
DaisyField hw;

bool effectOn = true;

void AudioCallback(float **in, float **out, size_t size)
{
	hw.ProcessAllControls();

	flanger.SetDelay(hw.knob[0].Process());
	flanger.SetFeedback(hw.knob[1].Process());
	flanger.SetLfoFreq(hw.knob[2].Process() * .5);
	flanger.SetLfoDepth(hw.knob[3].Process());

	effectOn ^= hw.sw[0].RisingEdge();

	for (size_t i = 0; i < size; i++)
	{
		out[0][i] = out[1][i] = flanger.Process(in[0][i]);
	}
}

int main(void)
{
	hw.Init();
	float sample_rate = hw.AudioSampleRate();

	flanger.Init(sample_rate);

	hw.StartAdc();
	hw.StartAudio(AudioCallback);
	while(1) {}
}
