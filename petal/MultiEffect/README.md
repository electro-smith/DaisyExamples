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
| Encoder Turn | Dry/Wet | |
| Encoder Press | Effect Dry/Wet | Select which effect's dry/wet to change, or master dry/wet control |
| Ring Leds | Dry/Wet Indicator | Crush = Red, Wah = Green, Delay = Blue, Reverb = Purple, Master = White |
| Footswitches | Effect On | Delay, Reverb, Crush, AutoWah |
| Audio In | Effects In | |
| Audio Out | Effects Out | |

# Diagram

<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/petal/MultiEffect/resources/Petal_MultiEffect_Diagram.png" alt="MultiDelay.png" style="width: 100%;"/>


# Sound Demo

<iframe width="560" height="315" src="https://www.youtube.com/embed/MzivmxIsOcY" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen></iframe>

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
		
		for (int eff = 0; eff < REV; eff++)
		{
			float oldSigL = sigl;
			float oldSigR = sigr;
			
			if(effectOn[eff])
			{
				GetSample(sigl, sigr, (effectTypes)eff); 
			}
			
			sigl = sigl * dryWet[eff] + oldSigL * (1 - dryWet[eff]);
			sigl = sigr * dryWet[eff] + oldSigR * (1 - dryWet[eff]);
		}
	
		float verbl = sigl * dryWet[REV];
		float verbr = sigr * dryWet[REV];
		GetReverbSample(verbl, verbr);
		
		out[i]   = sigl * dryWet[ALL] + in[i]     * (1 - dryWet[ALL]) + verbl;
		out[i+1] = sigr * dryWet[ALL] + in[i + 1] * (1 - dryWet[ALL]) + verbr;
    }
}
```


