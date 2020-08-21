#include "daisy_petal.h"
#include "daisysp.h" 

#define FILT_STAGES 4

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Oscillator lfo;

Svf filt[FILT_STAGES];

bool bypass;

void ProcessControls(float& depth)
{
	//knobs
	depth = hw.knob[0].Process() * 3.f;
	lfo.SetFreq(hw.knob[1].Process() * 10);

	//bypass
	bypass = hw.switches[0].RisingEdge() ? !bypass : bypass;
}

float UpdateFilters(float in, float lfoSignal)
{
	
	
	for (int i = 0; i < FILT_STAGES; i++)
	{
		filt[i].SetFreq(lfoSignal);
		filt[i].Process(in);
		in = filt[i].Notch();
	}
	
	return in;
}

void callback(float **in, float **out, size_t size)
{
	float depth;
	ProcessControls(depth);
	
    for (size_t i = 0; i < size; i++)
    {
		float lfoSignal = lfo.Process() + 2500; //0Hz - 5kHz
		
		for (int chn = 0; chn < 2; chn++)
		{
			if (! bypass)
			{
				in[chn][i] += depth * UpdateFilters(in[0][i], lfoSignal);
				in[chn][i] *= .25f;
			}
			
			out[chn][i] = in[chn][i];
		}
    }
}

void InitFilters(float samplerate)
{
	for (int i = 0; i < FILT_STAGES; i++)
	{	
		filt[i].Init(samplerate);
		filt[i].SetRes(0);
		filt[i].SetDrive(0);
	}
}

int main(void)
{
    hw.Init();
    float samplerate = hw.AudioSampleRate();
	
	lfo.Init(samplerate);
	lfo.SetFreq(1);
	lfo.SetAmp(2500);
	lfo.SetWaveform(Oscillator::WAVE_SIN);
	
	InitFilters(samplerate);

	bypass = false;

    hw.StartAdc();
    hw.StartAudio(callback);
    
	int i = 0;
	while(1) 
    {
        dsy_system_delay(300);
		hw.ClearLeds();
		hw.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);
		i++;
        i %= 8;
		
		hw.SetFootswitchLed((DaisyPetal::FootswitchLed)0, bypass);
		
		hw.UpdateLeds();
    }
}
