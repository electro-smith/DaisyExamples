#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"

using namespace daisy;

DaisyPod       hardware;
SdmmcHandler   sdcard;
FatFSInterface fsi;
WavPlayer      wavPlayer;

size_t numberOfFiles;
size_t currentFile = 0;
bool sampleChanged = false;
bool restart = false;

void ProcessControls();

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    ProcessControls();

    float sample = 0.0f;
    for(size_t i = 0; i < size; i += 2)
    {
        sample = s162f(wavPlayer.Stream());
        out[i] = out[i + 1] = sample * 0.5f;
    }
}

void ProcessControls(){
    // Debounce digital controls
    hardware.ProcessDigitalControls();

    // Change file with encoder.
    int32_t increment = hardware.encoder.Increment();
    if(increment != 0)
    {
        int next = currentFile + increment;

        // Make sure we don't go out of bounds.
        next = std::min(next, int (numberOfFiles) - 1);
        next = std::max(next, 0);
        currentFile = size_t (next);

        sampleChanged = true;
    }

    if(hardware.button1.RisingEdge())
    {
        restart = true;
    }

    if(hardware.button2.RisingEdge())
    {
        wavPlayer.SetLooping(!wavPlayer.GetLooping());
        hardware.led2.Set(wavPlayer.GetLooping(),0,0);
    }

    hardware.UpdateLeds();     
}

int main(void)
{
    // Init hardware
    hardware.Init();
    hardware.SetAudioBlockSize(128);

    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Read from the root folder
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    wavPlayer.Init(fsi.GetSDPath());
    wavPlayer.SetLooping(true);

    // Set led1 to indicate Looping status.
    float r = wavPlayer.GetLooping();
    hardware.led2.Set(r,0,0);

    // Init Audio
    hardware.StartAdc();
    hardware.StartAudio(AudioCallback);

    // Loop forever...
    while(true)
    {
        // Prepare buffers for wavPlayer as needed
        wavPlayer.Prepare();

        numberOfFiles = wavPlayer.GetNumberFiles();

        if (sampleChanged)
        {
            wavPlayer.Close();
            wavPlayer.Open(currentFile);
            sampleChanged = false;
        }

        currentFile = wavPlayer.GetCurrentFile();
        
        if (restart){
            wavPlayer.Restart();
            restart = false;
        }
    }
}
