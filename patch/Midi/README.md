# Description
Control a synth voice over Midi.

# Controls
Midi CC 1 is for filter cutoff.
Midi CC 2 sets the filter resonance.
Audio will come out of all 4 outputs.

# Code Snippet
```cpp
switch(p.control_number)
{
    case 1:
        // CC 1 for cutoff.
        filt.SetFreq(daisysp::mtof((float)p.value));
        break;
    case 2:
        // CC 2 for res.
        filt.SetRes(((float)p.value / 127.0f));
        break;
	
        ......  
```
