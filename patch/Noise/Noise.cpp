#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

static void AudioCallback(float **in, float **out, size_t size)
{
    for (size_t i = 0; i < size; i += 2)
    {
        for (size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = 0.f;
        }
    }
}

int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();
   
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1) 
    {
    }
}
