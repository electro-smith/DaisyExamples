#include "daisy_petal.h"
#include "daisysp.h" 

#define FILT_STAGES 4
#define BUFFER_SIZE 9600

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;
Oscillator lfo;
Svf filt[FILT_STAGES];
float buff[FILT_STAGES][BUFFER_SIZE];
float depth;
float filterFreq;

float UpdateFilters(float in)
{
	float lfoSignal = lfo.Process() + 10000;
	
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
    for (size_t i = 0; i < size; i++)
    {
		in[0][i] = depth * UpdateFilters(in[0][i]);
		in[0][i] *= .5f;
		out[0][i] = out[1][i] = in[0][i];
    }
}

void InitFilters(float samplerate)
{
	for (int i = 0; i < FILT_STAGES; i++)
	{
		for (int j = 0; j < BUFFER_SIZE; j++)
		{
			buff[i][j] = 0.f;
		}
	
		//filt[i].Init(samplerate, buff[i], BUFFER_SIZE);
		filt[i].Init(samplerate);
		filt[i].SetRes(.8);
		filt[i].SetDrive(.5);
	}
}

int main(void)
{
    hw.Init();
    float samplerate = hw.AudioSampleRate();
	
	lfo.Init(samplerate);
	lfo.SetFreq(1);
	lfo.SetAmp(10000);
	lfo.SetWaveform(Oscillator::WAVE_SIN);
	
	InitFilters(samplerate);

	depth = .5f;

    hw.StartAdc();
    hw.StartAudio(callback);
    while(1) 
    {
        dsy_system_delay(6);
        hw.UpdateLeds();
    }
}
