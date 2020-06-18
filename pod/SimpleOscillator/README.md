# SimpleOscillator 
**Author:** Andrew Ikenberry

**Date:** May 2020

**Hardware:** Daisy Pod

## Description: 
Example showing a basic SimpleOscillator with controls.

## Controls:

**Pot 1:** Coarse frequency 10Hz - 12kHz

**SW 1:** Octave down

**SW 2:** Octave up

**Encoder:** Waveform choice (sine, triangle, band-limited sawtooth, band-limited square)

# Code Snippet

    //Process controls
    if(hw.button2.RisingEdge())
        octave++;
    if(hw.button1.RisingEdge())
        octave--;
    octave = DSY_CLAMP(octave, 0, 4);

    // convert MIDI to frequency and multiply by octave size
    freq = mtof(p_freq.Process() + (octave * 12));
    osc.SetFreq(freq);
    