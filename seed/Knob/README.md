# Knob

## Description

Using a knob to control the brightness of an led.

## Breadboard

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/seed/Knob/resources/Knob_bb.png" alt="SeedKnob_bb.png" style="width: 100%;"/>

## Code Snippet  

```cpp
// Loop forever
for(;;)
{
    // Set the onboard LED to the value we read from the knob
    led1.Set(hardware.adc.GetFloat(0));

    //Update the led to reflect the set value
    led1.Update();

    //wait 1 ms
    dsy_system_delay(1);
}

```

## Schematic  

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/seed/Knob/resources/Knob_schem.png" alt="SeedKnob_schem.png" style="width: 100%;"/>
