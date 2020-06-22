# Description
Loops incoming audio at a user defined interval.  
Use the simple controls to record loops, play and pause them, and record over them.   
Loops can be very long, or very short.  
  
# Controls
Press button two to record. First recording sets loop length. Automatically starts looping if 5 minute limit is hit.  
After first loop sound on sound recording enabled. Press button two to toggle SOS recording. Hold button two to clear loop.  
The red light indicates record enable. The green light indicates play enable.  
Press button one to pause/play loop buffer.  
Knob one mixes live input and loop output. Left is only live thru, right is only loop output.

# Code Snippet
    void NextSamples(float &output, float* in, size_t i)
    {
        if (rec)
        {
    	    WriteBuffer(in, i);
        }
        
        output = buf[pos];
        
        ......
    
        if(play)
        {
    	    pos++;
    	    pos %= mod;
        }
    
        if (!rec)
        {
    	    output = output * drywet + in[i] * (1 -drywet);
        }
    }
