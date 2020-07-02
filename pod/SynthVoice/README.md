# Description
Simple Synth voice with resonant filter, self cycling envelope, and vibrato control.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Mode 1 | Filter / Freq | Knob 1: Cutoff, Knob 2: Osc. Freq. |
| Mode 2 | Envelope | Knob 1: Attack Knob 2: Decay |
| Mode 3 | Vibrato | Knob 1: Rate Knob 2: Depth |
| Led | Mode Indicate | 1. Blue 2. Green 3. Red |
| Turn Encoder | Mode Select | |
| Press Encoder | Waveform Select | |
| Button 1 | Trigger envelope | |
| Button 2 | Envelope Cycle | Led 2 lights purple when cycling |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/SynthVoice/resources/SynthVoice.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
//Process Samples
float ad_out = ad.Process();
vibrato = lfo.Process();

osc.SetFreq(oscFreq + vibrato);

sig = osc.Process();
sig = flt.Process(sig);
sig *= ad_out;
```
    
