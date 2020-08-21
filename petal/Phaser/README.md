# Verb

Phaser Effect.

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Knob 1 | Phaser Amount | |
| Knob 2 | Rate | 0Hz - 10Hz |
| Switch 1 | Pass Thru | Led On = No effect |
| In 0 - 1 | Stereo In | |
| Out 0 - 1 | Stereo Out | |

# Code Snippet
```cpp
void callback(float **in, float **out, size_t size)
{
	float depth;
	ProcessControls(depth);
	
    for (size_t i = 0; i < size; i++)
    {
		float lfoSignal = lfo.Process() + 2500; //0Hz - 5kHz

		for (int chn = 0; chn < 2; chn++)
		{
			if (! bypass)
			{
				in[chn][i] += depth * UpdateFilters(in[0][i]);
				in[chn][i] *= .25f;
			}
			
			out[chn][i] = in[chn][i];
		}
    }
}
```