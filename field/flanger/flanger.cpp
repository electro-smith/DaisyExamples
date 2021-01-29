#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

Flanger flanger;
DaisyField hw;

bool effectOn = true;

float knob_vals[4];
void AudioCallback(float **in, float **out, size_t size)
{
	hw.ProcessAllControls();

	for(int i = 0; i < 4; i++){
		knob_vals[i] = hw.knob[i].Process();
	}

	flanger.SetDelay(knob_vals[0]);
	flanger.SetFeedback(knob_vals[1]);
	flanger.SetLfoFreq(knob_vals[2] * .5);
	flanger.SetLfoDepth(knob_vals[3]);

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
	while(1) {
		hw.display.Fill(false);
		
		char cstr[11];
		sprintf(cstr, "Effect %s", effectOn ? "On" : "Off");
		hw.display.SetCursor(0,0);
		hw.display.WriteString(cstr, Font_7x10, true);
		
		hw.display.Update();
	
		size_t knob_leds[] = {
			DaisyField::LED_KNOB_1,
			DaisyField::LED_KNOB_2,
			DaisyField::LED_KNOB_3,
			DaisyField::LED_KNOB_4, };
		
		for(size_t i = 0; i < 4; i++)
		{
			hw.led_driver.SetLed(knob_leds[i], knob_vals[i]);
		}
		
		hw.led_driver.SwapBuffersAndTransmit();
	}
}
