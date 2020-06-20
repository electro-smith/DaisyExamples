# Description
Simple 8 step sequencer. Has controls per step for envelope decay, pitch, and step activation.

# Controls
Edit and play mode, press encoder to switch modes. Edit is red, play is green.\ 

Edit mode\ 
  * Rotate encoder to run through steps.\ 
  * Knob one sets decay time.\ 
  * Knob two sets pitch. (A pentatonic major)\ 
  * Press either button to activate or deactivate a step.\ 
  * Led one tells you which step you're on (color).\ 
  * Led two is lit if the current step is active.\ 
  * When a step is activated, its envelope will cycle. Listen for pitch and envelope shape.\ 

  * In edit mode the LEDS indicate which step you're on.\ 
  * One: Red\ 
  * Two: Green\ 
  * Three:  Blue\ 
  * Four: White\ 
  * Five: Purple\ 
  * Six: Cyan\ 
  * Seven: Gold / Orange\ 
  * Eight: Yellow\ 

Play mode\ 
  * Knob one controls tempo.\ 
  * Knob two controls filter cutoff\ 
  * Turning the encoder switches waveform.

# Code Snippet
    void NextSamples(float& sig)
    {
        env_out = env.Process();
        osc.SetAmp(env_out);
        sig = osc.Process();
        sig = flt.Process(sig);
        
        if (tick.Process() && !edit)
        {
	    step++;
    	    step %= 8;
    	    if (active[step])
    	    {
    	        env.Trigger();	   
    	    }
        }
        
        if (active[step])
        {
    	    env.SetTime(ADENV_SEG_DECAY, dec[step]);
    	    osc.SetFreq(pitch[step]);
    	    if (edit && ! env.IsRunning())
    	    {
    	        env.Trigger();
    	    }
        }
    }
