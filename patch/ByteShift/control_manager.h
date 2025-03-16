#ifndef CONTROL_MANAGER_H
#define CONTROL_MANAGER_H

#include "daisy_patch.h"

class ControlManager {
public:
    void Init(daisy::DaisyPatch* p);
    void Update();

    float GetCtrl1() const { return ctrl1; }
    float GetCtrl2() const { return ctrl2; }
    float GetCtrl3() const { return ctrl3; }
    float GetCtrl4() const { return ctrl4; }
    int GetEncoderCount() const { return encoderCount; }
    bool IsEncoderPressed() const { return encoderPressed; }

private:
    daisy::DaisyPatch* patch;
    
    float ctrl1, ctrl2, ctrl3, ctrl4;
    int encoderCount;
    bool encoderPressed;
};

#endif