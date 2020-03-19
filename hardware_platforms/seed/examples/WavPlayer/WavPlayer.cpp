// # WavPlayer
// ## Description
// Fairly simply sample player.
// Loads 16
//
// Play .wav file from the SD Card.
//
#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"

using namespace daisy;

daisy_pod    hw;
SdmmcHandler sdcard;
WavPlayer    sampler;

void AudioCallback(float *in, float *out, size_t size)
{
    int32_t inc;

    // Debounce digital controls
    hw.encoder.Debounce();
    hw.switches[SW_1].Debounce();
    hw.switches[SW_2].Debounce();

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

    if(hw.switches[SW_2].RisingEdge())
    {
        sampler.Restart();
    }

    if(hw.switches[SW_1].RisingEdge())
    {
        sampler.SetLooping(!sampler.GetLooping());
        dsy_gpio_write(&hw.leds[LED_2_B],
                       static_cast<uint8_t>(!sampler.GetLooping()));
    }

    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = out[i + 1] = s162f(sampler.Stream()) * 0.5f;
    }
}


int main(void)
{
    // Init hardware
    size_t blocksize = 48;
    daisy_pod_init(&hw);
    // Set all LEDs off:
    for(size_t i = 0; i < LED_LAST; i++)
    {
        dsy_gpio_write(&hw.leds[i], true);
    }

    sdcard.Init();
    sampler.Init();
    sampler.SetLooping(true);

    // SET LED to indicate Looping status.
    dsy_gpio_write(&hw.leds[LED_2_B],
                   static_cast<uint8_t>(!sampler.GetLooping()));


    // Init Audio
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, blocksize);
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, AudioCallback);
    dsy_audio_start(DSY_AUDIO_INTERNAL);
    // Loop forever...
    for(;;)
    {
        // Prepare buffers for sampler as needed
        sampler.Prepare();
    }
}
