# Description
Simple effects for incoming audio. Includes reverb, delay, downsampling, and autowah.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Knob 1 | Reverb LP Freq | 20Hz - 10kHz |
| Knob 2 | Reverb Time |  |
| Knob 3 | Delay Time |  |
| Knob 4 | Delay Amount |  |
| Knob 5 | Crush Amount |  |
| Knob 6 | Wah Amount |  |
| Encoder | DryWet | Ring leds : brighter = more wet|
| Footswitches | Effect On | Reverb, Delay, Crush, AutoWah
| Audio In | Effects In | |
| Audio Out | Effects Out | |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/MultiEffect/resources/MultiEffect.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
void AudioCallback(float *in, float *out, size_t size)
{
    Controls();
    
    //audio
    for (size_t i = 0; i < size; i += 2)
    {
        float sigl = in[i];
        float sigr = in[i + 1];
		
		if(effectOn[0])
			GetCrushSample(sigl, sigr);
		if(effectOn[1])
			GetWahSample(sigl, sigr);
		if(effectOn[2])
			GetDelaySample(sigl, sigr);		
		if(effectOn[3])
			GetReverbSample(sigl, sigr);
	
		out[i]   = sigl * dryWet + in[i] * (1 - dryWet);
		out[i+1] = sigr * dryWet + in[i + 1] * (1 - dryWet);
    }
}
```


