#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    float ctrlVal[4];
    for(int i = 0; i < 4; i++)
    {
        //Get the four control values
        ctrlVal[i] = patch.controls[i].Process();
    }

    for(size_t i = 0; i < size; i++)
    {
        float output = 0.f;
        //sum all four inputs, attenuated by the control levels
        for(size_t chn = 0; chn < 4; chn++)
        {
            output += ctrlVal[chn] * in[chn][i];
        }

        //attenuate by 1/4
        output *= .25f;

        //send the same thing to all 4 outputs
        for(size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = output;
        }
    }
}

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    //start audio
    patch.StartAdc();
    patch.StartAudio(AudioCallback);


    while(1)
    {
        //display the four control levels
        patch.DisplayControls(false);
    }
}
