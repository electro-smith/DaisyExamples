# Description 
Dual Sample and hold or Track and hold module. Each individual circuit can be set to one of these two modes.  
Sample and hold samples, or stores the incoming signal on the rising edge of a gate or trigger.  
In other words, it only updates once per trigger.  
Track and Hold will continue tracking, or updating, as long as the gate is held high.  

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Gate Ins | Trigger / Gate In | Coorespond to circuits one and two |
| Ctrls 1-2 | S&H / T&H In | Also coorespond to circuits one and two |
| Encoder | Menu Control | Turn to cycle through menu, click to change modes. |
| CV Outs | S&H / T&H Out | Also also coorespond to circuits one and two |

# Code Snippet
```cpp
struct sampHoldStruct {
    SampleHold sampHold;
    SampleHold::Mode mode;
    float output;

    void Process(bool trigger, float input)
    {
        output = sampHold.Process(trigger, input, mode);
    }
};

....

void UpdateOutputs()
{
    dsy_dac_write(DSY_DAC_CHN1, sampHolds[0].output * 4095);
    dsy_dac_write(DSY_DAC_CHN2, sampHolds[1].output * 4095);
}
```