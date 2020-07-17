#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Compressor comp;

bool isSideChained;

void UpdateControls();

Parameter threshParam, ratioParam, attackParam, releaseParam;

static void AudioCallback(float **in, float **out, size_t size)
{
    UpdateControls();
    
    for (size_t i = 0; i < size; i ++)
    {
	float sig = isSideChained ? comp.Process(in[0][i], in[1][i]) : comp.Process(in[0][i]);
	
	out[0][i] = out[1][i] = sig;
	out[2][i] = out[3][i] = sig;
    }
}

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware
    samplerate = patch.AudioSampleRate();

    comp.Init(samplerate);
  
    isSideChained = false;

    //parameter parameters
    threshParam.Init(patch.controls[0], -80.0f, 0.f, Parameter::LINEAR);
    ratioParam.Init(patch.controls[1], 1.f, 40.f, Parameter::LINEAR);
    attackParam.Init(patch.controls[2], 0.001f, 10.f, Parameter::LINEAR);
    releaseParam.Init(patch.controls[3], 0.001f, 10.f, Parameter::LINEAR);
    
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
	//update the oled
	patch.display.Fill(false);

	patch.display.SetCursor(0,0);
	std::string str = "Compressor";
	char* cstr = &str[0];
	patch.display.WriteString(cstr, Font_7x10, true);

	patch.display.SetCursor(0,25);
	str = "Sidechain: ";
	str += isSideChained? "On" : "Off";
	patch.display.WriteString(cstr, Font_7x10, true);
	
	patch.display.Update();
    }
}

void UpdateControls()
{
    patch.UpdateAnalogControls();
    patch.DebounceControls();
    
    //encoder click
    isSideChained = patch.encoder.RisingEdge() ? !isSideChained : isSideChained;

    //controls
    comp.SetThreshold(threshParam.Process());
    comp.SetRatio(ratioParam.Process());
    comp.SetAttack(attackParam.Process());
    comp.SetRelease(releaseParam.Process());
}
