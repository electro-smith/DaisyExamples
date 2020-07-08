# Description
5 Step Sequencer with gate and step outputs.

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Gate Ins | Advance Sequence | Try using both ins at the same time! |
| Gate Out | Trigger Out | Triggers when entering an active step. X indicates active |
| CV Outs | Step Out | OLED value is in terms of percent. So 35 indicates .35 * 5 = 1.75 Volts |
| Encoder | Navigate Menu | Turn to move through steps. Press to enter/leave submenu (values), or activate/deactive step |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/Sequencer/resources/Sequencer.png" alt="Sequencer.png" style="width: 100%;"/>

# Code Snippet
```cpp
//gate in
if (patch.gate_input[0].Trig() || patch.gate_input[1].Trig())
{
    stepNumber++;
    stepNumber %= 5;
    trigOut = trigs[stepNumber];
}

......

void UpdateOutputs()
{
    dsy_dac_write(DSY_DAC_CHN1, values[stepNumber] * 819.f);
    dsy_dac_write(DSY_DAC_CHN2, values[stepNumber] * 819.f);

    dsy_gpio_write(&patch.gate_output, trigOut);
    trigOut = false;
}
```