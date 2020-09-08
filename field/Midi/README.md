# Field - MIDI

## Description

Polyphonic MIDI Synth

24 voices of polyphony. Each voice is a band-limited saw waveform running through a state variable filter.

## Controls

| Control | Description |
| --- | --- |
| Button 1 | Choke all voices |
| Knob 1 | Filter Cutoff |
| MIDI Input | Note In/Note Off messages with Simple Polyphony (no voice stealing) |

## Diagram

TODO: Add Diagram

## Code Snippet

Basic MIDI Note Message Handling

```cpp
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            // Note Off can come in as Note On w/ 0 Velocity
            if(p.velocity == 0.f)
            {
                voice_handler.OnNoteOff(p.note, p.velocity);
            }
            else
            {
                voice_handler.OnNoteOn(p.note, p.velocity);
            }
        }
        break;
        case NoteOff:
        {
            NoteOnEvent p = m.AsNoteOn();
            voice_handler.OnNoteOff(p.note, p.velocity);
        }
        break;
        default: break;
    }
}
```
