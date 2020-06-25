#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Parameter cutoff_ctrl, res_ctrl, drive_ctrl;
Svf svf;

static void AudioCallback(float **in, float **out, size_t size)
{

    float cutoff = cutoff_ctrl.Process();
    float res = res_ctrl.Process();
    float drive = drive_ctrl.Process();

    svf.SetFreq(cutoff);
    svf.SetRes(res);
    svf.SetDrive(drive);
    
    for (size_t i = 0; i < size; i ++)
    {
	svf.Process(in[0][i]);
	
	out[0][i] = svf.Low();
	out[1][i] = svf.High();
	out[2][i] = svf.Band();
	out[3][i] = svf.Notch();
    }
}

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();

    svf.Init(samplerate);
    
    cutoff_ctrl.Init(patch.controls[0], 20, 20000, Parameter::LOGARITHMIC);
    res_ctrl.Init(patch.controls[1], .3, 1, Parameter::LINEAR);
    drive_ctrl.Init(patch.controls[2], .3, 1, Parameter::LINEAR);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
	patch.display.SetCursor(0,0);
	patch.display.Fill(false);
	patch.display.Update();
	dsy_system_delay(300);
	
	patch.display.WriteString("SVF!!!", Font_11x18, true);
	patch.display.Update();
	dsy_system_delay(300);
    } 
}
