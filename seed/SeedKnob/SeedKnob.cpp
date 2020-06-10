#include "daisy_seed.h"

// Use the daisy namespace to prevent having to type 
// daisy:: before all libdaisy functions
using namespace daisy;

// Declare a DaisySeed object called hardware
DaisySeed hardware;

int main(void)
{
    Led led1;
    led1.Init(hardware.GetPin(28), false, 10000);

    
    
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hardware.Configure();
    hardware.Init();

    AdcChannelConfig adcConfig;
    adcConfig.InitSingle(hardware.GetPin(21));
    hardware.adc.Init(&adcConfig, 1);
    hardware.adc.Start();
    
    // Loop forever
    for(;;)
    {
        // Set the onboard LED
        led1.Set(hardware.adc.GetFloat(0));
	led1.Update();
    }
}
