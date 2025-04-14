// # WavPlayer
// ## Description
// Fairly simply sample player.
// Loads 16
//
// Play .wav file from the SD Card.
//
#include <stdio.h>
#include <string.h>

// switch between Patch and Pod - the encoders are different
#ifndef USE_PATCH
#define USE_PATCH
#endif // USE_PATCH
#ifndef USE_PATCH_SM
//#define USE_PATCH_SM
// Patch_sm is like the POD in its Audio (2 channel) and lack of OLED display
// unlike the POD, it has no encoder, so we would have to use other controls like a knob and a pushbutton
#endif // USE_PATCH_SM

#ifdef USE_PATCH
#include "daisy_patch.h"
#else
    #ifdef USE_PATCH_SM
    #include "daisy_patch_sm.h"
    #else
    #include "daisy_pod.h"
    #endif //USE_PATCH_SM
#endif // USE_PATCH

using namespace daisy;
#ifdef USE_PATCH_SM
using namespace patch_sm;
#endif //USE_PATCH_SM

#ifdef USE_PATCH
DaisyPatch   hw;
#else
    #ifdef USE_PATCH_SM
    DaisyPatchSM hw;
    #else
    DaisyPod       hw;
    #endif //USE_PATCH_SM
#endif // USE_PATCH

SdmmcHandler   sdcard;
FatFSInterface fsi;
WavPlayer      sampler;

#ifdef USE_PATCH
void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
    size_t size)
#else // Pod and Patch_sm
void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
#endif // USE_PATCH
{
    int32_t inc;

    // Debounce digital controls
    hw.ProcessDigitalControls();

// turn the encoder support on and off, for debugging

#ifndef USE_PATCH_SM 
    // no encoder on Patch_sm
#ifndef USE_ENCODER
#define USE_ENCODER
#endif // USE_ENCODER
#endif // USE_PATCH_SM

#ifdef USE_ENCODER
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
#endif // USE_ENCODER

    //    if(hw.button1.RisingEdge())
    //    {
    //        sampler.Restart();
    //    }
    //
    //    if(hw.button2.RisingEdge())
    //    {
    //        sampler.SetLooping(!sampler.GetLooping());
    // ozh - the DaisyPatch::LED_2_B is a reference for the pre-OLED prototype of the Daisy
    //       2/23/2022 use hw.seed.SetLed(bool state);  // this is tested below
    //        //hw.SetLed(DaisyPatch::LED_2_B, sampler.GetLooping());
    //        //dsy_gpio_write(&hw.leds[DaisyPatch::LED_2_B],
    //        //               static_cast<uint8_t>(!sampler.GetLooping()));
    //    }

#ifdef USE_PATCH
    // this is a little counter intuitive.  
    // We have a channel and an index eg  out[CHNL][i] 
    // so, we don't need to move forward 2 for the index
    #define AUDIO_CHANNELS 1
#else  // Pod and Patch_sm
    #define AUDIO_CHANNELS 2
#endif // USE_PATCH
    for(size_t i = 0; i < size; i += AUDIO_CHANNELS)
    {
#ifdef USE_PATCH
        out[0][i] = out[1][i] = s162f(sampler.Stream()) * 0.5f;
#else // Pod and Patch_sm
        out[i] = out[i + 1] = s162f(sampler.Stream()) * 0.5f;
#endif
    }
}


int main(void)
{
    // Init hardware
    size_t blocksize = 4;
    //ozh - chg blocksize to match Init blocksize in daisy_patch.cpp
    //AudioHandle::Config cfg;
    //cfg.blocksize  = 48;
    //cfg.samplerate = SaiHandle::Config::SampleRate::SAI_48KHZ;
 
    hw.Init();
    //    hw.ClearLeds();
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd_cfg.speed = SdmmcHandler::Speed::MEDIUM_SLOW; //MEDIUM_SLOW; SLOW;// OZH set it to slower speed for debugging
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    sampler.Init(fsi.GetSDPath());
    sampler.SetLooping(true);
    // test only ozh
    //sampler.SetLooping(false); 

    // SET LED to indicate Looping status. // ozh this works to show Looping status on the "boot" led on the Daisy Seed
// different syntax for ORed conditions
#ifdef USE_PATCH
    hw.seed.SetLed(sampler.GetLooping());
#else
    #ifdef USE_PATCH_SM
    hw.SetLed(sampler.GetLooping());
    #endif
#endif // USE_PATCH

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
