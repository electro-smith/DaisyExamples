# Description
Three delay bundled in one. Set their delay times independently. Feedback is controlled globally.
The three delays mixed with the dry signal is available at all outputs.
Set the dry/wet amount of this final mix with the encoder.

# Control

| Control | Description | Comment |
| --- | --- | --- |
| Ctrls 1 - 3 - 5| Delay Time | Time for delays 1 through 3 |
| Ctrl 2 - 4 - 5 | Feedback | Feedback for delays 1 through 3 |
| Encoder | Dry/Wet | Set the dry/wet amount for the outputs |
| Audio In 1 | Audio input | |
| Audio Out 1 - 2 | Mix Out | |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/MultiDelay/resources/MultiDelay.png" alt="MultiDelay.png" style="width: 100%;"/>

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