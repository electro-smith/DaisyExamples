# Description
White noise with two resonant filters.

# Controls

| Control | Description |
| --- | --- |
| Ctrl 1 | Lowpass Cutoff |
| Ctrl 2 | Lowpass Resonance |
| Ctrl 3 | Highpass Cutoff |
| Ctrl 4 | Highpass Resonance|
| Audio Outs | Filtered Noise Out |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/Noise/resources/Noise.png" alt="Noise.png" style="width: 100%;"/>

# Code Snippet
```cpp
static void AudioCallback(float **in, float **out, size_t size)
{
    patch.UpdateAnalogControls();

    lowpass.UpdateControls();
    highpass.UpdateControls();

    for (size_t i = 0; i < size; i += 2)
    {
        float sig = noise.Process();
        sig = lowpass.Process(sig);
        sig = highpass.Process(sig);

        for (size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = sig;
        }
    }
}
```