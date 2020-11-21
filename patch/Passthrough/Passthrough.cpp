#include "daisy_patch.h"
#include <string>

using namespace daisy;

DaisyPatch hw;

void AudioCallback(float **in, float **out, size_t size)
{

    // Buffer variables
    float *in1, *in2, *in3, *in4, *out1, *out2, *out3, *out4;

    // Assign buffer memory locations to buffer variables for easy reading/writing
    in1 = in[0], in2 = in[1], in3 = in[2], in4 = in[3];
    out1 = out[0], out2 = out[1], out3 = out[2], out4 = out[3];

    // Read audio inputs and assign to outputs
    for(size_t i = 0; i < size; i++)
    {
        out1[i] = in1[i];
        out2[i] = in2[i];
        out3[i] = in3[i];
        out4[i] = in4[i];
    }
}

int main(void)
{
    // Initialize Patch hardware
    hw.Init();

    // Set sample rate to 96kHz
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

    // Start audio processing
    hw.StartAudio(AudioCallback);

    // Write to OLED display
    std::string str  = "Passthrough Example at 96kHz";
    char *      cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);
    hw.display.Update();
    hw.DelayMs(1000);

    for(;;)
    {
        // loop forever
    }
}
