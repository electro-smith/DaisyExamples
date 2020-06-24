# Pod Encoder

Simple example for Daisy Pod showing Encoder use.

Turning the Encoder cycles through which LED/color is active.

Clicking the Encoder sets LEDs to OFF (color 5) on press.

# Code snippet
```cpp
// Debounce the Encoder at a steady, fixed rate.
hw.Encoder.Debounce();
inc = hw.Encoder.Increment();
// Change the selected LED based on the increment.
if (inc > 0)
{
    led_sel += 1;
    // Wrap around
    if (led_sel > NUM_COLORS - 1)
    {
        led_sel = 0;
    }
}
```