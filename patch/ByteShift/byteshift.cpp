#include "byteshift.h"

void ByteShift::Init(daisy::DaisyPatch* p) {
    patch = p;
    speed = paramA = paramB = paramC = 0.0f;
    encoderValue = 0;
}

void ByteShift::SetControlValues(float c1, float c2, float c3, float c4, int enc) {
    speed = c1;
    paramA = c2;
    paramB = c3;
    paramC = c4;
    encoderValue = enc;
}

void ByteShift::ProcessAudio(float** out, size_t size) {
    for (size_t i = 0; i < size; i++) {
        out[0][i] = speed * 0.01f;
        out[1][i] = paramA * 0.01f;
    }
}