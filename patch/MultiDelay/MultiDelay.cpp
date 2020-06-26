#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

DelayLine <float, MAX_DELAY>  DSY_SDRAM_BSS delay[3];

struct delTime{
    float currentDelay;
    float delayTarget;
};

delTime delTimes[3];
Parameter params[4];

float feedback;
int drywet;

void ProcessControls();

static void AudioCallback(float **in, float **out, size_t size)
{
    ProcessControls();
    
    for (size_t i = 0; i < size; i ++)
    {
	float mix = 0;

	//update delayline with feedback
	for (int d = 0; d < 3; d++)
	{
	    float read = delay[d].Read();
	    delay[d].Write((feedback * read) + (1 - feedback) * in[0][i]);
	    mix += read;   
	}

	//apply drywet and attenuate
	float fdrywet = (float)drywet / 100.f;
	mix = ((fdrywet * mix * .3f) + ((1.0f - fdrywet) * in[0][i]));
	for (size_t chn = 0; chn < 4; chn++)
        {   
	    out[chn][i] = mix;
        }
    }
}

void InitDelays(float samplerate)
{
    for (int i = 0; i < 3; i++)
    {
	//Init delays
	delay[i].Init();
	//3 delay times
	params[i].Init(patch.controls[i], samplerate * .1, MAX_DELAY, Parameter::LOGARITHMIC);
    }
    
    //feedback
    params[3].Init(patch.controls[3], 0, 1, Parameter::LINEAR); 
}

void UpdateOled();

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();
    
    InitDelays(samplerate);

    drywet = 50;
    
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
	UpdateOled();
    } 
}

void UpdateOled()
{
    patch.display.Fill(false);
    
    patch.display.SetCursor(0,0);
    patch.display.WriteString("Multi Delay", Font_7x10, true);

    patch.display.SetCursor(0, 30);
    patch.display.WriteString("Dry/Wet:", Font_7x10, true);

    patch.display.SetCursor(60, 30);
    std::string str = std::to_string(drywet);
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    
    patch.display.Update();

}

void ProcessControls()
{
    patch.UpdateAnalogControls();
    patch.DebounceControls();

    //knobs
    for (int i = 0; i < 3; i++)
    {
	delTimes[i].delayTarget = params[i].Process();
    }
    feedback = params[3].Process();
    
    //encoder
    drywet += 5 * patch.encoder.Increment();
    drywet > 100 ? drywet = 100 : drywet = drywet;
    drywet < 0 ? drywet = 0 : drywet = drywet;

    //set delays
    for (int i = 0; i < 3; i++)
    {
	fonepole(delTimes[i].currentDelay, delTimes[i].delayTarget, .004f);
	delay[i].SetDelay(delTimes[i].currentDelay);
    }
}
