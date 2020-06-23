#include "daisy_patch.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;


// Hardware
DaisyPatch hw;

void AudioCallback(float **in, float **out, size_t size)
{
    // Synthesis.
    for(size_t i = 0; i < size; i += 2)
    {
	out[i] = 0;
        out[i+1] 0;
    }
}

int main(void)
{
    // Init everything.
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();

    // Start the ADC and Audio Peripherals on the Hardware
    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    for(;;) {}
}
