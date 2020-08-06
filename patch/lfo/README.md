# Description
Simple dual LFO module. Amplitude, frequency, and waveform per lfo.  
Turn the encoder to select which LFO to edit. Click the encoder to edit that waveform.  
@ Denotes the selection has been made, and o indicates it has not.  

# Controls
| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 and Ctrl 3 | LFO Frequency |  |
| Ctrl 2 and Ctrl 4 | LFO Amplitude | From from off to 5V unipolar |
| Encoder | Waveform | Turn to select wave or lfo, click to change selection. |
| CV Outputs | LFO outs. | | 

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/lfo/resources/lfo.png" alt="lfo.png" style="width: 100%;"/>

# Code Snippet
```cpp
    for (int i = 0; i < 2; i++)
    {
        lfos[i].osc.SetFreq(lfos[i].freqCtrl.Process());
        lfos[i].osc.SetAmp(lfos[i].ampCtrl.Process());        
        lfos[i].osc.SetWaveform(lfos[i].waveform);   //waveform
    }

    ....

 	dsy_dac_write(DSY_DAC_CHN1, (lfos[0].osc.Process() + 1)/2 * 4096);
	dsy_dac_write(DSY_DAC_CHN2, (lfos[1].osc.Process() + 1)/2 * 4096);
```