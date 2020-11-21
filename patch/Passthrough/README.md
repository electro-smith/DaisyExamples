# Description
Plucked string synthesizer into lush delay and reverb algorithms.
Really easy to create unique sonic spaces!

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | String Pitch | |
| Ctrl 2 | String decay time | |
| Ctrl 3 | Delay Time | |
| Ctrl 4 | Delay Feedback | |
| Gate In 1 | Trigger In | Plucks the string |
| Outs 1-2 | Stereo Outputs | Out 1:L, Out 2: R |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/Passthrough/resources/Passthrough.png" alt="Passthrough.png" style="width: 100%;"/>

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
