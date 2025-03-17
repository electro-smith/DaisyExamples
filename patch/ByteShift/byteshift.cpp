#include "byteshift.h"
#include <cmath>

// *******************************************************************
// 
// Byte Shift
// 
// *******************************************************************

void ByteShift::Init(float sampleRate) {
    pitchShifter.Init(sampleRate);
    pitchShifter.SetTransposition(0.0f);  // Default: No pitch shift
}

void ByteShift::SetPitchShift(int encoderCount) {
    float semitoneShift = static_cast<float>(encoderCount);
    pitchShifter.SetTransposition(semitoneShift);
}

float ByteShift::ProcessSample(float inSample) {
    return pitchShifter.Process(inSample);
}