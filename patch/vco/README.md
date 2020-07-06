# Description

VCO with sine, triangle, sawtooth, and square waveforms. 
Both standard and bandlimited waveforms are accessible. 

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Ctrl1 | Coarse tune | |
| Ctrl2 | Fine tune | 0 - 7 semitones |
| Ctrl3 | Waveform | Sine, tri, ramp, square, bandlimited same |
| Ctrl4 | Amplitude |  |
| Audio Outs | Oscillator Out | Available on all outputs |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/vco/resources/vco.png" alt="vco.png" style="width: 100%;"/>

# Code Snippet
```cpp
// Read Knobs
freq = mtof(freqctrl.Process());
wave = wavectrl.Process();
amp = ampctrl.Process();

// Set osc params
osc.SetFreq(freq);
osc.SetWaveform(wave);
osc.SetAmp(amp);

//Process
sig = osc.Process();
```