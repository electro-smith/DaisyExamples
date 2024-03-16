#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;
Parameter  cutoff_ctrl, res_ctrl, drive_ctrl;
Svf        svf;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    //get new control values
    float cutoff = cutoff_ctrl.Process();
    float res    = res_ctrl.Process();
    float drive  = drive_ctrl.Process();

    //Set filter to the values we got
    svf.SetFreq(cutoff);
    svf.SetRes(res);
    svf.SetDrive(drive);

    for(size_t i = 0; i < size; i++)
    {
        //send the next sample to the filter
        svf.Process(in[0][i]);

        //send the different filter types to the different outputs
        out[0][i] = svf.Low();
        out[1][i] = svf.High();
        out[2][i] = svf.Band();
        out[3][i] = svf.Notch();
    }
}

int main(void)
{
    float samplerate;
    hw.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = hw.AudioSampleRate();

    //Initialize filter
    svf.Init(samplerate);

    //setup controls
    cutoff_ctrl.Init(hw.controls[0], 20, 20000, Parameter::LOGARITHMIC);
    res_ctrl.Init(hw.controls[1], .3, 1, Parameter::LINEAR);
    drive_ctrl.Init(hw.controls[2], .3, 1, Parameter::LINEAR);

    //Put controls onscreen
    std::string str  = "Cut  Res  Drive  ";
    char*       cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.SetCursor(0, 50);
    str = "LP  HP  BP  Notch";
    hw.display.WriteString(cstr, Font_7x10, true);
    hw.display.Update();

    //start audio
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1) {}
}
