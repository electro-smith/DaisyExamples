// # WavPlayer
// ## Description
// Fairly simply sample player.
// Loads the first file it encounters on the SD card ( probably with alphabetic sort ).
//
// Play .wav file from the SD Card.
//
// ozh Feb 2022 - port to Patch_sm hardware, specifically the Patch.init() Eurorack module
//     Controls: 
//          Toggle switch will 1) start playing the selected .WAV file
//                             2) control whether the file loops at its end
//          Push button will restart playing the sample (upon release of the button).
//          Gate 1 input will restart playing the sample (upon the rising edge of the gate).
// 
//     limitations: no file selection mechanism.  Only the first file (ASCII name order) will be played.
// 
//     TODO:
//            use a knob to select between (upto) 16 (mono 48kHz) .WAV files on the SD card (refernce USE_ENCODER code)

#include <stdio.h>
#include <string.h>

#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM hw;

SdmmcHandler   sdcard;
FatFSInterface fsi;
WavPlayer      sampler;

/** Switch objects */
Switch toggle;  // toggle switch
Switch button;  // push button

// switch off encoder because the Patch.init() does not have one
#ifndef USE_ENCODER
//#define USE_ENCODER
#endif
static bool gate_state = false, prev_gate_state = false;

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
#ifdef USE_ENCODER
    int32_t inc;
#endif

    // Debounce digital controls
    hw.ProcessDigitalControls();

// no encoder on Patch_sm, so none of this encoder code applies
// ozh - only the first file encountered on the SD card will be played.
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

#define AUDIO_CHANNELS 2
    for(size_t i = 0; i < size; i += AUDIO_CHANNELS)
    {
        out[i] = out[i + 1] = s162f(sampler.Stream()) * 0.5f;
    }
}


int main(void)
{
    // Init hardware
    size_t blocksize = 4; // could make this 4 or 48, not critical here, 
    // cuz callback is very lightweight relative to the SD card read ( .Prepare() ) in the "while" loop below

    hw.Init();

    /* Initialize the switch
	 - We'll read the switch on pin B8
	*/

    static bool state, prev_state;
    toggle.Init(hw.B8);

    /* Initialize the switch
	 - We'll read the switch on pin B7
	*/
    button.Init(hw.B7);

    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd_cfg.speed = SdmmcHandler::Speed::MEDIUM_SLOW; //MEDIUM_SLOW; SLOW;// OZH set it to slower speed for debugging
    sdcard.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    sampler.Init(fsi.GetSDPath());
    sampler.SetLooping(false);  // set to false to play just once

    // SET LED to indicate Looping status. // ozh this works to show Looping status on the "boot" led on the Daisy Seed
    hw.SetLed(sampler.GetLooping());

    // Init Audio
    hw.SetAudioBlockSize(blocksize);
    hw.StartAudio(AudioCallback);

    // Loop forever...
    for(;;)
    {
        // Prepare buffers for sampler as needed
        sampler.Prepare();

        /** Debounce the button */
        button.Debounce();
        if(button.FallingEdge())
        {
            sampler.Restart();
        }
        else
        {
            // detect Rising Edge
            prev_gate_state = gate_state;
            gate_state      = hw.gate_in_1.State();
            /** Get the current gate in state */
            if((gate_state)
               && (gate_state != prev_gate_state)) // gate is high, prev was low
            {
                sampler.Restart();
            }
        }
        /** Debounce the switch */
        toggle.Debounce();

        /** Get the current toggle state */
        prev_state = state; // see if it changed
        state = toggle.Pressed(); // flipped up

        if(state != prev_state)
        { // toggle detected
            sampler.SetLooping(state); // loops if Patch.init() toggle switch is up
            /** Set the onboard led to the current state */
            hw.SetLed(state);
        }
    }
}
