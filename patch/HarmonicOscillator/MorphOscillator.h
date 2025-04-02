#pragma once

#include "daisysp.h"
using namespace daisysp;

class MorphOscillator {
  public:
    void Init(float sample_rate);

    void SetFreq(float f);
    void SetShape(float s);
    void SetAmp(float a);
    float Process();

  private:
    Oscillator osc[4];
    float freq;
    float shape;
};