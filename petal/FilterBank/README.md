# Description
Fixed resonant peak filterbank with amplitude control per filter. Try running it with whitenoise, oscillators, or any sound source you please!  
There are two banks of four filters to control.  

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Encoder Turn | Control Mode | Selects which filters ctrls map to |
| Ctrl 1-4 | Filter Amplitude | Volume per filter |
| Audio In 1 | Filter bank Input | |
| Audio Out 1-4 | Filter Bank Output | |
| Leds | Brightness 1-8 shows amplitude per filter | Whichever side is purple is the side being controlled |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/FilterBank/resources/FilterBank.png" alt="FilterBank.png" style="width: 100%;"/>

# Code Snippet
```cpp
static void AudioCallback(float **in, float **out, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
	float sig = 0.f;
	for (int j = 0; j < 16; j++)
	{
	    sig += filters[j].Process(in[0][i]);
	}
	sig *= .06;
	
	out[0][i] = out[1][i] = out[2][i] = out[3][i] = sig;
    }
}
```