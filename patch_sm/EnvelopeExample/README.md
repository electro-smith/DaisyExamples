# EnvelopeExample

This example demonstrates using the daisy for an ADSR envelope

This example shows off the ability to output audio rate control voltage signals from the CV output pins of the dac.

## Controls

| Pin Name | Pin Location | Function | Comment |
| --- | --- | --- | --- |
| CV 1 | C5 | Attack Time | Range is from instant to 1 second |
| CV 2 | C4 | Decay Time | Range is from instant to 1 second |
| CV 3 | C3 | Sustain Level | Range is from 0v to 5V for sustain stage |
| CV 4 | C2 | Release Time | Range is from instant to 1 second |
| Gate In 1 | B10 | ADSR Gate | Input for engaging the envelope |
| Button | B7 | ADSR Button | Engages the envelope while pressed |
