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
//#include "daisy_patch.h"

using namespace daisy;

//DaisyPatch   hw;
DaisyPod       hw;
SdmmcHandler   sdcard;
FatFSInterface fsi;
WavPlayer      sampler;

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    int32_t inc;

    // Debounce digital controls
    hw.ProcessDigitalControls();

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
    size_t blocksize = 4;
    hw.Init();
    //    hw.ClearLeds();
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    sampler.Init(fsi.GetSDPath());
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
