#include "daisy_seed.h"

// Use the daisy namespace to prevent having to type 
// daisy:: before all libdaisy functions
using namespace daisy;

// Declare a DaisySeed object called hardware
DaisySeed hw;

int main(void)
{
    // Configure and Initialize the Daisy Seed
    // These are separate to allow reconfiguration of any of the internal
    // components before initialization.
    hw.Configure();
    hw.Init();
    
    //Configure and initialize button
    Switch button1;
    button1.Init(hw.GetPin(28), 10000);

    //hw.adc.Start();
    
    // Loop forever
    for(;;)
    {
        button1.Debounce();
	hw.SetLed(button1.Pressed());	
    }
}
