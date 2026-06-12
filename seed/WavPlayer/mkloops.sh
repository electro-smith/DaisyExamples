#!/bin/bash

# generate 8 WAV files for use in the WavPlayer
echo "making files..."
sox -n -r 48000 -b 16 -c 2 loop-1.wav synth 1 sine 220 gain -6
sox -n -r 48000 -b 16 -c 2 loop-2.wav synth 1 sine 220 gain -6
sox -n -r 48000 -b 16 -c 2 loop-3.wav synth 1 sine 440 gain -6
sox -n -r 48000 -b 16 -c 2 loop-4.wav synth 1 sine 880 gain -6
sox -n -r 48000 -b 16 -c 2 loop-5.wav synth 2 sine 220-880 gain -6
sox -n -r 48000 -b 16 -c 2 loop-6.wav synth 2 sine 880-220 gain -6
sox -n -r 48000 -b 16 -c 2 loop-7.wav synth 4 sine 55-1760 gain -6
sox -n -r 48000 -b 16 -c 2 loop-8.wav synth 4 sine 1760-55 gain -6
echo "done."