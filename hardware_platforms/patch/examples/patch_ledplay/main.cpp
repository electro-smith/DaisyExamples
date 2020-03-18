// Short Example showing how to write to the LEDs on Daisy Patch
#include "daisy_patch.h"

using namespace daisy;

daisy_patch patch;

int main(void)
{
    uint8_t selected_led = 0;
    daisy_patch::led ld;
    patch.Init();
    while(1) 
    {
        dsy_system_delay(100);

        // Step through LEDs wrapping at the end 
        selected_led = (selected_led + 1) % daisy_patch::LED_LAST;

        // Cast the idx  to an led type
        ld = static_cast<daisy_patch::led>(selected_led);

        // Clear the LEDs, and set only the selected one to maximum brightness.
        patch.ClearLeds();
        patch.SetLed(ld, 1.0f);
        patch.UpdateLeds();
    }
}

