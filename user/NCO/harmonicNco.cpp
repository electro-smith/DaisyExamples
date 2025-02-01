#include "harmonicNco.h"

// HarmonicNCO default constructor
HarmonicNCO::HarmonicNCO() {
   uint8_t nHarmonic;

   for (nHarmonic = 0; nHarmonic < NUM_HARMONICS; nHarmonic++) {
      nco[nHarmonic].SetSampleRate(NCO::DEFAULT_FS);
      amp[nHarmonic] = 1.0;
   }
}

HarmonicNCO::~HarmonicNCO() {
   // TODO
}

void HarmonicNCO::SetSampleRate(uint32_t sample_freq) {
   uint8_t nHarmonic;

   for (nHarmonic = 0; nHarmonic < NUM_HARMONICS; nHarmonic++) {
      nco[nHarmonic].SetSampleRate(sample_freq);
   }
}

void HarmonicNCO::SetFrequency(float freq) {
   uint8_t nHarmonic;
   float harmonicFreq = 0.0;

   for (nHarmonic = 0; nHarmonic < NUM_HARMONICS; nHarmonic++) {
      harmonicFreq = (nHarmonic + 1) * freq;
      nco[nHarmonic].SetFrequency(harmonicFreq);
   }
}

void HarmonicNCO::SetAmplitudes(float *amplitudes) {
   uint8_t nHarmonic;

   if (amplitudes) {

      for (nHarmonic = 0; nHarmonic < NUM_HARMONICS; nHarmonic++) {
         amp[nHarmonic] = amplitudes[nHarmonic];
      }
   }
}

void HarmonicNCO::SetPhases(float *phases) {
   uint8_t nHarmonic;

   if (phases) {

      for (nHarmonic = 0; nHarmonic < NUM_HARMONICS; nHarmonic++) {
         nco[nHarmonic].SetPhase(phases[nHarmonic]);
      }
   }
}

float HarmonicNCO::NextSample() {
   uint8_t nHarmonic;
   float sample = 0.0;
   float ampTotal = 0.0;

   for (nHarmonic = 0; nHarmonic < NUM_HARMONICS; nHarmonic++) {
      sample += (amp[nHarmonic] * nco[nHarmonic].NextSample());
      ampTotal += amp[nHarmonic];
   }
   // Normalize amplitude of sample
   sample /= ampTotal;

   return sample;
}
