# SimpleOscillator

Basic sine wave oscillator.

This example demonstrates the both the basics of setting up an oscillator as well as using calibrated pitch tracking for accuracy with 1V/Octave signals.

To keep this example on the simpler side, the calibration itself is handled in another program.

See the `patch_sm/VoctCalibration` example, which will go through the process of calibrating the CV_5 input.

## Controls

| Pin Name | Pin Location | Function | Comment |
| --- | --- | --- | --- |
| CV_1 | C5 | Coarse Tuning | Sets the pitch of the oscillator |
| CV_5 | C6 | 1V/Octave Input | Musical pitch tracking input |
