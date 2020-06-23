# Description
Plucked string synthesizer into lush delay and reverb algorithms.
Really easy to create unique sonic spaces!

# Controls
Send a trigger to Gate In 1 to trigger the string.
Output can be heard on audio outs 1 and 2.
Control one controls the string's pitch.
Control two controls the string's decay time.
Control three controls delay time.
Control four controls delay feedback amount.

# Code Snippet
```cpp
// Smooth delaytime, and set.
fonepole(smooth_time, deltime, 0.0005f);
delay.SetDelay(smooth_time);

// Synthesize Plucks
sig        = synth.Process(trig, nn);

// Handle Delay
delsig     = delay.Read();
delay.Write(sig + (delsig * delfb));

// Create Reverb Send
dry        = sig + delsig;
send       = dry * 0.6f;
verb.Process(send,send, &wetl, &wetr);

// Output 
out_left[i] = dry + wetl;
out_right[i]     = dry + wetr;
```