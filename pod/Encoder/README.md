# Pod Encoder
Simple example for Daisy Pod showing Encoder use.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Encoder | Led Color | Turn to set color, press to turn off |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/Encoder/resources/Encoder.png" alt="Button_schem.png" style="width: 100%;"/>

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