# Looper

## Author

beserge

## Description

Simple Looper for Patch SM.  
Includes controls for record on / off / clear, Input Volume, and Loop Volume.  
The output is a 50/50 mix of the post gain input and loop signals.  

If you are using the patch.Init(), the controls are already connected as they should be.  

If you are using a bare Patch SM, refer to the 1.x figures in the [Patch SM Datasheet](https://github.com/electro-smith/DaisyPatchSM/blob/main/doc/datasheet/ES_Patch_SM_datasheet_v1.0.pdf).  


## Controls

| Pin Name | Pin Location | Function | Comment |
| --- | --- | --- | --- |
| CV_1 | C5 | Input Level | Connect a potentiometer here. |
| CV_2 | C4 | Loop Level | Connect a potentiometer here. |
| B7 | B7 | Record Toggle |  Connect a tactile switch here. Press to toggle recording, hold to clear. |
| OUT_R | B1 | Audio Out R | Connect a jack here. |
| OUT_L | B2 | Audio Out L | Connect a jack here. |
| IN_R | B3 | Audio In R | Connect a jack here. |
| IN_L | B4 | Audio In L | Connect a jack here. |