#include "daisysp.h"
#include "daisy_petal.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPetal petal;
int freqs[8];

int bank;

struct ConditionalUpdate
{
    float oldVal = 0;

    bool Process(float newVal)
    {
        if (abs(newVal - oldVal) > .04)
	{
	    oldVal = newVal;
	    return true;
	}
	
	return false;
    }
};

ConditionalUpdate condUpdates[4];

struct Filter{
    Svf filt;
    float amp;
    
    void Init(float samplerate, float freq)
    {
	filt.Init(samplerate);
	filt.SetRes(1);
	filt.SetDrive(.002);
	filt.SetFreq(freq);
	amp = .5f;
    }
    
    float Process(float in)
    {
	filt.Process(in);
	return filt.Peak() * amp;
    }
};

Filter filters[8];

static void AudioCallback(float **in, float **out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
	float sig = 0.f;
	for (int j = 0; j < 8; j++)
	{
	    sig += filters[j].Process(in[0][i]);
	}
	sig *= .06;
	
	out[0][i] = out[1][i] = out[2][i] = out[3][i] = sig;
    }
}

void InitFreqs()
{
    freqs[0] = 350;
    freqs[1] = 500;
    freqs[2] = 750;
    freqs[3] = 1100;
    freqs[4] = 2200;
    freqs[5] = 3600;
    freqs[6] = 5200;
    freqs[7] = 7500;
}

void InitFilters(float samplerate)
{
    for (int i = 0; i < 8; i++)
    {
	filters[i].Init(samplerate, freqs[i]);
    }
}

void UpdateControls();
void UpdateLeds();

int main(void)
{
    float samplerate;
    petal.Init(); // Initialize hardware (daisy seed, and petal)
    samplerate = petal.AudioSampleRate();

    InitFreqs();
    InitFilters(samplerate);
    bank = 0;
    
    petal.StartAdc();
    petal.StartAudio(AudioCallback);
    while(1) 
    {
	UpdateControls();
	UpdateLeds();
    }
}

void UpdateControls()
{
    petal.UpdateAnalogControls();
    petal.DebounceControls();
    
    //encoder
    bank += petal.encoder.Increment();
    bank = (bank % 2 + 2) % 2;

    bank = petal.encoder.RisingEdge() ? 0 : bank;

    //controls
    for (int i = 2 ; i < 6; i++)
    {
	float val = petal.knob[i].Process();
	if (condUpdates[i].Process(val))
	{
	    filters[i + bank * 2].amp = val;
	}
    }
}

void UpdateLeds()
{
    for (int i = 0; i < 4; i++)
    {
		petal.ring_led[i].Set(filters[i].amp, (bank == 0) * filters[i].amp, filters[i].amp);
    }
    for (int i = 4; i < 8; i++)
    {
		petal.ring_led[i].Set(filters[i].amp, (bank == 1) * filters[i].amp, filters[i].amp);
    }



    petal.UpdateLeds();
}
