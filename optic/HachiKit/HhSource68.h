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
        static const uint8_t MORPH_606_VALUE = 0.35;
        // the morph value at which the sound imitates an 808
        static const uint8_t MORPH_808_VALUE = 0.65;


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

        float GetMorph();
        void SetMorph(float morph);

    private:
        static const float freqs606[];
        static const float freqs808[];
        static const float MIN_FREQ_FACTOR;

        Oscillator oscs[OSC_COUNT];
        float morph;
        float frequencies[OSC_COUNT];
        float signal;

};



#endif
