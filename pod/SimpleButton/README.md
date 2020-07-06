# Description:
Example showing basic usage of pushbutton switches for momentary and latching behavior.

# Controls:
| Control | Description |
| --- | --- |
| Button 1 | Activate Led 1 |
| Button 2 | Activate Led 2 |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/SimpleButton/resources/SimpleButton.png" alt="Button_schem.png" style="width: 100%;"/>

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
