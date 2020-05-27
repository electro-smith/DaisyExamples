// # SimpleAdc
//
// ## Description
//
// Uses a Potentiometer connected to Pin D21 to control how fast the led will blink
//
// LED will be fasted with voltage of pot wiper closest to GND
//
//
// ## Wiring Guide
//
// Connect Wiper of pot to pin D15
//
// Connect other legs of pot to GND and 3v3_A respectively.
//
#include "daisy_seed.h"

using namespace daisy;

DaisySeed     seed;  // access to hardware
AnalogControl knob1; // for demonstration purposes
float         ms;    // for blink rate

// Processing the knob at 1kHz (48kHz / 48 samples per callback)
void AudioCallback(float *in, float *out, size_t size)
{
    // Read from Analog Control
    ms = 20.f + (knob1.Process() * 200.f);
}

int main(void)
{
    bool             ledstate;
    AdcChannelConfig knob_cfg;

	// Setup/Initialize the Hardware.
    seed.Configure();
    seed.Init();

	// Set Initial Variable Values
    ledstate = false;
    ms       = 20.f;

    // Potentiometer connected to Pin 15 of the daisy
    knob_cfg.InitSingle(seed.GetPin(15));

    // Init ADC with pointer to knob_cfg and number of channels being used.`=:w
    seed.adc.Init(&knob_cfg, 1);

    // Set up Analog Control with built-in filtering, etc.
    // Takes pointer to the ADC Channel data, and a sample rate.
    knob1.Init(seed.adc.GetPtr(0), 1000.0f);

    // Start ADC and Audio Callback
    seed.adc.Start();
    seed.StartAudio(AudioCallback);
    // Loop
    while(1)
    {
        // Blink LED faster or slower based on knob position.
        dsy_system_delay(ms);
        seed.SetLed(ledstate);
        ledstate = !ledstate;
    }
}
