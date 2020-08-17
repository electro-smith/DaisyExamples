# Description
Distorts incoming signal

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Knob 1 | Gain  | More gain = more distortion |
| Knob 2 | Dry/Wet control |
| In 1 | Distortion In | |
| Out 1 - 2 | Distortion Output | |

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
