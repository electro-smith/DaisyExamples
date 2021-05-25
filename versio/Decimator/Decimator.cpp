#include "daisy_versio.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyVersio hw;

// bitcrush effect
Decimator decimator_l;
Decimator decimator_r;

static float kval;

// This runs at a fixed rate, to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    // Audio is interleaved stereo by default
    for(size_t i = 0; i < size; i += 2)
    {
        // left out
        out[i] = decimator_l.Process(in[i]);

        // right out
        out[i + 1] = decimator_r.Process(in[i + 1]);
    }
}

int main(void)
{
    // Initialize Versio hardware and start audio, ADC
    hw.Init();

    decimator_l.Init();
    decimator_r.Init();

    kval = 0.f;

    hw.StartAudio(callback);
    hw.StartAdc();

    while(1)
    {
        hw.ProcessAnalogControls(); // Normalize CV inputs
        hw.UpdateExample(); // Control the LED colors using the knobs and gate inputs
        hw.UpdateLeds();

        decimator_l.SetBitcrushFactor(hw.GetKnobValue(DaisyVersio::KNOB_0));
        decimator_l.SetDownsampleFactor(hw.GetKnobValue(DaisyVersio::KNOB_1));

        decimator_r.SetBitcrushFactor(hw.GetKnobValue(DaisyVersio::KNOB_2));
        decimator_r.SetDownsampleFactor(hw.GetKnobValue(DaisyVersio::KNOB_3));

        kval = hw.GetKnobValue(DaisyVersio::KNOB_6);
    }
}
