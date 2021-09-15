#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatchSM patch;
ReverbSc     reverb;

MappedFloatValue param_time(0.3f, 0.99f, 0.85f);
MappedFloatValue
                 param_damp(1000.f, 19000.f, 12000.f, MappedFloatValue::Mapping::log);
MappedFloatValue param_in_level(0.0f, 1.f, 0.7f);
MappedFloatValue param_send_level(0.3f, 0.99f, 0.7f);

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();

    /** Update Params with the four knobs */
    param_time.SetFrom0to1(patch.GetAdcValue(patch.CV_1));
    param_damp.SetFrom0to1(patch.GetAdcValue(patch.CV_2));
    param_in_level.SetFrom0to1(patch.GetAdcValue(patch.CV_3));
    param_send_level.SetFrom0to1(patch.GetAdcValue(patch.CV_4));

    reverb.SetFeedback(param_time);
    reverb.SetLpFreq(param_damp);

    for(size_t i = 0; i < size; i++)
    {
        float dryl  = IN_L[i] * param_in_level;
        float dryr  = IN_R[i] * param_in_level;
        float sendl = IN_L[i] * param_send_level;
        float sendr = IN_R[i] * param_send_level;
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
    bool  ledstate = true;
    float phs      = 0.f;
    patch.WriteCvOut(patch.CV_OUT_BOTH, ledstate ? 5.f : 0.f);
    while(1)
    {
        //System::Delay(500);
        System::Delay(500);
        phs += 0.01f;
        if(phs > 1.f)
            phs -= 1.f;
        // patch.WriteCvOut(patch.CV_OUT_BOTH, phs * 5.f);
        patch.WriteCvOut(patch.CV_OUT_BOTH, ledstate ? 5.f : 0.f);
        patch.SetLed(ledstate);
        if(ledstate)
            ledstate = false;
        else
            ledstate = true;
    }
}
