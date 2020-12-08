// Basic Template for DaisyPetal
#include "daisy_petal.h"

using namespace daisy;

DaisyPetal hw;

void AudioCallback(float *in, float *out, size_t size)
{
    hw.ProcessDigitalControls();
    hw.ProcessAnalogControls();
    for(size_t i = 0; i < size; i += 2)
    {
        out[i]     = in[i];
        out[i + 1] = in[i + 1];
    }
}

int main(void)
{
    hw.Init();
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;) {}
}
