#include "daisy_seed.h"
#include "daisysp.h"

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;
using namespace daisysp;

// Declare a DaisySeed object called hardware
DaisySeed  hardware;
Oscillator osc;
AdEnv      env;

// Set pitches for each button
float pitches[] = {
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

Switch buttons[] = {
    button1,
    button2,
    button3,
    button4,
    button5
};

static int NUM_BUTTONS = 5; // TODO: figure out how to get the length of the array lol

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
        AudioHandle::InterleavingOutputBuffer out,
        size_t                                size)
{
    float osc_out, env_out;

    // Debounce the buttons
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].Debounce();
    }

    // Set frequency based on which button pressed
    for (int i = 0; i < NUM_BUTTONS; i++) {
        if(buttons[i].RisingEdge()) {
            osc.SetFreq(pitches[i]);
            env.Trigger(); // Trigger the envelope!
        }
    }

    // Fill the block with samples
    for(size_t i = 0; i < size; i += 2)
    {
        // Get the next envelope value
        env_out = env.Process();
        // Set the oscillator volume to the latest env value
        osc.SetAmp(env_out);
        // get the next oscillator sample
        osc_out = osc.Process();

        // Set the left and right outputs
        out[i]     = osc_out;
        out[i + 1] = osc_out;
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
    float samplerate = hardware.AudioSampleRate();

    // Set button to pin 28, to be updated at a 1kHz  samplerate
    int pin = 28;
    for (int i = 0; i < NUM_BUTTONS; i++) {
        buttons[i].Init(hardware.GetPin(pin), samplerate / 48.f);
        pin--;
    }

    // Set up oscillator
    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetAmp(1.f);

    // Set up volume envelope
    env.Init(samplerate);
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
