# Description
Simple effects for incoming audio. Includes reverb, delay, and downsampling.

# Controls
 Multiple effects on audio in.  
 Turn encoder to rotate through modes. (Only one effect active at a time)  
 Led brightness indicates knob position.   
   * Note: when you cycle through modes it changes their settings!!  
 1. Mode one (red) : reverb  
   * Knob one sets dry wet  
   * Knob two sets reverb time (feedback)  
 2. Mode two (green) : delay   
    * Knob one sets delay time   
    * Knob two sets feedback level / drywet amount   
 3. Mode three (purple) : downsampler  
    * Knob one sets lowpass filter cutoff  
    * Knob two sets downsample amount

# Code Snippet
    void GetReverbSample(float &outl, float &outr, float inl, float inr)
    {
        rev.Process(inl, inr, &outl, &outr);
        outl = drywet * outl + (1 - drywet) * inl;
        outr = drywet * outr + (1 - drywet) * inr;
    }

    ......

    void GetCrushSample(float &outl, float &outr, float inl, float inr)
    {
        crushcount++;
        crushcount %= crushmod;
        if (crushcount == 0)
        {
            crushsr = inr;
            crushsl = inl;
        }
        outl = tone.Process(crushsl);
        outr = tone.Process(crushsr);
    }


