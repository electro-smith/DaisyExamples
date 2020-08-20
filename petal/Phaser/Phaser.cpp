#include "daisy_petal.h"
#include "daisysp.h" 

#define ALLPASS_STAGES 1
#define BUFFER_SIZE 9600

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Oscillator lfo;
Allpass filt[ALLPASS_STAGES];
float buff[ALLPASS_STAGES][BUFFER_SIZE];
float depth;
float filterFreq;

float UpdateFilters(float in)
{
	float lfoSignal = lfo.Process() + .5;
	fonepole(filterFreq, lfoSignal, .01f);
	
	for (int i = 0; i < ALLPASS_STAGES; i++)
	{
		filt[i].SetFreq(filterFreq);
		in = filt[i].Process(in);
	}
	
	return in;
}

void callback(float **in, float **out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
		in[0][i] += depth * UpdateFilters(in[0][i]);
		in[0][i] *= .5f;
		out[0][i] = out[1][i] = in[0][i];
    }
}

void InitFilters(float samplerate)
{
	for (int i = 0; i < ALLPASS_STAGES; i++)
	{
		for (int j = 0; j < BUFFER_SIZE; j++)
		{
			buff[i][j] = 0.f;
		}
	
		filt[i].Init(samplerate, buff[i], BUFFER_SIZE);
	}	
}

int main(void)
{
    hw.Init();
    float samplerate = hw.AudioSampleRate();
	
	lfo.Init(samplerate);
	lfo.SetFreq(1);
	lfo.SetAmp(.5);
	lfo.SetWaveform(Oscillator::WAVE_SIN);
	
	InitFilters(samplerate);

	filterFreq = 0.f;
	depth = .5f;

    hw.StartAdc();
    hw.StartAudio(callback);
    while(1) 
    {
        dsy_system_delay(6);
        hw.UpdateLeds();
    }
}
