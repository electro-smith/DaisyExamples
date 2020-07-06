# Description
Stereo Reverb algorithm.

# Controls
Control 1 is drywet.  
Control 2 is the amount of the input to send to the reverb.  
Audio ins 1 and 2 are left and right input respectively.  
Audio outs 1 and 2 are left and right dry/wet mixed outputs.  
Audio outs 3 and 4 are left and right wet only outputs.

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/verb/resources/verb.png" alt="verb.png" style="width: 100%;"/>

# Code Snippet
```cpp
// read some controls
drylevel = patch.GetCtrlValue(patch.CTRL_1);
send     = patch.GetCtrlValue(patch.CTRL_2);

// Read Inputs (only stereo in are used)
dryL = in[0][i];
dryR = in[1][i];

// Send Signal to Reverb
sendL = dryL * send;
sendR = dryR * send;
verb.Process(sendL, sendR, &wetL, &wetR);

// Dc Block
wetL = blk[0].Process(wetL);
wetR = blk[1].Process(wetR);

// Out 1 and 2 are Mixed 
out[0][i] = (dryL * drylevel) + wetL;
out[1][i] = (dryR * drylevel) + wetR;

// Out 3 and 4 are just wet
out[2][i] = wetL;
out[3][i] = wetR;
```
