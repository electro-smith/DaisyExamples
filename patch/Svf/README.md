# Description
State variable filter with simultaneous low pass, high pass, band pass, and notch filters.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | Cutoff Frequency | 20Hz to 20,000kHz |
| Ctrl 2 | Resonance | Controls the amount of filter resonance |
| Ctrl 3 | Drive | Gets really gritty! |
| Audio In 1 | Audio Input | Inputs 2 - 4 are inactive |
| Audio Out 1 | Low Pass Out | Passes frequencies below cutoff |
| Audio Out 2 | High Pass Out | Passes frequencies above cutoff |
| Audio Out 3 | Band Pass Out | Passes frequencies in a band, centered around cutoff |
| Audio Out 4 | Notch Out | Rejects a band of frequencies, centered around cutoff |

# Code Snippet
```cpp
for (size_t i = 0; i < size; i ++)
{
    //send the next sample to the filter
    svf.Process(in[0][i]);
    
    //send the different filter types to the different outputs
    out[0][i] = svf.Low();
    out[1][i] = svf.High();
    out[2][i] = svf.Band();
    out[3][i] = svf.Notch();
}
```