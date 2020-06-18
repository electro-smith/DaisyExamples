# Description
Simple Synth voice example. Turning the encoder cycles through three control modes. Each mode has different knob functions.
Modes:  Led one shows mode.
In mode one (blue), knob one controls filter cutoff and knob two controls oscillator frequency.
In mode two (green), knob one 1 controls envelope attack time and knob two controls envelope decay time.
In mode three (red), knob one controls vibrato rate and knob two controls vibrato depth. Very fast vibratos are heard as simple FM synthesis!
Click the encoder to cycle through waveforms.
Press the left button to trigger the synth.
Press the right button to set the envelope to self cycle (loop).
The right led lights purple if the envelope is set to self cycle.

# Code Snippet

    //Process Samples
    float ad_out = ad.Process();
    vibrato = lfo.Process();
    
    osc.SetFreq(oscFreq + vibrato);
    
    sig = osc.Process();
    sig = flt.Process(sig);
    sig *= ad_out;
    
    ......

    