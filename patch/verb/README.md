# Description
Stereo Reverb algorithm.

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | Dry/Wet | Dry / Wet Control for the Mix outputs |
| Ctrl 2 | Input Level | How much of the signal to send to the reverb |
| Ctrl 3 | Feedback / Rev Time | Controls the length of the reverb tail |
| Ctrl 4 | Lowpass / Color | The higher the brighter |
| Audio In 1-2 | Reverb In | Stereo: In 1 L, In 2 R |
| Audio Out 1-2 | Mix Outputs | Stereo: Out 1 L, Out 2 R |
| Audio Out 3-4 | Wet Outputs | Stereo: Out 3 L, Out 4 R |

# Code Snippet
```cpp
// read some controls
drylevel = patch.GetCtrlValue(patch.CTRL_1);
send     = patch.GetCtrlValue(patch.CTRL_2);
verb.SetFeedback(patch.GetCtrlValue(patch.CTRL_3) * .94f);
verb.SetLpFreq(lpParam.Process());

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
