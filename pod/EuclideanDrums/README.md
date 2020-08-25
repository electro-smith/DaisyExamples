# Description 
Euclidean drum sequencer. Snare and kick drum, controls per drum for density, pattern length. Global tempo control.  
Since the two sequencers run independently, they can have different lengths leading to complex interlocking patterns.  

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Encoder turn | Select sequencer (snare or kick) | |
| Led 2 | Green = editing kick. Blue =  editing snare. | |
| Knob 1 | Pattern Density | 0 to 1 |
| Knob 2 | Pattern Length | 1 to 32 steps |
| Buttons | Global Tempo | Hold button one to increase tempo, and hold button two to decrease. |
| Led 1 | Tempo indicator | Red led. Brighter is faster |

# Code Snippet
```cpp
for (size_t i = 0; i < size; i+= 2)
{
    snr_env_out = snareEnv.Process();
	kck_env_out = kickVolEnv.Process();

	osc.SetFreq(kickPitchEnv.Process());
	osc.SetAmp(kck_env_out);
	osc_out = osc.Process();

	noise_out = noise.Process();
	noise_out *= snr_env_out;

	sig = .5 * noise_out + .5 + osc_out;

    out[i] = sig;
	out[i+1] = sig;
}
```