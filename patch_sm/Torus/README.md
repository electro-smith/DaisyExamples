# DaisyMutations Torus

Mutable Instruments Rings for the Daisy Patch.

Ported by: Ben Sergentanis  
Originally by: Émilie Gillet

Please refer to the [Rings manual](https://pichenettes.github.io/mutable-instruments-documentation/modules/rings/manual/) for more detail on everything Rings.

## Quickstart

Patch a gate signal to Gate In 1 (on first boot the strum is normalized so that it's easy to get going.). You should now hear sound when the gate
triggers. You may also want to patch a v/oct signal to CV 5 to change the pitch
(you should follow the calibration procedure below for correct 1v/oct tracking).

## Controls and I/O

| Control | Normal                                          | Shift                                                                                           | Normalize            |
| ------- | ----------------------------------------------- | ----------------------------------------------------------------------------------------------- | -------------------- |
| Knob 1  | Structure                                       | Frequency                                                                                       | Strum (gate) patched |
| Knob 2  | Brightness                                      |                                                                                                 | Note (v/oct) patched |
| Knob 3  | Damping                                         |                                                                                                 | Exciter patched      |
| Knob 4  | Position                                        |                                                                                                 |                      |
| Button  | Model (hold for 3 seconds to toggle easter egg) | Polyphony (hold for 3 seconds to enter normalize settings, hold 5 seconds to enter calibration) |                      |
| Switch  | Shift                                           |                                                                                                 |                      |
|         |                                                 |                                                                                                 |                      |

### Inputs/outputs

When patching CV or exciter inputs the normalization must be set using the
normalize mode (see below).

- Gate In 1: Strum (Trigger In)
- CV 5: V/Oct (See the calibration section below for correct 1v/oct tracking.)
- CV 6: Structure
- CV 7: Brightness
- CV 8: Position
- Audio In 1: External Exciter In
- Audio Out 1: Left Out
- Audio Out 2: Right Out

### Knobs

In the normal mode the knobs control: Structure, Brightness, Damping and
Position. When the shift switch is on the Structure knob changes to Frequency.

### Model

The resonator model to be used. These options are accessed by
pressing the button when the shift switch is off.
The models are:

- Modal
- Sympathetic Strings
- Inharmonic Strings
- FM Voice
- Western Chords
- String and Reverb

### Polyphony

How many voices can play at once. Polyphony modes 2 and 4 cause
every other note to hardpan left and right. These options are accessed by
pressing the button when the shift switch is on.
The polyphony modes are:

- One
- Two
- Four

### Easter Egg

To toggle the easter egg mode turn the shift switch to off and hold the mode
button for 3 seconds (at which point the LED will flash.)
The easter egg mode can be changed by pressing the button.
The easter egg modes are:

- Formant
- Chorus
- Reverb
- Formant 2
- Ensemble
- Reverb 2

### Normalize

This mode simulates the normalization detection used on Rings. For proper behavior you **must** turn on normalization for each of the controls
you have patched. These settings will get restored at power on.

To enter the normalize mode turn the shift switch to on and hold the mode
button for 3 seconds (at which point the LED will flash.) Turn the knobs to
normalize each input. Turning a knob to the right hand side (past
the middle point) will turn normalizing on. Turning a knob to the left will turn
it off.

To exit the normalizing mode press the button again.

Note: Each time you enter the normalizing mode all knob positions will be read,
not just those that you change.

The normalize controls are:

- Knob 1: Strum
- Knob 2: Note
- Knob 3: Exciter

### Calibration

This mode calibrates the 1v/oct input. The calibration process is as follows:

1. To enter the calibration mode turn the shift switch to on and hold the mode
   button for 5 seconds. At 3 seconds the LED will begin to flash for the
   normalization mode and at 5 seconds it will stay lit, at which point releasing
   the button will begin calibration.

2. Once in the calibration mode the LED will flash slowly. Patch in a 1v signal
   (C1 on a v/oct keyboard)
   to CV_5 and press the button.

3. The LED should now be flashing quickly. Patch a 3v signal (C3 on a v/oct keyboard) into CV_5 and
   press the button one more time.

Calibration is now complete. The calibration data will be stored between
power cycles.
