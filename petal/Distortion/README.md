# Description
Distorts incoming signal. There are two stages, a soft clip and a hard clip. Turn on one or both.  
Signal path is: (pregain) -> (soft clip) -> (gain) -> (hard clip) -> (output)  

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Knob 3 | Pregain| Give the signal a small boost |
| Knob 4 | Gain | Give the signal a BIG boost | 
| Knob 5 | Dry/wet Control | |
| Footswitch 1 | Turn soft distortion on/off | |
| Footswitch 2 | Turn hard distortion on/off | |
| Leds | If the LED is on, that distortion stage is on | |
| In 1 -2  | Stereo Distortion In | |
| Out 1 - 2 | Stereo Distortion Out | |

# Code Snippet  
```cpp    
for(size_t i = 0; i < size; i ++)
{	
	float wet = in[0][i];
		
	wet *= gain;
	wet = clip(wet);
	
	if (wet > 0)
		out[0][i] = out[1][i] = 1 - expf(-wet);
	else
		out[0][i] = out[1][i] = -1 + expf(wet);
	
	out[0][i] = wet * drywet + in[0][i] * (1 - drywet);
}
```
