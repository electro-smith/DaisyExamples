# Nimbus

## Author

Ported by Ben Sergentanis
Originally by Emilie Gillet

## Description

Nimbus is a port of Mutable Instrument's Clouds. Clouds is a granular  
audio processor, specializing in making huge clouds of sound from even the tiniest source.  
Nimbus includes a menu to allow the user to map any of the Daisy Patch's four controls to any of Clouds' parameters.  

Ported from [pichenettes/eurorack](https://github.com/pichenettes/eurorack)

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| Encoder Turn | Navigate through menu | |
| Encoder Click | Select a menu item to edit | |
| Encoder Long Press | Switch between pages one and two | |
| Gate In 1 | Freeze Gate In | Activates Freeze if not already done via menu |
| Audio In 1 | Left In 1 | All Audio Ins are a mix of In 1 and In 2 |
| Audio In 2 | Right In 1 | |
| Audio In 3 | Left In 2 | |
| Audio In 4 | Right In 2 | |
| Audio Out 1 | Left Out 1 | |
| Audio Out 2 | Right Out 1 | |
| Audio Out 3 | Left Out 2 | Copy of Out 1 |
| Audio Out 4 | Right Out 2 | Copy of Out 1 |

## Menu
You can toggle between pages one and two by long pressing on the encoder.

### Page One
This page allows you to assign one Nimbus parameter to each control on the Patch.  
These options are:
- position
- size
- pitch
- density
- texture
- dry/wet
- stereo spread
- feedback
- reverb

Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) for more information on these controls.

### Page Two
#### Freeze
Freeze can be forced on using the menu control. To do so, click freeze, and turn the encoder once. A box will be drawn around freeze. To turn freeze off, select freeze and turn the encoder again.  

Freeze can also be turned on using Gate In 1. This will turn Freeze on (if it isn't already) for the duration the gate is high. The box will also be drawn when Freeze is turned on via the gate.

#### Mode
You can select from Nimbus' four alternate modes here. These are:
- Granular
- Pitch Shift / Time Stretch
- Looping Delay
- Spectral Processor
Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) section on "The Infamous Alternate Modes".  

#### Quality
You can also select from four quality / mono stereo modes. These are:
- 16 bit Stereo
- 16 bit Mono
- 8bit u-law Stereo
- 8bit u-law Mono
Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) section on "Audio Quality".  