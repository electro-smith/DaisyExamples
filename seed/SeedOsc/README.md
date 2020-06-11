#Description

Uses a knob and button to control a simple sine wave oscillator.
The button triggers the envelope and the knob controls the oscillator pitch.
The button is  connected between pin 28 and ground.
The knob is connected between power, ground, and pin 21.

We use the AdcChannelConfig class to create a configuration in which pin 21 is an analog input.
We then use this configuration with the daisy seed adc.
We use the Oscillator class to generate a sine wave.
An AdEnv is used to control its amplitude.

In the callback, we read the button and knob.
If the button is pressed we trigger the envelope. We set the oscillator to the frequency the knob says.
We process the envelope and oscillator at the sample rate, and calculate the next block of samples.

#Sch
![SeedOsc_schem.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/SeedOsc/resource/SeedOsc_schem.png)

#breadboard
![SeedOsc_bb.png](https://github.com/electro-smith/DaisyExamples/blob/master/seed/SeedOsc/resource/SeedOsc_bb.png)
