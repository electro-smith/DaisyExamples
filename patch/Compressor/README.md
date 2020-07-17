# Description
Compressor with optional sidechain input.

# Controls

| Control | Description | Comment |
| --- | --- | --- |
| CTRL 1 | Threshold | -80 to 0 |
| CTRL 2 | Ratio | 1 to 40 |
| CTRL 3 | Attack Time | .001 to 1 |
| CTRL 4 | Release Time | .001 to 1 |
| In 1 | Audio In | |
| In 2 | Sidechain In | Only works if sidechain is on |
| Encoder Press | Sidechain On/Off | |
| Outs 1-4 | Compressor Out | |

# Diagram
<img src="https://raw.githubusercontent.com/electro-smith/DaisyExamples/master/patch/Compressor/resources/Compressor.png" alt="Compressor.png" style="width: 100%;"/>

# Code Snippet
```cpp
//encoder click
isSideChained = patch.encoder.RisingEdge() ? !isSideChained : isSideChained;

//controls
comp.SetThreshold(threshParam.Process());
comp.SetRatio(ratioParam.Process());
comp.SetAttack(attackParam.Process());
comp.SetRelease(releaseParam.Process());

........

for (size_t i = 0; i < size; i ++)
{	    
    float sig = isSideChained ? comp.Process(in[0][i], in[1][i]) : comp.Process(in[0][i]);
	
    out[0][i] = out[1][i] = sig;
    out[2][i] = out[3][i] = sig;
}
```