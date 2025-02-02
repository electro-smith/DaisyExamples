// nco.h
#pragma once
#ifndef NCO_H
#define NCO_H

#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "daisysp.h"

using namespace std;

class NCO
{
    public:
        static const uint8_t    N_PHASE_BITS         = 10;
        static const uint8_t    N_PHASE_ACCUM_BITS   = 16;
        static const uint16_t   NCO_LUT_SIZE         = (1 << N_PHASE_BITS);
        static const uint16_t   NCO_LUT_MASK         = (NCO_LUT_SIZE - 1);
        static const uint32_t   DEFAULT_FS           = 10000;

        NCO();
        NCO(uint32_t sample_freq);
        ~NCO();

        void    SetSampleRate(uint32_t sample_freq);
        void    SetFrequency(float freq);
        void    SetPhase(float phase);
        float   NextSample();

    private:
        // Actual LUT for sampled sine wave - shared between all instances
        static float    sine_table[];
        static bool     sine_table_init;
        uint32_t        sample_rate;
        uint16_t        phase_accum;
        uint16_t        k_val;

        void InitSineTable();
};

#endif
