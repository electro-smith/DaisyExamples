#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** TODO: ADD CALIBRATION */

DaisyPatchSM patch;
Oscillator   osc;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAllControls();
    float coarse = fmap(patch.GetAdcValue(CV_1), 36.f, 96.f);
    float voct   = fmap(patch.GetAdcValue(CV_5), 0.f, 60.f);
    float freq   = mtof(fclamp(coarse + voct, 0.f, 127.f));
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
    patch.Init();
    osc.Init(patch.AudioSampleRate());
    patch.StartAudio(AudioCallback);
    while(1) {}
}
