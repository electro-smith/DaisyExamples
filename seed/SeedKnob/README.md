#Description

Led is connected to pin 28 configured as an output. A resistor is included to protect the LED.
A knob is connected to pin 21, configured as an analog input. The knob is connected to 3.3V and ground to complete the circuit.

The Led class is used to control the Led on pin 28.

We create an AdcChannelConfig, which is then used to initialize the Seed's ADC.
The config defines pin 21 as an analog input. When we subsequently read ADC channel 0,
we are therefore measuring the output of the knob.

We use the Led class to set the led to the brightness specified by the knob. This is achieved via PWM internally within the Led class.

#Sch
![SeedKnob_schem.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/SeedKnob/resource/SeedKnob_schem.png)

#breadboard
![SeedKnob_bb.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/SeedKnob/resource/SeedKnob_bb.png)