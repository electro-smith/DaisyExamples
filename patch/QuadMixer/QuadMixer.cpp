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
    
    for (size_t i = 0; i < size; i ++)
    {
	float output = 0.f;
	for (size_t chn = 0; chn < 4; chn++)
        {
            output += ctrlVal[chn] * in[chn][i];
        }

	output *= .25f;
	
	for (size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = output;
        }
    }
}

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
	patch.DisplayControls(false);
    }
}
