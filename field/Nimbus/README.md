# Nimbus

## Author

Ported by Gregor Noriskin to the Field from the port by Ben Sergentanis

Originally by Emilie Gillet

## Description

Nimbus is a port of Mutable Instrument's Clouds. Clouds is a granular
audio processor, specializing in making huge clouds of sound from even the tiniest source. Nimbus for Field uses all 8 knobs to control Clouds' parameters. Values of each parameter are shown on the OLED   when the value is changed, or by navigating to the specific parameter page with SW1 and SW2. Nimbus for Field also uses the keys to select the current Playback Mode and Quality, and engage freeze, silence, bypass and shift. Shift is used to allow for the adjustment of additionnal parameters using the knobs. Reverb is currently the only parameter that requires shift to change. Because it shares a knob with the Feedback parameter, it will also affect that parameter when shift is turned off (though it can just be set back to the original value and Reverb will stay where you set it). Given that the Field only has one Gate In, which is used to trigger Clouds, CV1 In is used to trigger freeze (with a voltage of 3.5V or above).

Originally ported by Ben Sergentanis from [pichenettes/eurorack](https://github.com/pichenettes/eurorack)

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| Knob 1 | Change Position Parameter | |
| Knob 2 | Change Size Parameter | |
| Knob 3 | Change  Parameter | |
| Knob 4 | Change DensityParameter | |
| Knob 5 | Change Texture Parameter | |
| Knob 6 | Change Dry/Wet Parameter | |
| Knob 7 | Change Stereo Spread Parameter | |
| Knob 8 | Change Feedback Parameter | |
| Knob 8 + Shift | Change Reverb Parameter | |
| SW 1 | Navigate to previous Parameter Page | |
| SW 2 | Navigate to next Parameter Page | |
| KEY A1 | Select Granular Playback Mode | |
| KEY A2 | Select Pitch Shift/Time Stretch Playback Mode | |
| KEY A3 | Select Looping Delay Playback Mode  | |
| KEY A4 | Select Spectral Processor Playback Mode  | |
| KEY A8 | Toggle Shift | Lit when on |
| KEY B1 | Select 16 bit Stereo Playback Quality | |
| KEY B2 | Select 16 bit Mono Playback Quality | |
| KEY B3 | Select 8bit u-law Stereo Playback Quality | |
| KEY B4 | Select 8bit u-law Mono Playback Quality | |
| KEY B6 | Toggle Freeze | Lit when on |
| KEY B7 | Toggle Silence | Lit when on |
| KEY B8 | Toggle Bypass | Lit when bypassed |
| Gate In 1 | Cloud Gate In | |
| CV In 1 | Freeze Gate In | Triggers at 3.5V |
| Audio In 1 | Left In 1 | |
| Audio In 2 | Right In 1 | |
| Audio Out 1 | Left Out 1 | |
| Audio Out 2 | Right Out 1 | |

## Display

The display will automatically jump to the page that contains the parameter you are currently editing, and you can navigate between pages using SW1 and SW2.

### Parameters Page 1

This page contains bars indicating the values of:

- position
- size
- pitch

### Parameters Page 2

This page contains bars indicating the values of:

- density
- texture
- dry/wet

### Parameters Page 3

This page contains bars indicating the values of:

- stereo spread
- feedback
- reverb

The code also contains a mechanism for showing the parameters as integer values between 0 and 100.

### Buttons Page 1

This page shows the current setting for Playback Mode and Quality.

### Buttons Page 2

This page shows the current toggeled state of:

- shift
- freeze
- silence
- bypass

Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) for more information on these controls.

#### Freeze

Freeze can be forced on by hitting the B6 key. Freeze will stay on until the key is hit again or if CV1 is connected and a voltage below 3.5v is detected. Unlike Ben's port, CV1 overrides the button setting.

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
