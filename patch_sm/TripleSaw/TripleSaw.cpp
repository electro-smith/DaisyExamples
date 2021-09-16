#include "daisy_patch_sm.h"
#include "daisysp.h"

/** TODO: ADD CALIBRATION */

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM patch;
Oscillator   osc_a, osc_b, osc_c;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAllControls();

    /** Get Coarse, Fine, and V/OCT inputs from hardware 
     *  MIDI Note number are easy to use for defining ranges */
    float coarse_tune = fmap(patch.GetAdcValue(CV_1), 12, 84);
    float fine_tune   = fmap(patch.GetAdcValue(CV_2), 0, 10);
    float voct        = fmap(patch.GetAdcValue(CV_5), 0, 60.f);

    /** Convert from MIDI note number to frequency */
    float freq_a = mtof(fclamp(coarse_tune + fine_tune + voct, 0.f, 127.f));

    /** Calculate a detune amount */
    float detune_amt = patch.GetAdcValue(CV_3);
    float freq_b     = freq_a + (0.05 * freq_a * detune_amt);
    float freq_c     = freq_a - (0.05 * freq_a * detune_amt);

    /** Set all three oscillators' frequencies */
    osc_a.SetFreq(freq_a);
    osc_b.SetFreq(freq_b);
    osc_c.SetFreq(freq_c);

    /** Process each sample of the oscillator and send to the hardware outputs */
    for(size_t i = 0; i < size; i++)
    {
        float sig = osc_a.Process() + osc_b.Process() + osc_c.Process();
        OUT_L[i]  = sig;
        OUT_R[i]  = sig;
    }
}

int main(void)
{
    patch.Init();

    osc_a.Init(patch.AudioSampleRate());
    osc_b.Init(patch.AudioSampleRate());
    osc_c.Init(patch.AudioSampleRate());

    osc_a.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc_b.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc_c.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);

    patch.StartAudio(AudioCallback);
    while(1) {}
}
