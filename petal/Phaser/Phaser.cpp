#include "daisy_petal.h"
#include "daisysp.h" 

#define FILT_STAGES 1
#define CHANNELS 2
#define BUFF_SIZE static_cast<size_t>(9600)

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Oscillator lfo;

Allpass filt[CHANNELS][FILT_STAGES];
float DSY_SDRAM_BSS buff[CHANNELS][FILT_STAGES][BUFF_SIZE];

bool bypass;

void ProcessControls(float& depth, float& feedback)
{
	hw.DebounceControls();
	hw.UpdateAnalogControls();
	
	//knobs
	depth = hw.knob[2].Process();
	lfo.SetFreq(hw.knob[3].Process() * 10);
	feedback = hw.knob[4].Process();

	//bypass
	bypass = hw.switches[0].RisingEdge() ? !bypass : bypass;
}

float lastFilterOut[CHANNELS];

float ProcessFilters(float in, float lfoSignal, float depth, float feedback, int chn)
{
	in += feedback * lastFilterOut[chn];
	in *= .5f;
	
	for (int i = 0; i < FILT_STAGES; i++)
	{
		filt[chn][i].SetFreq(lfoSignal);
		in = filt[chn][i].Process(in);
	}
	
	lastFilterOut[chn] = in;
	
	in *= depth;	
	return in;
}

void callback(float **in, float **out, size_t size)
{
	float depth, feedback;
	ProcessControls(depth, feedback);
	
    for (size_t i = 0; i < size; i++)
    {
		float lfoSignal = lfo.Process() + 0.05f; //0s - .2s
		
		for (int chn = 0; chn < CHANNELS; chn++)
		{
			out[chn][i] = in[chn][i];
			if (! bypass)
			{
				out[chn][i] += ProcessFilters(in[chn][i], lfoSignal, depth, feedback, chn);
				out[chn][i] *= .5f;
			}
		}
    }
}

void InitFilters(float samplerate)
{
	for (int chn = 0; chn < CHANNELS; chn++)
	{
		for (int i = 0; i < FILT_STAGES; i++)
		{	
			filt[chn][i].Init(samplerate, buff[chn][i], BUFF_SIZE);
		
			for (int bufPos = 0; bufPos < (int)BUFF_SIZE; bufPos++){
				buff[chn][i][bufPos] = 0;
			}
		
		}
		
		lastFilterOut[chn] = 0.f;
	}
}

int main(void)
{
    hw.Init();
    float samplerate = hw.AudioSampleRate();
	
	lfo.Init(samplerate);
	lfo.SetFreq(1);
	lfo.SetAmp(0.05f);
	lfo.SetWaveform(Oscillator::WAVE_SIN);
	
	InitFilters(samplerate);

	bypass = false;

    hw.StartAdc();
    hw.StartAudio(callback);
    
	int i = 0;
	while(1) 
    {
        dsy_system_delay(200);
		hw.ClearLeds();
		hw.SetRingLed((DaisyPetal::RingLed)i, 1.f, 0.f, 0.f);
		i++;
        i %= 8;
		
		hw.SetFootswitchLed((DaisyPetal::FootswitchLed)0, bypass);
		
		hw.UpdateLeds();
    }
}
