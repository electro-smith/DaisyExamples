// # WavPlayer
// ## Description
// Fairly simply sample player.
// Loads 16
//
// Play .wav file from the SD Card.
//
#include <stdio.h>
#include <string.h>
//#include "daisy_pod.h"
#include "daisy_patch.h"

using namespace daisy;

//DaisyPatch hw;
DaisyPatch   hw;
SdmmcHandler sdcard;
WavPlayer    sampler;

void AudioCallback(float *in, float *out, size_t size)
{
    int32_t inc;

    // Debounce digital controls
    hw.DebounceControls();

    // Change file with encoder.
    inc = hw.encoder.Increment();
    if(inc > 0)
    {
        size_t curfile;
        curfile = sampler.GetCurrentFile();
        if(curfile < sampler.GetNumberFiles() - 1)
        {
            sampler.Open(curfile + 1);
        }
    }
    else if(inc < 0)
    {
        size_t curfile;
        curfile = sampler.GetCurrentFile();
        if(curfile > 0)
        {
            sampler.Open(curfile - 1);
        }
    }

//    if(hw.button1.RisingEdge())
//    {
//        sampler.Restart();
//    }
//
//    if(hw.button2.RisingEdge())
//    {
//        sampler.SetLooping(!sampler.GetLooping());
//        //hw.SetLed(DaisyPatch::LED_2_B, sampler.GetLooping());
//        //dsy_gpio_write(&hw.leds[DaisyPatch::LED_2_B],
//        //               static_cast<uint8_t>(!sampler.GetLooping()));
//    }

    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = out[i + 1] = s162f(sampler.Stream()) * 0.5f;
    }
}


int main(void)
{
    // Init hardware
    size_t blocksize = 48;
    hw.Init();
//    hw.ClearLeds();
    sdcard.Init();
    sampler.Init();
    sampler.SetLooping(true);

    // SET LED to indicate Looping status.
    //hw.SetLed(DaisyPatch::LED_2_B, sampler.GetLooping());

    // Init Audio
    hw.SetAudioBlockSize(blocksize);
    hw.StartAudio(AudioCallback);
    // Loop forever...
    for(;;)
    {
        // Prepare buffers for sampler as needed
        sampler.Prepare();
    }
}
