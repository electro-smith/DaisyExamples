# Description
Simple effects for incoming audio. Includes reverb, delay, and downsampling.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Mode 1 | Reverb | Knob 1: Dry/wet. Knob 2: Reverb time |
| Mode 2 | Delay | Knob 1: delay time. Knob 2: Feedback |
| Mode 3 | Bitcrush / Lowpass | Knob 1: LPF cutoff Knob 2: Downsample |
| Encoder | Mode Select | |
| Led | Mode Indicate | 1: Red 2: Green 3: Purple | 
| Audio In | Effect In | |
| Audio Out | Effect Out | |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/MultiEffect/resources/MultiEffect.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
void GetReverbSample(float &outl, float &outr, float inl, float inr)
{
    rev.Process(inl, inr, &outl, &outr);
    outl = drywet * outl + (1 - drywet) * inl;
    outr = drywet * outr + (1 - drywet) * inr;
}

......

void GetCrushSample(float &outl, float &outr, float inl, float inr)
{
    crushcount++;
    crushcount %= crushmod;
    if (crushcount == 0)
    {
        crushsr = inr;
        crushsl = inl;
    }
    outl = tone.Process(crushsl);
    outr = tone.Process(crushsr);
}
```


