#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatchSM patch;
ReverbSc     reverb;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();

    float rev_time = 0.3 + (0.67 * patch.GetAdcValue(patch.CV_1));
    reverb.SetFeedback(rev_time);

    /** Well I'm sure we want a helper function or something for this */
    const float kDampFreqMin = log(1000.f);
    const float kDampFreqMax = log(19000.f);
    float       damp_control = patch.GetAdcValue(patch.CV_2);
    float       damping
        = exp(kDampFreqMin + (damp_control * (kDampFreqMax - kDampFreqMin)));
    reverb.SetLpFreq(damping);

    float inlevel = patch.GetAdcValue(patch.CV_3);
    float sendamt = patch.GetAdcValue(patch.CV_4);

    for(size_t i = 0; i < size; i++)
    {
        float dryl  = IN_L[i] * inlevel;
        float dryr  = IN_R[i] * inlevel;
        float sendl = IN_L[i] * sendamt;
        float sendr = IN_R[i] * sendamt;
        float wetl, wetr;
        reverb.Process(sendl, sendr, &wetl, &wetr);
        OUT_L[i] = dryl + wetl;
        OUT_R[i] = dryr + wetr;
    }
}

int main(void)
{
    patch.Init();
    reverb.Init(patch.AudioSampleRate());

    patch.StartAudio(AudioCallback);
    while(1) {}
}
