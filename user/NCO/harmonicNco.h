// harmonicNco.h
#pragma once
#ifndef HARMONIC_NCO_H
#define HARMONIC_NCO_H

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "daisysp.h"
#include "nco.h"

using namespace std;

class HarmonicNCO {
   public:
      static const uint8_t NUM_HARMONICS = 8;

      HarmonicNCO();
      ~HarmonicNCO();

      void  SetSampleRate(uint32_t sample_freq);
      void  SetFrequency(float freq);
      void  SetAmplitudes(float *amplitudes);
      void  SetPhases(float *phases);
      float NextSample();

   private:
      NCO   nco[NUM_HARMONICS];  // The NCO for each harmonic
      float amp[NUM_HARMONICS];  // The amplitude for each harmonic
};

#endif
