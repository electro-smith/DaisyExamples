#include "daisy_patch_sm.h"
#include "daisysp.h"


/** TODO move these to libDaisy if we like them */
#define IN_LEFT in[0][i]
#define IN_RIGHT in[1][i]
#define OUT_LEFT out[0][i]
#define OUT_RIGHT out[1][i]

using namespace daisy;
using namespace daisysp;

DaisyPatchSM patch;
Oscillator   osc_a, osc_b, osc_c;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    patch.ProcessAnalogControls();

    float coarse_tune = 12.f + (patch.GetAdcValue(DaisyPatchSM::CV_1) * 72.f);
    float fine_tune   = patch.GetAdcValue(patch.CV_2) * 10.f;
    float freq_a      = mtof(coarse_tune + fine_tune);
    float detune_amt  = patch.GetAdcValue(patch.CV_3);
    float freq_b      = freq_a + (0.05 * freq_a * detune_amt);
    float freq_c      = freq_a - (0.05 * freq_a * detune_amt);

    osc_a.SetFreq(freq_a);
    osc_b.SetFreq(freq_b);
    osc_c.SetFreq(freq_c);

    for(size_t i = 0; i < size; i++)
    {
        float sig = osc_a.Process() + osc_b.Process() + osc_c.Process();
        OUT_LEFT = OUT_RIGHT = sig;
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
