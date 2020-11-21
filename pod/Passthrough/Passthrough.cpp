#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;

static void  AudioCallback(float *in, float *out, size_t size)
{

    for(size_t i = 0; i < size; i += 2)
    {
        // Assign left input to left output
        out[i] = in[i];

        // Assign right input to right output
        out[i+1] = in[i+1];
    }
}

int main(void)
{
	// Initialize Pod hardware
    hw.Init();

    // Set sample rate to 96kHz
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

    // Start audio processing
	hw.StartAudio(AudioCallback);

    while(1)
    {
        // loop forever
    }
}
