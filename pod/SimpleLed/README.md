# SimpleLed
**Author:** Andrew Ikenberry

**Date:** May 2020

**Hardware:** Daisy Pod

## Description: 
Example showing basic usage of RGB LED by mapping R/G values of LED 1 to knob.

## Controls:

**Pot 1:** Sets the R brightness value for LED 1.

**Pot 2:** Sets the G brightness value for LED 1.

**LED 1:** Changes color based on Pot 1 setting. 

# Code Snippet  
```cpp  
r = p_knob1.Process();
g = p_knob2.Process();

hw.led1.Set(r, g, b);    
hw.UpdateLeds();
```
