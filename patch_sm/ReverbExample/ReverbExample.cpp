#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace patch_sm;

DaisyPatchSM patch;
ReverbSc     reverb;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();

    /** Update Params with the four knobs */
    float time = fmap(patch.GetAdcValue(CV_1), 0.3f, 0.99f);
    float damp = fmap(patch.GetAdcValue(CV_2), 1000.f, 19000.f, Mapping::LOG);
    float in_level   = patch.GetAdcValue(CV_3);
    float send_level = patch.GetAdcValue(CV_3);


    reverb.SetFeedback(time);
    reverb.SetLpFreq(damp);

    for(size_t i = 0; i < size; i++)
    {
        float dryl  = IN_L[i] * in_level;
        float dryr  = IN_R[i] * in_level;
        float sendl = IN_L[i] * send_level;
        float sendr = IN_R[i] * send_level;
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

    /** And the new stuff for gpio as well */

    while(1) {}
}
