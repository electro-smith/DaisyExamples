# Description
Simple effects for incoming audio. Includes reverb, delay, downsampling, and autowah.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Mode 1 | Reverb | Knob 3: Dry/wet. Knob 4: Reverb time |
| Mode 2 | Delay | Knob 3: delay time. Knob 4: Feedback |
| Mode 3 | Bitcrush / Lowpass | Knob 3: LPF cutoff Knob 4: Downsample |
| Mode 4 | Autowah | Knob 3: Dry/Wet Knob 4: Wah Amount |
| Encoder | Mode Select | |
| Led | Mode Indicate | Leds rotate around encoder in 2s to indicate mode and knob values | 
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


