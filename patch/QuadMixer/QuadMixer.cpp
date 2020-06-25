#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

static void AudioCallback(float **in, float **out, size_t size)
{
    float ctrlVal[4];
    for (int i = 0; i < 4; i++)
    {
	ctrlVal[i] = patch.controls[i].Process();
    }
    
    for (size_t i = 0; i < size; i += 2)
    {
        for (size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = ctrlVal[chn] * in[chn][i];
        }
    }
}

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1) {
	patch.DisplayControls(false);
    } // loop forever
}
