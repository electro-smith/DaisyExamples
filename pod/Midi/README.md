# Description
Midi Controlled oscillator with resonant LPF.  

# Controls
| Control | Description |
| --- | --- |
| CC 1 | Filter Cutoff |
| CC 2 | Filter resonance |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/pod/Midi/resources/Midi.png" alt="Button_schem.png" style="width: 100%;"/>

# Code Snippet  
```cpp  
midi.Listen();
// Handle MIDI Events
while(midi.HasEvents())
{   
    HandleMidiMessage(midi.PopEvent());
}
.......
void HandleMidiMessage(MidiEvent m)
{
switch(m.type)
{
    case NoteOn:
    {
    ......
```
