# Looper

## Author

beserge

## Description

Simple Looper for Patch SM.  
Includes controls for record on / off / clear, Input Volume, and Loop Volume.  
The output is a 50/50 mix of the post gain input and loop signals.  

For help connecting the tactile switch, and two potentiometers, refer to figures 1.5 and 1.2 respectively in the [Patch SM Datasheet](https://github.com/electro-smith/DaisyPatchSM/blob/main/doc/datasheet/ES_Patch_SM_datasheet_v1.0.pdf).  


## Controls

| Pin Name | Pin Location | Function | Comment |
| --- | --- | --- | --- |
| CV_1 | C5 | Input Level | Connect a potentiometer here. |
| CV_2 | C4 | Loop Level | Connect a potentiometer here. |
| B7 | B7 | Record Toggle |  Connect a tactile switch here. Press to toggle recording, hold to clear. |
