// # WavPlayer
// ## Description
// Fairly simply sample player.
// Loads up to 16 WAV files.
//
// The encoder moves from file to file, and SW1
// will trigger resetting the file playback
//
// Play .wav file from the SD Card.
//
#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"

using namespace daisy;

const size_t kMaxFiles = 16;

DaisyPod             hw;
SdmmcHandler         sdcard;
FatFSInterface       fsi;
FileTable<kMaxFiles> file_table;
WavPlayer<16384>     sampler;

WavFormatInfo selected_file_info;
int           selected_file_index = 0;

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    // Debounce digital controls
    hw.ProcessDigitalControls();

    // Trigger file change with encoder
    // Actual change will occur in main()
    int inc = hw.encoder.Increment();
    selected_file_index += inc;
    if(selected_file_index < 0)
    {
        selected_file_index = 0;
    }
    else if(selected_file_index
            > static_cast<int>(file_table.GetNumFiles()) - 1)
    {
        selected_file_index = file_table.GetNumFiles() - 1;
    }

    if(hw.button1.RisingEdge())
    {
        sampler.Restart();
    }

    for(size_t i = 0; i < size; i += 2)
    {
        /** `out` is interleaved in this case so we can pass it directly.
         *  with the typical non-interleaved callbacks, you would need to create an
         *  array of `float samps[2] = {out[0][i], out[1][i]}`
         */
        sampler.Stream(&out[i], 2);
    }
}


int main(void)
{
    // Init hardware
    size_t blocksize = 48;
    hw.Init();
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    // Load files from root directory of SD Card
    file_table.Fill(fsi.GetSDPath(), ".wav");

    if(file_table.GetNumFiles() == 0)
    {
        while(1)
        {
            // Blink Seed LED fast to indicate lack of files
            hw.seed.SetLed((System::GetNow() & 127) < 63);
        }
    }

    sampler.Init(file_table.GetFileName(0));
    sampler.SetLooping(true);

    // Init Audio
    hw.SetAudioBlockSize(blocksize);
    hw.StartAudio(AudioCallback);

    int file_idx = selected_file_index;
    for(;;)
    {
        if(file_idx != selected_file_index)
        {
            file_idx = selected_file_index;
            sampler.Open(file_table.GetFileName(selected_file_index));
            sampler.Restart();
        }

        // Prepare buffers for sampler as needed
        sampler.Prepare();
    }
}
