// Temporary Control Instructions:
// Press Encoder to cycle between time and curve controls.
// Turn encoder to cycle between envelopes 1,2 and 3,4
// In curve mode the pairs of knobs both control the same thing.
// i.e. If the screen says curve, env1, Both control 1 and control 2 can set envelope 1's curve.
// The latest one to be turned is in control

#include "daisy_patch.h"
#include "daisysp.h"
#include <cstring>
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;
AdEnv envelopes[4];
Parameter timeParam[4];
Parameter curveParam[4];

bool lastInput[4];
int envMode;
int curveTimeMode;

float envSig[4]; //Envelopes 1 and 3 are available at the CV outputs

void ProcessControls();

void AudioCallback(float **in, float **out, size_t size)
{
    ProcessControls();
    
    for(size_t i = 0; i < size; i++)
    {
	for (size_t chn = 0; chn < 4; chn++)
	{
	    float env = envelopes[chn].Process();
	    envSig[chn] = env;
	    out[chn][i] = env * in[chn][i];
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

void UpdateOled();

int main(void)
{
    // Init everything.
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    InitEnvelopes(samplerate);

    envMode = 0;
    curveTimeMode = 0;
    
    UpdateOled();
	
    // Start the ADC and Audio Peripherals on the Hardware
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;)
    {
	dsy_dac_write(DSY_DAC_CHN1, envSig[0] * 4095);
	dsy_dac_write(DSY_DAC_CHN2, envSig[2] * 4095);
	hw.DelayMs(1);
    }
}

void UpdateOled()
{
    hw.display.Fill(false);

    std::string num = std::to_string(envMode * 2 + 1);
    std::string env = "env";
    char envOne[4];
    strcpy(envOne, env.append(num).c_str());

    hw.display.SetCursor(0,0);
    hw.display.WriteString(envOne, Font_7x10, true);

    num = std::to_string(envMode * 2 + 2);
    env = "env";
    char envTwo[4];
    strcpy(envTwo, env.append(num).c_str());

    hw.display.SetCursor(70,0);
    hw.display.WriteString(envTwo, Font_7x10, true);
    
    hw.display.SetCursor(0,50);
    curveTimeMode ? hw.display.WriteString("curve", Font_7x10, true) : hw.display.WriteString("attack/decay", Font_7x10, true); 
    
    hw.display.Update();
}

void ProcessEncoder()
{
    int inc = hw.encoder.Increment();
    envMode += inc;
    envMode = (envMode % 2 + 2) % 2;

    int edge = hw.encoder.RisingEdge();
    curveTimeMode += edge;
    curveTimeMode = curveTimeMode % 2;

    if (edge != 0 || inc != 0)
    {
	UpdateOled();
    }
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


