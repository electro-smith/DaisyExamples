# Description

Blinks the Seed's onboard LED at a constant rate.

# Code Snippet  
```cpp

// Loop forever
for(;;)
{
    // Set the onboard LED
    hardware.SetLed(led_state);
     // Toggle the LED state for the next time around.
    led_state = !led_state;
     // Wait 500ms
    dsy_system_delay(500);
}
```
