# Description
Control a synth voice over Midi.

# Controls  
| Control | Description |  
| --- | --- |  
| Midi CC 1 | Filter Cutoff |  
| Midi CC 2 | Filter Resonance |  
| Audio Outs | Oscillator Output|  

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/Midi/resources/Midi.png" alt="Midi.png" style="width: 100%;"/>

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
