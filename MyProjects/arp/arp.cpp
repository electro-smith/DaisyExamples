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
    Switch button2;
    Switch button3;
    Switch button4;
    Switch button5;
    //Set button to pin 28, to be updated at a 1kHz  samplerate
    // make thsi an array to loop through later
    button1.Init(hw.GetPin(28), 1000);
    button2.Init(hw.GetPin(27), 1000);
    button3.Init(hw.GetPin(26), 1000);
    button4.Init(hw.GetPin(25), 1000);
    button5.Init(hw.GetPin(24), 1000);

    // Loop forever
    for(;;)
    {
        //Debounce the button
        button1.Debounce();
        button2.Debounce();
        button3.Debounce();
        button4.Debounce();
        button5.Debounce();
        //If the button is pressed, turn the LED on
        // add if block to determine which button status to pass to setled
        bool buttonPressed = button1.Pressed() || button2.Pressed() || button3.Pressed() || button4.Pressed() || button5.Pressed();
        hw.SetLed(buttonPressed);
        //wait 1 ms
        System::Delay(1);
    }
}
