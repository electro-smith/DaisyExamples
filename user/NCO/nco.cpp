#include "nco.h"

// Define/initialize NCO's static members
float NCO::sine_table[NCO_LUT_SIZE];
bool  NCO::sine_table_init = false;

// NCO Default Constructor
NCO::NCO()
{
    // Set sampling frequency to default
    SetSampleRate(DEFAULT_FS);

    phase_accum = 0;
    k_val = 0;

    if (!sine_table_init)
    {
        InitSineTable();
    }
}

NCO::NCO(uint32_t sample_freq)
{
    // Set sampling frequency
    SetSampleRate(sample_freq);

    phase_accum = 0;
    k_val = 0;

    if (!sine_table_init)
    {
        InitSineTable();
    }
}

NCO::~NCO()
{
   // TODO
}

void NCO::SetSampleRate(uint32_t sample_freq)
{
    sample_rate = (sample_freq > 0) ? sample_freq : DEFAULT_FS;
}

void NCO::SetFrequency(float freq)
{
    k_val = (uint16_t)((freq / sample_rate) * (1 << N_PHASE_ACCUM_BITS));
}

void NCO::SetPhase(float phase)
{
    // Limit phase param between 0.0 - 1.0
    if (phase >= 0.0 && phase < 1.0)
    {
        phase_accum = (uint16_t)(phase * (1 << N_PHASE_ACCUM_BITS));
    }
}

float NCO::NextSample()
{
    uint16_t lut_ndx = 0;

    // Increment phase accumulator
    phase_accum += k_val;
    // Use top 10 bits of phase_accum as LUT index
    lut_ndx  = phase_accum >> (N_PHASE_ACCUM_BITS - N_PHASE_BITS);
    lut_ndx &= NCO_LUT_MASK;
    // Return the new LUT sample
    return sine_table[lut_ndx];
}

void NCO::InitSineTable()
{
    int ndx;

    for (ndx = 0; ndx < NCO_LUT_SIZE; ndx++)
    {
        sine_table[ndx] = sin(2 * M_PI * ndx / (float)NCO_LUT_SIZE);
    }
    sine_table_init = true;
}
