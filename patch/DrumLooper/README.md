# Description
Free looping drum machine. Record unquantized drums on three different channels.  
The first recording sets the loop length.
The recording can be launched by a trigger or encoder press.  
After the first loop, turn record back on to record more drums. There is an end of cycle output available to sync other gear.  
Try different trigger inputs! Drum pads, switches, and sequencers are all good ideas!  

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| Ctrl 1 | Kick Decay Time | |
| Ctrl 2 | Kick Pitch | |
| Ctrl 3 | Snare Decay Time | |
| Ctrl 4 | Hat Decay Time | |
| Encoder Turn | Select Channel | |
| Encoder Press | Record | The first loop sets the length. |
| Encoder long press | Reset looper | |
| Gate In 1 | Trigger to record | Starts recording in ready mode |
| Gate Out | End of Cycle | |
| Audio Out 1 | Kick Out | |
| Audio Out 2 | Snare Out |
| Audio Out 3 | Hat Out | |
| Audio Out 4 | Mix Out | |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/DrumLooper/resources/DrumLooper.png" alt="DrumLooper.png" style="width: 100%;"/>

# Code Snippet
```cpp
for (size_t i = 0; i < size; i ++){
    float outs[3]; 

    osc.SetFreq(kckPitchEnv.Process());
    osc.SetAmp(volEnvs[0].Process());
    outs[0] = osc.Process();

    float noise_out = noise.Process();
    outs[1] = noise_out * volEnvs[1].Process();

    outs[2]= noise_out * volEnvs[2].Process();

    float mix = 0.f;
    float mixLevels[] = {.5f, .1f, .05f};

    for (size_t chn = 0; chn < 3; chn++)
    {
        out[chn][i] = outs[chn];
        mix += mixLevels[chn] * outs[chn];
    }

    out[3][i] = mix;
}
```