#ifndef BYTESHIFT_H
#define BYTESHIFT_H

#include "daisysp.h"

// *******************************************************************
// 
// Byte shift
// 
// *******************************************************************

class ByteShift {
public:
    void Init(float sampleRate);
    float ProcessSample(float inSample);  // Takes input sample from BytebeatSynth
    void SetPitchShift(int encoderCount);

private:
    daisysp::PitchShifter pitchShifter;
};

#endif
