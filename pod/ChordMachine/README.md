# Description
A simple sine wave chord machine. Cycle through different chord types and inversions.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Knob 1 | Root pitch | Shifts the whole chord |
| Knob 2 | Inversion | 0 – 4 |
| Encoder | Rotate: Cycle through different chord types<br>Press: Reset to the first chord | |
| LED | Chord indicator | |

### Chord types
  1. Major triad, Red
  2. Minor triad, Green
  3. Augmented triad, Blue
  4. Diminished triad, White
  5. Major seven, Purple
  6. Minor seven, Light Blue
  7. Dominant seven, Orange
  8. Minor/Major seven, Yellow
  9. Diminished seven, Pink
  10. Half diminished seven, Light green

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/SynthVoice/resources/SynthVoice.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet
    void UpdateKnobs()
    {   
        int freq = (int) p_freq.Process(); 
        int inversion = (int) p_inversion.Process();
        
        notes[0] = freq + (12 * (inversion >= 1));
        notes[1] = freq + chord[chordNum][0] + (12 * (inversion >= 2));
        notes[2] = freq + chord[chordNum][1] + (12 * (inversion >= 3));
        notes[3] = freq + chord[chordNum][2] + (12 * (inversion >= 4));
    }
