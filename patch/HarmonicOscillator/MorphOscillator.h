#pragma once

#include "daisysp.h"
using namespace daisysp;

class MorphOscillator {
  public:
    void Init(float sample_rate);

    void SetFreq(float f);
    void SetAmp(float a);
    void SetShape(float s);
    float Process();

  private:
    Oscillator osc[4]; 
    float freq;
    float shape;
};