# SimpleButton
**Author:** Andrew Ikenberry

**Date:** May 2020

**Hardware:** Daisy Pod

## Description:
Example showing basic usage of pushbutton switches for momentary and latching behavior.

## Controls:

**SW 1:** Configured as momentary switch for controlling LED1

**SW 2:** Configured as latching switch for controlling LED2

# Code Snippet  
```cpp
hw.DebounceControls();

// using button1 as momentary switch for turning on/off led1
brightness1 = hw.button1.Pressed();

// using button2 as latching switch for toggling led2
if(hw.button2.RisingEdge())
    brightness2 = !brightness2;

// assign brightness levels to each led (R, G, B)
hw.led1.Set(brightness1, brightness1, brightness1);
hw.led2.Set(brightness2, brightness2, brightness2);
hw.UpdateLeds();
```
