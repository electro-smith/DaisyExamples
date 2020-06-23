# Description
Turns on the seed's onboard LED as long as a button is held.

# Breadboard
![Button_bb.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/Button/resources/Button_bb.png)

# Code Snippet  
```cpp

// Loop forever
for(;;)
{
    //Debounce the button
    button1.Debounce();
    //If the button is pressed, turn the LED on
    hw.SetLed(button1.Pressed());
    //wait 1 ms
    dsy_system_delay(1);
}

```
# Schematic  

![Button_schem.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/Button/resources/Button_schem.png)


