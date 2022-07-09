# Nimbus

**Note**: This is a bootloader app. In order to run this on the daisy you first need to install the Daisy bootloader. 

To install the bootloader on the Daisy: 

1. Connect to the Daisy as usual via the Web Programmer
2. Click the advanced button in the Programming Section 
3. Click "Flash Bootloader Image"

Once the bootloader has been programmed. You can load this program to the daisy by following the following steps:

1. Press reset on the daisy, the USR LED will briefly pulse
2. While the LED is pulsing, press boot to force the daisy to stay in the bootloader. The LED will blink rapidly to indicate you are staying in the bootloader.
3. The Daisy will now appear in the list of USB devices available (As either STM32 Firmware Update, or Daisy Bootloader) 
4. Once connected, you can click program as usual.

## Author

Ported by Gregor Noriskin to the Field from the port by Ben Sergentanis

Originally by Emilie Gillet

## Description

Nimbus is a port of Mutable Instrument's Clouds. Clouds is a granular
audio processor, specializing in making huge clouds of sound from even the tiniest source. Nimbus for Field uses all 8 knobs to control Clouds' parameters. Values of each parameter are shown on the OLED   when the knob controlling that value is moved (unless it is currently CV mapped), or by navigating to the specific parameter page with SW1 and SW2. Nimbus for Field also uses the keys to select the current Playback Mode and Quality, and engage freeze, silence, bypass and shift. Shift is used to allow for the adjustment of additional parameters using the knobs. If a knob does not control an alternate parameter it will just adjust its default parameter. Reverb is currently the only parameter that requires shift to change. All knobs implement a parameter "catch" scheme, and will only start adjusting a parameter when the knob is close to what the currently set value of that parameter is. This will be noticeable when you power the Field on, when you enter and exit Shift, and when un-mapping CV. Given that the Field only has one Gate In, which is used to trigger Clouds, CV1 In is used to toggle freeze (with a voltage of 3.5V or above). The code includes a mechanism to only freeze when the voltage is high, as opposed to toggling freeze. This port also allows you to map one or more of the parameters to CV In 2, 3 and 4. You can map as many parameters as you like to each of the CVs inputs, but cannot map a parameter to more than one CV input.

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
| KEY A5 | Hold to map and unmap parameter to CV2 | Only one parameter can be mapped or unmapped for each key press |
| KEY A6 | Hold to map and unmap parameter to CV3 | Only one parameter can be mapped or unmapped for each key press |
| KEY A7 | Hold to map and unmap parameter to CV4 | Only one parameter can be mapped or unmapped for each key press |
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

The display will automatically jump to the page that contains the parameter you are currently editing, and you can navigate between pages using SW1 and SW2. This will not happen for shift, freeze, silence or bypass.

### Splash Screen

Because sometimes you gotta be reminded what code is running on you Daisy.

### Parameters Screen 1

This screen shows the values of parameters:

- position
- size
- pitch

### Parameters Screen 2

This screen shows the values of parameters:

- density
- texture
- dry/wet

### Parameters Screen 3

This screen shows the values of parameters:

- stereo spread
- feedback
- reverb

The code also contains a mechanism for showing the parameters as integer values between 0 and 100.

### Buttons Screen 1

This screen shows the current setting for Playback Mode and Quality.

### Buttons Screen 2

This screen shows the current toggled state of:

- shift
- freeze
- silence
- bypass

Refer to the [Clouds Manual](https://mutable-instruments.net/modules/clouds/manual/) for more information on these controls.

### CV2 Parameter Mapping Screen

This screen shows the current parameters that are mapped to CV In 2.

### CV3 Parameter Mapping Screen

This screen shows the current parameters that are mapped to CV In 3.

### CV4 Parameter Mapping Screen

This screen shows the current parameters that are mapped to CV In 4.

### Scope Screen

This screen shows the current CPU utilization and a simple scope showing the current audio output.

#### Freeze

Freeze can be forced on by hitting the B6 key. Freeze will stay on until the key is hit again or if CV1 is connected and a voltage below 3.5v is detected.

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

#### CV In to Parameter Mapping

You can modulate any or all of the 9 Nimbus parameters with CV In 2, 3, and 4 (1 is reserved for Freeze). To map a parameter just hold down the key associated with the CV  you wish to map and then move the knob associated with the parameter. The CV mapping keys are not lit but will be when you hold them down. Once you have finished mapping a parameter, release the key. You can only map or unmap  one parameter for each hold of the key. To unmap a parameter just press the key again and move the knob. When a parameter is mapped, input from the knob is ignored and the LED will be dark. The display will still show changes in the parameter values though. You can see which parameters are currently mapped by navigating to each of the CV mapping pages.
