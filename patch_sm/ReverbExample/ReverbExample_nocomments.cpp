#include "DaisyPatchSM.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatchSM hw;
ReverbSc     reverb;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    /** First we'll tell the hardware to process the 8 analog inputs */
    hw.ProcessAnalogControls();

    float rev_time = 0.3 + (0.67 * hw.GetAdcValue(0));
    reverb.SetFeedback(rev_time);

    float damping = mtof(72.0 + hw.GetAdcValue(1) * 55.0);
    reverb.SetLpFreq(damping);

    float inlevel = hw.GetAdcValue(2);
    float sendamt = hw.GetAdcValue(3);

    for(size_t i = 0; i < size; i++)
    {
        float dryl  = in[0][i] * inlevel;
        float dryr  = in[1][i] * inlevel;
        float sendl = in[0][i] * sendamt;
        float sendr = in[1][i] * sendamt;

        float wetl, wetr;
        reverb.Process(sendl, sendr, &wetl, &wetr);

        out[0][i] = dryl + wetl;
        out[1][i] = dryr + wetr;
    }
}

int main(void)
{
    hw.Init();
    reverb.Init(hw.AudioSampleRate());

    hw.StartAudio(AudioCallback);
    while(1) {}
}
