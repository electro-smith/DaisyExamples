# Osc

## Description

Simple oscillator for daisy seed. Has button triggered envelope and knob controlled pitch.

## Breadboard

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/seed/Osc/resources/Osc_bb.png" alt="Osc_bb.png" style="width: 100%;"/>

## Code Snippet  

```cpp
//Nobody likes a bouncy button
button1.Debounce();

//If you push the button,...
if (button1.RisingEdge())
{
  env.Trigger(); //Trigger the envelope!
}

//Convert floating point knob to midi (0-127)
//Then convert midi to freq. in Hz
osc.SetFreq(mtof(hardware.adc.GetFloat(0) * 127));

//Fill the block with samples
for (size_t i = 0; i < size; i+=2)
{
    //Get the next envelope value
    env_out = env.Process();
    //Set the oscillator volume to the latest env value
    osc.SetAmp(env_out);
    //get the next oscillator sample
    osc_out = osc.Process();

    //Set the left and right outputs
    out[i] = osc_out;
    out[i+1] = osc_out;
}
```

## Schematic  

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/seed/Osc/resources/Osc_schem.png" alt="Osc_schem.png" style="width: 100%;"/>
