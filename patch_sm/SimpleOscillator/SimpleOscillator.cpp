#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** TODO: ADD CALIBRATION */

DaisyPatchSM hw;
Oscillator   osc;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();

    float coarse_knob = hw.GetAdcValue(CV_1);
    float coarse      = fmap(coarse_knob, 36.f, 96.f);

    float voct_cv = hw.GetAdcValue(CV_5);
    float voct    = fmap(voct_cv, 0.f, 60.f);

    float midi_nn = fclamp(coarse + voct, 0.f, 127.f);
    float freq    = mtof(midi_nn);

    osc.SetFreq(freq);

    for(size_t i = 0; i < size; i++)
    {
        float sig = osc.Process();
        OUT_L[i]  = sig;
        OUT_R[i]  = sig;
    }
}

int main(void)
{
    hw.Init();
    osc.Init(hw.AudioSampleRate());
    hw.StartAudio(AudioCallback);
    while(1) {}
}
