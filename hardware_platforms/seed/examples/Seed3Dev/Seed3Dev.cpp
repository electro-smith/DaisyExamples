// # Seed3Dev
// ## Description
// Dev Playground for Seed3
//
#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "daisysp.h"
using namespace daisy;

// Globals
DaisySeed           hw;
daisysp::Oscillator DSY_SDRAM_BSS osc;

// Local
static void InitAk4556();

float    sig;

void AudioTest(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        sig        = osc.Process();
        //        out[i]     = in[i];
        //        out[i+1]     = in[i+1];
        out[i] = out[i + 1] = sig;
        //        out[i]     = sig;
        //        out[i + 1] = in[i + 1];
    }
}


int main(void)
{
    // Configure hardware
    hw.Configure();
    hw.Init();
    // Reconfigure for AK4556
//    hw.audio_handle.sai->device[DSY_AUDIO_INTERNAL]   = DSY_AUDIO_NONE;
//    hw.audio_handle.sai->device[DSY_AUDIO_EXTERNAL]   = DSY_AUDIO_NONE;
//    hw.audio_handle.sai->bitdepth[DSY_AUDIO_INTERNAL] = DSY_AUDIO_BITDEPTH_24;
    // Init
//    InitAk4556();
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, 256);
    // Init Synthesis
    osc.Init(DSY_AUDIO_SAMPLE_RATE);
    osc.SetAmp(0.75f);
    osc.SetFreq(100.0f);
    // 4 second time to boot just to have to time to open a COM terminal.
    hw.StartAudio(AudioTest);
    for(;;) {}
}

static void InitAk4556()
{
    dsy_gpio reset_pin;
    reset_pin.pin  = {DSY_GPIOB, 11};
    reset_pin.mode = DSY_GPIO_MODE_OUTPUT_PP;
    dsy_gpio_init(&reset_pin);

    // Before audio starts we need to reset the chip
    dsy_gpio_write(&reset_pin, 0); // Power down and reset
    dsy_system_delay(10);
    dsy_gpio_write(&reset_pin, 1); // Normal Operation
}
