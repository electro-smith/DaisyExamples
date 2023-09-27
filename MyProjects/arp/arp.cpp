#include "daisy_seed.h"
#include "daisysp.h"

#include <vector>

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace daisysp;

// Declare a DaisySeed object called hardware
DaisySeed  hardware;
Oscillator osc;
AdEnv      env;

// Set pitches for each button
static std::vector<float> pitches = {
    466.164,
    415.305,
    369.994,
    311.127,
    277.183,
};

// Initialize buttons
Switch button1;
Switch button2;
Switch button3;
Switch button4;
Switch button5;

static std::vector<Switch> buttons = {
    button1,
    button2,
    button3,
    button4,
    button5
};

// TODO: Use hardware to set these globals
static bool arpMode = true;
static float arpSpeed = 1000; // ms 

static int counter = 0;
static size_t currentPitchIndex = 0;

// Replace with std::queue
static std::vector<float> currentPitches = {
    -1.0,
    -1.0,
    -1.0,
    -1.0,
    -1.0,
};

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
        AudioHandle::InterleavingOutputBuffer out,
        size_t                                size)
{
    float oscOut, envOut;
    
    // TODO: Refactor playback logic 
    // Reset current pitches T_T
    for (size_t i = 0; i < buttons.size(); i++) {
        currentPitches[i] = -1.0;
    }

    for (size_t i = 0; i < buttons.size(); i++) {

        if(buttons[i].Pressed()) {
            currentPitches[i] = pitches[i];
        } else {
            currentPitches[i] = -1;
        }

        buttons[i].Debounce();
    }

    counter++;
    if(counter > arpSpeed) {
        counter = 0;

        if(currentPitches[currentPitchIndex] == -1) {
            currentPitchIndex++;
        }

        if(currentPitchIndex > buttons.size()) {
            currentPitchIndex = 0;
        }

        osc.SetFreq(currentPitches[currentPitchIndex]);
        env.Trigger();

        currentPitchIndex++;
    }

    // Fill the block with samples
    for(size_t i = 0; i < size; i += 2)
    {
        // Get the next envelope value
        envOut = env.Process();
        // Set the oscillator volume to the latest env value
        osc.SetAmp(envOut);
        // Get the next oscillator sample
        oscOut = osc.Process();

        // Set the left and right outputs
        out[i]     = oscOut;
        out[i + 1] = oscOut;
    }
}


int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();
    hardware.SetAudioBlockSize(4);

    // How many samples we'll output per second
    float sampleRate = hardware.AudioSampleRate();

    // Set button to pin 28, to be updated at a 1kHz  sampleRate
    int pin = 28;
    for (size_t i = 0; i < buttons.size(); i++) {
        buttons[i].Init(hardware.GetPin(pin), sampleRate / 48.f);
        pin--;
    }

    // Set up oscillator
    osc.Init(sampleRate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetAmp(1.f);

    // Set up volume envelope
    env.Init(sampleRate);
    // Envelope attack and decay times
    env.SetTime(ADENV_SEG_ATTACK, .01);
    env.SetTime(ADENV_SEG_DECAY, .4);
    // Minimum and maximum envelope values
    env.SetMin(0.0);
    env.SetMax(1.f);
    env.SetCurve(0); // linear

    // Start calling the audio callback
    hardware.StartAudio(AudioCallback);

    // Loop forever
    for(;;) {}
}
