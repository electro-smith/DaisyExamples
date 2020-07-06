# MusicBox
Generate random melodies.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Button 1 | Trigger Envelope | Selects new random note |
| Knob 1 | Decay Time |  |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/MusicBox/resources/MusicBox.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
//Generates next note
if(hw.button1.RisingEdge())
{
    freq = mtof(48.0f + get_new_note());
    osc.SetFreq(freq);
    env.SetTime(ADENV_SEG_DECAY, dec);
    env.Trigger();
}
```
