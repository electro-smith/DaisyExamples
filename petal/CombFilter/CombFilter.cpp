#include "daisy_petal.h"
#include "daisysp.h" 

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
Comb comb;
Oscillator lfo;
CrossFade fader;

bool bypassOn;
float buf[9600];

Parameter lfoFreqParam;
Parameter combFreqParam;

void UpdateControls();

void AudioCallback(float **in, float **out, size_t size)
{
	UpdateControls();
	
    for (size_t i = 0; i < size; i++)
    {
		float inf  = in[0][i];
		float process = comb.Process(in[0][i]);
		out[0][i] = out[1][i] = fader.Process(inf, process); 
	}
}

int main(void)
{
    float samplerate;
    petal.Init();
    samplerate = petal.AudioSampleRate();

	lfoFreqParam.Init(petal.knob[0], .1, 20, Parameter::LOGARITHMIC);
	combFreqParam.Init(petal.knob[2], .01, 20, Parameter::LINEAR);

	lfo.Init(petal.AudioCallbackRate());
	lfo.SetAmp(1);
	lfo.SetWaveform(Oscillator::WAVE_SIN);

    for(int i = 0; i < 9600; i++)
    {
        buf[i] = 0.0f;
    }

    // initialize Comb object
    comb.Init(samplerate, buf, 9600);

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
	comb.SetFreq(combFreqParam.Process() + (lfo.Process()) * 10);
	
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