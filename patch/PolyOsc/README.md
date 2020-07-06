# Description

Three oscillators in one package! Make evolving chords, and shift them around!

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 - 3 | Oscillator Frequency | Controls the individual oscillators |
| Ctrl 4 | Global Frequency | Recommended to set this at 50% at first. |
| Encoder | Waveform | Global control. Cycle through Sine, Triangle, Saw, Ramp, and Square |
| Audio Out 1 - 3 | Oscillator Input | Outputs for individual oscillators |
| Audio Out 4 | Mix output | Listen to all 3 oscillators | 

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/PolyOsc/resources/PolyOsc.png" alt="PolyOsc.png" style="width: 100%;"/>

# Code Snippet
```cpp
for (size_t i = 0; i < size; i ++)
    {
        float mix = 0;
        //Process and output the three oscillators
        for (size_t chn = 0; chn < 3; chn++)
        {
            float sig = osc[chn].Process();	    
            mix += sig * .25f;
            out[chn][i] = sig;
	}
        
        //output the mixed oscillators
        out[3][i] = mix;
    }
```
