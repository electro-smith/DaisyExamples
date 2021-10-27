#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;
Switch       button;

Looper<48000 * 10>               looper_l;
Looper<48000 * 10>               looper_r;
Buffer<48000 * 10> DSY_SDRAM_BSS buffer_l;
Buffer<48000 * 10> DSY_SDRAM_BSS buffer_r;

float loop_level, in_level;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAllControls();
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

    looper_r.Init(&buffer_l);
    looper_r.Init(&buffer_r);

    button.Init(patch.B7);

    patch.StartAudio(AudioCallback);
    while(1)
    {
        patch.ProcessAllControls();
        button.Debounce();

        in_level   = patch.controls[0].Value();
        loop_level = patch.controls[1].Value();

        if(button.RisingEdge())
        {
            looper_l.TrigRecord();
            looper_r.TrigRecord();
        }

		if(button.TimeHeldMs() >= 1000.f){
			looper_l.Clear();
			looper_r.Clear();
		}
    }
}
