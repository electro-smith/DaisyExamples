# Description: 
Example showing a basic SimpleOscillator with controls.

# Controls:
| Control | Description | Comment |
| --- | --- | --- |
| Button 1 | Octave Down | |
| Button 2 | Octave Up | |
| Knob 1 |  Coarse Frequency | Coarse frequency 10Hz - 12kHz |
| Encoder |  Waveform Select | sine, triangle, band-limited sawtooth, band-limited square |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/SimpleOscillator/resources/SimpleOscillator.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
//Process controls
if(hw.button2.RisingEdge())
    octave++;
if(hw.button1.RisingEdge())
    octave--;
octave = DSY_CLAMP(octave, 0, 4);

// convert MIDI to frequency and multiply by octave size
freq = mtof(p_freq.Process() + (octave * 12));
osc.SetFreq(freq);
```
