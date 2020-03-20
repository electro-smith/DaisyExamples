#include "daisy_patch.h"
#include "daisysp.h"
#include <math.h>

using namespace daisy;
using namespace daisysp;


daisy_patch	hw;
Oscillator	 osc;
WhiteNoise	 nse;
static uint8_t wf;
parameter	  param_freq, param_nse_amp, param_osc_amp, param_bright;
parameter	  param_ampcv;

static float freq;
static void audio(float *in, float *out, size_t size)
{
	float sig, namp, oamp;
	// Check Switch to change waveform
    hw.button1.Debounce();
	//if(dsy_switch_falling_edge(&hw.button1))
    if (hw.button1.FallingEdge())
	{
		wf = (wf + 1) % osc.WAVE_LAST; // increment and wrap
		osc.SetWaveform(wf);
	}
	// Audio Loop
	for(size_t i = 0; i < size; i += 2)
	{
		// Get Parameters
		param_bright.process(); // for LEDs below
		freq  = param_freq.process();
		namp  = param_nse_amp.process();
		oamp  = param_osc_amp.process();
		oamp += param_ampcv.process();
		namp += (param_ampcv.value() * param_ampcv.value()); // exp only for noise
		// Set module parameters
		osc.SetFreq(freq);
		osc.SetAmp(oamp);
		nse.SetAmp(namp);
		// Process
		sig	= osc.Process();
		sig += nse.Process();
		out[i] = out[i+1] = sig;
	}
}

int main(void)
{
	// Initialize Hardware
	hw.Init();
	param_freq.init(hw.GetCtrl(daisy_patch::KNOB_1), 10.0f, 20000.0f, parameter::LOG);
	param_nse_amp.init(hw.GetCtrl(daisy_patch::KNOB_2), 0.0f, 1.0f, parameter::EXP);
	param_osc_amp.init(hw.GetCtrl(daisy_patch::KNOB_3), 0.0f, 0.4f, parameter::LINEAR);
	param_bright.init(hw.GetCtrl(daisy_patch::KNOB_4), 0.0f, 1.0f, parameter::CUBE);
	param_ampcv.init(hw.GetCtrl(daisy_patch::CV_2), 0.0f, 1.0f, parameter::LINEAR);
	// Init Osc and Nse
	dsy_tim_start();
	osc.Init(SAMPLE_RATE);
	nse.Init();
	// Old style still 
	dsy_audio_set_callback(DSY_AUDIO_INTERNAL, audio);
	dsy_audio_start(DSY_AUDIO_INTERNAL);
	//dsy_adc_start();

	for(uint16_t i = 0; i < daisy_patch::LED_LAST; i++) 
	{
		dsy_led_driver_set_led(i, 0.0f);
	}
	while(1) 
	{
		dsy_tim_delay_ms(20);
		for(uint16_t i = 0; i < daisy_patch::LED_LAST; i++)
		{
			dsy_led_driver_set_led(i, param_bright.value());
		}
	}
}

