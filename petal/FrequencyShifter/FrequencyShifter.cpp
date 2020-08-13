#include "daisy_petal.h"
#include "daisysp.h" 

using namespace daisy;
using namespace daisysp;

DaisyPetal hw;

void callback(float **in, float **out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
    }
}

int main(void)
{
    float samplerate;

    hw.Init();
    samplerate = hw.AudioSampleRate();

    hw.StartAdc();
    hw.StartAudio(callback);
    while(1) 
    {
    }
}
