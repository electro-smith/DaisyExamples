# Quadraphonic Mixer

## Author

C.M.Barth (allotrope ijk)

## Description 
Mixer module for stereo and quadraphonic performances that applies final reverb pass to all audio outputs.

[Source Code](https://github.com/electro-smith/DaisyExamples/tree/master/patch/QuadraphonicMixer)

## Controls
| Control | Description | Comment |
| --- | --- | --- |
| CTRL 1-4 | Input Audio Pan/Angle | When in stereo mode, CTRL values correspond to stereo panning. When in quad mode, CTRL values correspond to angles around a unit circle. Note: CTRL values map 1:1 to Audio Inputs |
| Audio Inputs 1-4 | Input Audio | 4 input audio signals. Note: for some patch modules there is cross talk across different audio input channels. Plug patch cables into all unused audio inputs to avoid signal duplication across multiple audio inputs in order to obtain cleaner panning. |
| Audio Outputs 1-4 | Output Audio | In stereo mode, the first two audio outputs are populated. In quad mode, all four audio outputs are populated. For quad mode, audio outputs correpsond to 0, 0.25f * 2PI, 0.5f * 2PI, and 0.75f * 2PI radians on a unit circle. Applying saw LFOs to CTRL values should result in circling effects when in quad mode. |
| Encoder Press | Switch between control modes | Used to toggle between stereo and quad mode. Used to update the global reverb wetness and feedback applied to all output audio channels. Used to update input audio level gains. |
