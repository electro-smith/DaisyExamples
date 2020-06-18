# Description
Using a knob to control the brightness of an led.

# Breadboard
![SeedKnob_bb.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/Knob/resources/Knob_bb.png)

# Code Snippet

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

# Schematic
![SeedKnob_schem.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/Knob/resources/Knob_schem.png)