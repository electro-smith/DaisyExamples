# Description
Three delay bundled in one. Set their delay times independently. Feedback is controlled globally.
The three delays mixed with the dry signal is available at all outputs.
Set the dry/wet amount of this final mix with the encoder.

# Control

| Control | Description | Comment |
| --- | --- | --- |
| Ctrls 1 - 3 | Delay Time | Time for delays 1 through 3 |
| Ctrl 4 | Feedback | Feedback on all three delays |
| Encoder | Dry/Wet | Set the dry/wet amount for the outputs |
| Audio In1 | Audio input | |
| Audio Out 1 - 3 | Individual Delay Outs | Outputs for delays 1 - 3. Dry/wet works here. |  
| Audio Out 4 | Mix Out | A mix of all three delays. Dry/wet works here as well. |

# Code Snippet

```cpp
float UpdateDelay(delay &delay, float in)
{
    float read = delay.del->Read();
    delay.del->Write((feedback * read) + (1 - feedback) * in);
    return read;
}

    ....

//update delayline with feedback
for (int d = 0; d < 3; d++)
{
    mix += UpdateDelay(delays[d], in[0][i]);
}

//apply drywet and attenuate
float fdrywet = (float)drywet / 100.f;
mix = ((fdrywet * mix * .3f) + ((1.0f - fdrywet) * in[0][i]));

```