#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;
AdEnv envelopes[4];
Parameter timeParam[4];
Parameter curveParam[4];


bool lastInput[4];
int envMode;
int curveTimeMode;

void ProcessControls();
void UpdateOled();

void AudioCallback(float **in, float **out, size_t size)
{
    ProcessControls();
    UpdateOled();
    
    for(size_t i = 0; i < size; i++)
    {
	//dsy_dac_write(DSY_DAC_CHN_BOTH, (i / size) * 4095);

	for (size_t j = 0; j < 4; j++)
	{
	    float env = envelopes[j].Process();
	    out[j][i] = env * in[j][i];
	}
    }
}

void InitEnvelopes(float samplerate)
{
    for (int i = 0; i < 4; i++)
    {
	//envelope values
	envelopes[i].Init(samplerate);
	envelopes[i].SetMax(1);
	envelopes[i].SetMin(0);
	envelopes[i].SetTime(ADENV_SEG_ATTACK, .1);
	envelopes[i].SetTime(ADENV_SEG_DECAY, .4);
	envelopes[i].SetCurve(0);

	//last trigger in
	lastInput[i] = false;
	
	//envelope parameters (att1, dec1, etc.)
	timeParam[i].Init(hw.controls[i], .01, 2, Parameter::EXPONENTIAL);
	curveParam[i].Init(hw.controls[i], -10, 10, Parameter::LINEAR);
    }
}

int main(void)
{
    // Init everything.
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    InitEnvelopes(samplerate);

    envMode = 0;

    //DAC for CV out
    dsy_dac_init(& hw.seed.dac_handle , DSY_DAC_CHN_BOTH);
    dsy_dac_start(DSY_DAC_CHN_BOTH);
    
    // Start the ADC and Audio Peripherals on the Hardware
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;) {}
}

void ProcessEncoder()
{
    envMode += hw.encoder.Increment();
    envMode = (envMode % 2 + 2) % 2;

    curveTimeMode += hw.encoder.RisingEdge();
    curveTimeMode = curveTimeMode % 2;
}

bool ConditionalParameter(float oldVal, float newVal)
{
    return abs(oldVal - newVal) > 0.0005;
}


//this function is UGLY!!
float oldVals[4];
void UpdateEnvelopes(float newVals[4])
{
    for (int i = 0; i < 2; i++)
    {
	if (curveTimeMode == 0)
	{
	    if (ConditionalParameter(oldVals[i * 2], newVals[i * 2]))
		envelopes[i + 2 * envMode].SetTime(ADENV_SEG_ATTACK, timeParam[i * 2].Process());
	    
	    if (ConditionalParameter(oldVals[i * 2 + 1], newVals[i * 2 + 1]))
		envelopes[i + 2 * envMode].SetTime(ADENV_SEG_DECAY, timeParam[i * 2 + 1].Process());
	}
	else
	{
	    if (ConditionalParameter(oldVals[i * 2], newVals[i * 2]))
		envelopes[i + 2 * envMode].SetCurve(curveParam[i * 2].Process());
	    
	    if (ConditionalParameter(oldVals[i * 2 + 1], newVals[i * 2 + 1]))
		envelopes[i + 2 * envMode].SetCurve(curveParam[i * 2 + 1].Process());
	}
    }
}

void ProcessKnobs()
{
    float newVals[4];
    for (int i = 0; i < 4; i++)
    {
	newVals[i] = hw.controls[i].Process();
    }
    
    UpdateEnvelopes(newVals);

    for (int i = 0 ; i < 4; i++)
    {
	oldVals[i] = newVals[i];
    }
}

void ProcessGates()
{
    if (hw.gate_input[DaisyPatch::GATE_IN_1].Trig())
    {
	envelopes[0].Trigger();
	envelopes[1].Trigger();
    }

    if (hw.gate_input[DaisyPatch::GATE_IN_2].Trig())
    {
	envelopes[2].Trigger();
	envelopes[3].Trigger();
    }
}

void ProcessControls()
{
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    
    ProcessEncoder();
    ProcessKnobs();
    ProcessGates();
}

void UpdateOled()
{
    hw.DisplayControls(curveTimeMode);
}
