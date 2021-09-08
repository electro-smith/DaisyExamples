#include "daisy_seed.h"

// Use the daisy namespace to prevent having to type
// daisy:: before all libdaisy functions
using namespace daisy;

// Declare a DaisySeed object called hardware
DaisySeed hardware;

int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();

    Led led1;
    //Initialize led1. We'll plug it into pin 28.
    //false here indicates the value is uninverted
    led1.Init(hardware.GetPin(28), false);

    //This is our ADC configuration
    AdcChannelConfig adcConfig;
    //Configure pin 21 as an ADC input. This is where we'll read the knob.
    adcConfig.InitSingle(hardware.GetPin(21));

    //Initialize the adc with the config we just made
    hardware.adc.Init(&adcConfig, 1);
    //Start reading values
    hardware.adc.Start();

    // Loop forever
    for(;;)
    {
        // Set the onboard LED to the value we read from the knob
        led1.Set(hardware.adc.GetFloat(0));

        //Update the led to reflect the set value
        led1.Update();

        //wait 1 ms
        System::Delay(1);
    }
}
