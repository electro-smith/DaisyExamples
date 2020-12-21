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
    //Set button to pin 28, to be updated at a 1kHz  samplerate
    button1.Init(hw.GetPin(28), 1000);

    // Loop forever
    for(;;)
    {
        //Debounce the button
        button1.Debounce();
        //If the button is pressed, turn the LED on
        hw.SetLed(button1.Pressed());
        //wait 1 ms
        System::Delay(1);
    }
}
