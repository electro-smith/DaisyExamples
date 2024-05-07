#ifndef HHSOURCE68_H
#define HHSOURCE68_H

#include "daisy_patch.h"
#include "daisysp.h"
#include "ISource.h"

using namespace daisy;
using namespace daisysp;

class HhSource68: public ISource {

    public:
        // how many square waves make up the sound source
        static const uint8_t OSC_COUNT = 6;

        // the morph value at which the sound imitates a 606
        static const float MORPH_606_VALUE;
        // the morph value at which the sound imitates an 808
        static const float MORPH_808_VALUE;

        static const float HH_HPF_MAX;
        static const float HH_HPF_MIN;
        static const float GAIN_MAX;

        /** Initialize model with default parameters.
         * \param sample_rate audio sample rate.
        */
        void Init(float sample_rate);

        /** Initialize model with specified parameters.
         * \param sample_rate audio sample rate.
         * \param frequency oscillator frequency in hertz.
        */
        void Init(float sample_rate, float morph);

        float Process();
        float Signal();
        float hpfFreq = 2700;
        float lpfFreq = 5000;

        float GetMorph();
        void SetMorph(float morph);
        float GetHpfFrequency();
        void SetHpfFrequency(float freq);
        float GetLpfFrequency();
        void SetLpfFrequency(float freq);

    private:
        static const float freqs606[];
        static const float freqs808[];
        static const float MIN_FREQ_FACTOR;

        float morph;
        float signal;
        float gain = 1.0;

        Oscillator* oscs[OSC_COUNT];
        Oscillator osc0, osc1, osc2, osc3, osc4, osc5;
        Svf hpf, lpf;

};



#endif
