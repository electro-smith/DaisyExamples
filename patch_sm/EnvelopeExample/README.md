# EnvelopeExample

This example demonstrates using the daisy for an ADSR envelope

This example shows off the ability to output audio rate control voltage signals from the CV output pins of the dac.

Controls:

* CV input 0 - Attack time
* CV input 1 - Decay time
* CV input 2 - Sustain level
* CV input 3 - Release time
* Pin B10 - Gate Input
* Pin B7 - Button input

## TODO

The DAC configuration should be able to be wrapped in DaisyPatchSM functions

Not sure exactly what we want this to look like. Obviously, the polling version: `dac.WriteValue(DacHandle::Channel::ONE, value)` is a lot easier, 
but depending on the output frequency you'll start getting the quantization error sound on the output signals, and with out a real time base doing anything with
the DaisySP moddules won't work as expected.

Mostly we'll just have to decide on what we want the Init function to look like, and then we can put the DacHandle, buffer, and start function in the board support file.

At its simplest, this can end up looking like:

```c++
hw.InitDac(DaisyPatchSM::CVOUT_1);
hw.StartDac(EnvelopeCallback);
```

Debatably we could even set it up so that if that argument is NULL, or doesn't have an argument it runs a secret callback that just writes a value that can be written to with setters.

Something like:

```c++

hw.InitDac(); /**< equivalent of hw.InitDac(nullptr); */

for (;;) {
  hw.CvWrite(DaisyPatchSM::CVOUT_1, value);
}


/** under the hood */
DaisyPatchSM::CvWrite(CvChannel chn, uint16_t value)
{
  cvout_1_val_ = value;
}

void InternalDacCallback(float **output, size_t size) 
{
  int i = 0;
  while(size--)
    output[0][i++] = cvout_1_val_;
}

```

we could even smooth it out in the internal callback to remove the aforementioned issues with quantization error.

All that said, doing the DaisySP modules in the dac will still need some sort of actual time base in order to be accurate. So we can decide best path forward here after some discussion.
