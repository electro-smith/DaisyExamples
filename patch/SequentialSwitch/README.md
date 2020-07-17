# Description
Switches mutliple inputs to one output, or one output to multiple inputs.
Clock input to trigger switch and reset in.
Selectable one to many, or many to one modes.

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Gate In 1 | Increment Step | Loops back to beginning after last step |
| Gate In 2 | Reset | Resets back to first step |
| Encoder Press | Mode Select | |
| Single In -> Multi Out | | Audio In 1 is sent to one of the four audio outs |
| Multi In -> Single Out | | One of the four audio ins is sent to Audio Out 1 |
| Audio Ins | Multi / Single In | |
| Audio Outs | Multi / Single Out | |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/SequentialSwitch/resources/SequentialSwitch.png" alt="SequentialSwitch.png" style="width: 100%;"/>

# Code Snippet
```cpp
for (size_t i = 0; i < size; i ++)
{
    out[0][i] = out[1][i] = out[2][i] = out[3][i] = 0.f; //reset non active outs
    if (isMultiIn)
        out[0][i] = in[step][i];
    else
        out[step][i] = in[0][i];
}
```