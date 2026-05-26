# 🎵 BytebeatSynth

BytebeatSynth is an experimental music synthesizer that generates sound using mathematical formulas known as bytebeat equations. It runs on the Daisy Patch platform, using control voltage (CV) and knob-based inputs to modify parameters in real time.

## 🔥 Features
- Bytebeat Audio Synthesis: Uses a selection of classic bytebeat formulas.
- Dynamic Formula Parameters: Modify parameters a, b, and c in real-time.
- Adjusts how fast the bytebeat algorithm runs, affecting pitch and timbre, with higher values creating higher tones and lower values slowing it down, sometimes to silence.
- Built-in Formula Switching: Cycle through multiple equations with the encoder.
- Analog & CV Input Controls: Adjust speed, parameters, and pitch using hardware knobs or external control voltages.

## 🎛️ Controls

### Control	Function
CTRL 1 (Speed/Pitch)	Controls the pitch/speed of the bytebeat equation. Accepts 0-5V CV input.
CTRL 2 (Parameter A)	Adjusts the first variable (a) in the bytebeat formula.
CTRL 3 (Parameter B)	Adjusts the second variable (b) in the bytebeat formula.
CTRL 4 (Parameter C)	Adjusts the third variable (c) in the bytebeat formula.
Encoder (Press)	Cycles through different bytebeat formulas.
OLED Display	Shows the current formula, pitch, and values of a, b, and c.

## ⚡ How to Use
1. Turn CTRL 1 to change the pitch and speed of the bytebeat synthesis.
2. Adjust CTRL 2, 3, and 4 to manipulate different formula parameters.
3. Press the encoder to switch to a different bytebeat equation.
4. Use CV input on CTRL 1 to modulate pitch from an external sequencer or LFO.
5. Observe the OLED display to see current formula and parameter values.

## 🛠️ Installation & Compilation

To build and flash BytebeatSynth onto your Daisy Patch:
1. Clone or copy the project files.
2. Run `make` to compile the firmware.
3. Flash the binary using: `st-flash --reset write build/bytebeat.bin 0x08000000`

## 🎚️ Example Patching Ideas
- Sequenced Rhythms: Use an external CV sequencer on CTRL 1 for evolving patterns.
- Glitchy Drones: Set CTRL 1 low and tweak CTRL 2, 3, 4 for texture shifts.
- Noise Percussion: Use a gate signal on CTRL 1 to trigger bursts of sound.

## 🎵 What is a Bytebeat?

A bytebeat is a type of generative music where sound is created using simple mathematical formulas instead of traditional synthesis or sampling. These formulas operate on a time variable (t) to generate an audio waveform in real time, often resulting in glitchy, chiptune-like, and chaotic sounds.

Bytebeats emerged in 2011 when Finnish developer viznut (aka Ville-Matias Heikkilä) discovered that short arithmetic expressions could create rich and evolving sound patterns. This led to an explosion of interest in algorithmic music, with musicians and programmers experimenting with different formulas to produce everything from rhythmic patterns to bizarre noise textures.

Since bytebeat synthesis is entirely formula-driven, modifying the equations can create new sonic landscapes, making it an ideal playground for experimentation in digital sound generation.
