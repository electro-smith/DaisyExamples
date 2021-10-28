#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;
Switch       button;

Looper<48000 * 60>               looper_l;
Looper<48000 * 60>               looper_r;
Buffer<48000 * 60> DSY_SDRAM_BSS buffer_l;
Buffer<48000 * 60> DSY_SDRAM_BSS buffer_r;

float loop_level, in_level;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    // Process the controls
    patch.ProcessAllControls();
    button.Debounce();

    // Set in and loop gain from CV_1 and CV_2 respectively
    in_level   = patch.GetAdcValue(0);
    loop_level = patch.GetAdcValue(1);

    //if you press the button, toggle the record state
    if(button.RisingEdge())
    {
        looper_l.TrigRecord();
        looper_r.TrigRecord();
    }

    // if you hold the button longer than 1000 ms (1 sec), clear the loop
    if(button.TimeHeldMs() >= 1000.f)
    {
        looper_l.Clear();
        looper_r.Clear();
    }

    // Set the led to 5V if the looper is recording
    patch.WriteCvOut(2, 5.f * looper_l.Recording());

    for(size_t i = 0; i < size; i++)
    {
        float in_l = IN_L[i] * in_level;
        float in_r = IN_R[i] * in_level;

        float sig_l = looper_l.Process(in_l) * loop_level + in_l;
        float sig_r = looper_r.Process(in_r) * loop_level + in_r;

        OUT_L[i] = sig_l;
        OUT_R[i] = sig_r;
    }
}

int main(void)
{
    patch.Init();

    looper_l.Init(&buffer_l);
    looper_r.Init(&buffer_r);

    button.Init(patch.B7);

    patch.StartAudio(AudioCallback);
    while(1) {}
}
