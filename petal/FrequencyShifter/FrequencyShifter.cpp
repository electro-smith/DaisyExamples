#include "daisy_petal.h"
#include "daisysp.h" 

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
PitchShifter ps;
Oscillator lfo;
CrossFade fader;

bool bypassOn;

Parameter lfoFreqParam;
Parameter shiftTransParam;

void UpdateControls();

void AudioCallback(float **in, float **out, size_t size)
{
	UpdateControls();
	
    for (size_t i = 0; i < size; i++)
    {
		ps.SetTransposition(shiftTransParam.Process() + (lfo.Process()) * 12);

		float inf  = in[0][i];
		float process = ps.Process(in[0][i]);
		out[0][i] = out[1][i] = fader.Process(inf, process); 
	}
}

int main(void)
{
    float samplerate;
    petal.Init();
    samplerate = petal.AudioSampleRate();

	lfoFreqParam.Init(petal.knob[0], .1, 20, Parameter::LOGARITHMIC);
	shiftTransParam.Init(petal.knob[2], -12, 12, Parameter::LINEAR);

	lfo.Init(samplerate);
	lfo.SetAmp(1);
	lfo.SetWaveform(Oscillator::WAVE_SIN);

	ps.Init(samplerate);

	fader.Init();

    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1) 
    {
		petal.SetFootswitchLed((DaisyPetal::FootswitchLed)0, bypassOn);
		dsy_system_delay(6);
	}
}

void UpdateControls()
{
	petal.UpdateAnalogControls();
	petal.DebounceControls();

	//knobs
	lfo.SetFreq(lfoFreqParam.Process());
	lfo.SetAmp(petal.knob[1].Process());

	fader.SetPos(petal.knob[3].Process());
	if (bypassOn)
	{
		fader.SetPos(0);
	}
	
	//bypass switch
	if (petal.switches[0].RisingEdge())
	{
		bypassOn = !bypassOn;
	}
}