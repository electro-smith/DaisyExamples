# Description

Simple Four Channel Audio Mixer.

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Ctrls | Sets input level | Each Ctrl goes to the Input of the same number. I.e. Ctrl1 goes with In1, etc. |
| Ins | Audio Inputs | Audio inputs are normalled In1 > In2 > In3 > In4 |
| Outs | Audio Outputs | The mixed signal is available on all 4 outputs |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/QuadMixer/resources/QuadMixer.png" alt="QuadMixer.png" style="width: 100%;"/>

# Code Snippet

```cpp
for (size_t i = 0; i < size; i++)
{
    float output = 0.f;
    //sum all four inputs, attenuated by the control levels
    for (size_t chn = 0; chn < 4; chn++)
    {
        output += ctrlVal[chn] * in[chn][i];
    }
    
    //attenuate by 1/4
    output *= .25f;
    
    //send the same thing to all 4 outputs
    for (size_t chn = 0; chn < 4; chn++)
    {
        out[chn][i] = output;
    }
}
```