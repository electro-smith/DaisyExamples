# Button

## Description

Turns on the seed's onboard LED as long as a button is held.

## Breadboard

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/seed/Button/resources/Button_bb.png" alt="Button_bb.png" style="width: 100%;"/>

## Code Snippet  

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

## Schematic  

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/seed/Button/resources/Button_schem.png" alt="Button_schem.png" style="width: 100%;"/>
