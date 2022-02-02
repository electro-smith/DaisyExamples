# Quad Envelope

## Author

Ben Sergentanis

## Description

VCA with internal triggerable envelopes. Ins 1 and 2 are tied to env1, and Ins 3 and 4 are tied to env 2.
Each input has its own output. The raw envelope signals are also available on the CV outs.
The envelope curves and attack/decay times ae controllable as well.

[Source Code](https://github.com/electro-smith/DaisyExamples/tree/master/patch/QuadEnvelope)

## Controls

| Control | Description | Comment |
| --- | --- | --- |
| Gate Ins | Trigger the internal envelopes | Gate In 1 goes with env1, and Gate In 2 with env2 |
| Encoder Press | Switch between control modes | Controls attack/decay or curve |
| Ctrl1 | Env one attack / curve | Controls envelope one's attack time, or curve, depending on the mode |
| Ctrl2 | Env one decay / nothing | Controls envelope one's decay time, or nothing, depending on the mode |
| Ctrl1 | Env two attack / curve | Controls envelope two's attack time, or curve, depending on the mode |
| Ctrl2 | Env two decay / nothing | Controls envelope two's decay time, or nothing, depending on the mode |
| Audio Ins | Audio to be VCA'd by the envelopes | Ins 1 and 2 go with env1, ins 2 and 4 with env2 |
| Audio Outs | Audio post VCA | Each output goes with its respective input |
| CV Outs | Envelope CV | Goes with envelopes one and two respectively |




