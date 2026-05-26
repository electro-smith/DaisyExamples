# 🎵 ByteShift

ByteShift is an experimental music synthesizer that generates sound using mathematical formulas known as bytebeat equations. It runs on the Daisy Patch platform, using control voltage (CV) and knob-based inputs to modify parameters in real time.

## 🔥 Features
- Bytebeat Audio Synthesis: Uses a selection of classic bytebeat formulas.
- Dynamic Formula Parameters: Modify parameters a, b, and c in real-time. Not all formulas use all all three controls, some use only two of the controls.
- Built-in Formula Switching: Cycle through 6 bytebeat equations.
- CV Input: Adjust parameters.
- Adjust Pitch in Semitones: ByteShift uses PitchShifter from the DaisySP Lib to change the pitch of output. This works positive and negative. PitcheShifter seems to have some limits when lowering pitch. At -12 semitones the sound disappears, and beyond that the picth rises. 
- The OLED display shows the values for a, b, c, pitch, and current formula. 

## 🎛️ Controls

### Control	Function

| Control | Function |
| :------ | :------- |
| CTRL 1  | (Parameter A) Adjusts the first variable (a) in the bytebeat formula. |
| CTRL 2  | (Parameter B) Adjusts the second variable (b) in the bytebeat formula. |
| CTRL 3  | (Parameter C) Adjusts the third variable (c) in the bytebeat formula. |
| Encoder (Press) | Cycles through different bytebeat formulas. |
| Encoder (Rotate) | changes the pitch it semitones. | 

## ⚡ How to Use
1. Audio is sent to out1 and out 2. 
2. Use CTRL1-3 to adjust parameters affecting bytebeat formula.
3. Rotate the encoder to change the pitch in semitones. 
4. Press the encoder to change the bytebeat formula. 

## 🛠️ Installation & Compilation

To build and flash BytebeatSynth onto your Daisy Patch:
1. Clone or copy the project files.
2. Run `make` to compile the firmware.

## 🎵 What is a Bytebeat?

A bytebeat is a type of generative music where sound is created using simple mathematical formulas instead of traditional synthesis or sampling. These formulas operate on a time variable (t) to generate an audio waveform in real time, often resulting in glitchy, chiptune-like, and chaotic sounds.

Bytebeats emerged in 2011 when Finnish developer viznut (aka Ville-Matias Heikkilä) discovered that short arithmetic expressions could create rich and evolving sound patterns. This led to an explosion of interest in algorithmic music, with musicians and programmers experimenting with different formulas to produce everything from rhythmic patterns to bizarre noise textures.

Since bytebeat synthesis is entirely formula-driven, modifying the equations can create new sonic landscapes, making it an ideal playground for experimentation in digital sound generation.
