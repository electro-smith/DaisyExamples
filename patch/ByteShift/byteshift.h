#ifndef BYTESHIFT_H
#define BYTESHIFT_H

#include "daisy_patch.h"
#include "control_manager.h"

class ByteShift {
public:
    void Init(daisy::DaisyPatch* p);
    void ProcessAudio(float** out, size_t size);
    void SetControlValues(float c1, float c2, float c3, float c4, int enc);

private:
    daisy::DaisyPatch* patch;
    float speed, paramA, paramB, paramC;
    int encoderValue;
};

#endif