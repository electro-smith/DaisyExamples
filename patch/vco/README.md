# Description

VCO with sine, triangle, sawtooth, and square waveforms. 
Both standard and bandlimited waveforms are accessible. 

# Controls

**Knob 1** = Coarse Frequency

**Knob 2** = Waveform

**Knob 3** = Amplitude

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