#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatchSM patch;
Oscillator   osc;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    /** Todo add cal */
    patch.ProcessAllControls();
    float coarse = 36.f + (patch.GetAdcValue(patch.CV_1) * 60.f);
    float voct   = patch.GetAdcValue(patch.CV_5) * 60.f;
    float freq   = mtof(fclamp(coarse + voct, 0.f, 127.f));
    osc.SetFreq(freq);
    for(size_t i = 0; i < size; i++)
    {
        float sig = osc.Process();
        OUT_LEFT[i]  = sig;
        OUT_RIGHT[i] = sig;
    }
}

int main(void)
{
    patch.Init();
    osc.Init(patch.AudioSampleRate());
    patch.StartAudio(AudioCallback);
    while(1) {}
}
