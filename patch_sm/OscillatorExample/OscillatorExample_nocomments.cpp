#include "DaisyPatchSM.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatchSM hw;
Oscillator   osc_a, osc_b, osc_c;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAnalogControls();

    float coarse_tune = 12.f + (hw.GetAdcValue(0) * 72.f);
    float fine_tune   = hw.GetAdcValue(1) * 10.f;
    float freq_a      = mtof(coarse_tune + fine_tune);
    float detune_amt  = hw.GetAdcValue(2);
    float freq_b      = freq_a + (0.05 * freq_a * detune_amt);
    float freq_c      = freq_a - (0.05 * freq_a * detune_amt);

    osc_a.SetFreq(freq_a);
    osc_b.SetFreq(freq_b);
    osc_c.SetFreq(freq_c);

    for(size_t i = 0; i < size; i++)
    {
        float sig = osc_a.Process() + osc_b.Process() + osc_c.Process();
        out[0][i] = out[1][i] = sig;
    }
}

int main(void)
{
    hw.Init();
    osc_a.Init(hw.AudioSampleRate());
    osc_b.Init(hw.AudioSampleRate());
    osc_c.Init(hw.AudioSampleRate());

    osc_a.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc_b.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);
    osc_c.SetWaveform(Oscillator::WAVE_POLYBLEP_SAW);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
