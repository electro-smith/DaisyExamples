#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;
AdEnv envelopes[4];
Parameter envParam[4];

bool lastInput[4];
int mode;

bool AudioInTrigger(int inIdx, float inVal)
{
    bool ret = false;
    bool currentIn = inVal > .8f;

    if (!lastInput[inIdx] && currentIn)
    {
	ret = true;
    }

    lastInput[inIdx] = currentIn;
    return ret;
}

void ProcessControls();

void AudioCallback(float **in, float **out, size_t size)
{
    ProcessControls();
    
    for(size_t i = 0; i < size; i++)
    {
	for (size_t j = 0; j < 4; j++)
	{
	    if (AudioInTrigger(j, in[j][i]))
	    {
		envelopes[j].Trigger();
	    }
	    out[j][i] = envelopes[j].Process();
	}
    }
}

void InitEnvelopes(float samplerate)
{
    for (int i = 0; i < 4; i++)
    {
	//enveloper values
	envelopes[i].Init(samplerate);
	envelopes[i].SetMax(1);
	envelopes[i].SetMin(0);
	envelopes[i].SetTime(ADENV_SEG_ATTACK, .1);
	envelopes[i].SetTime(ADENV_SEG_DECAY, .4);
	envelopes[i].SetCurve(0);

	//last trigger in
	lastInput[i] = false;
	
	//envelope parameters (att1, dec1, etc.)
	envParam[i].Init(hw.controls[i], .01, 2, Parameter::EXPONENTIAL);
    }
}

int main(void)
{
    // Init everything.
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    InitEnvelopes(samplerate);

    mode = 0;
    
    // Start the ADC and Audio Peripherals on the Hardware
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;) {}
}

void ProcessEncoder()
{
    mode += hw.encoder.Increment();
    mode = (mode % 2 + 2) % 2;
}

void ProcessKnobs()
{
    for (int i = 0; i < 2; i++)
    {
	envelopes[i + 2 * mode].SetTime(ADENV_SEG_ATTACK, envParam[i * 2].Process());
	envelopes[i + 2 * mode].SetTime(ADENV_SEG_DECAY, envParam[i * 2 + 1].Process());
    }
}

void ProcessControls()
{
    ProcessEncoder();
    ProcessKnobs();   
}
