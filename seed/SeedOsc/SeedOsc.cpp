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
    
    //Set up knob


    //Set up button


    
    
    // Loop forever
    for(;;)
    {}
}
