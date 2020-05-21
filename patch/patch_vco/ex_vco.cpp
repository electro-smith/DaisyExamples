#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

daisy_patch patch;
Oscillator osc;
Parameter freqctrl, wavectrl, ampctrl;

static void AudioCallback(float *in, float *out, size_t size)
{
	float sig, freq, amp;
	size_t wave;
    for (size_t i = 0; i < size; i += 2)
    {
        // Read Knobs
        freq = mtof(freqctrl.Process());
        wave = wavectrl.Process();
        amp = ampctrl.Process();
        // Set osc params
        osc.SetFreq(freq);
        osc.SetWaveform(wave);
        osc.SetAmp(amp);
        //.Process
    	sig = osc.Process();
    	// left out
        out[i] = sig;
        // right out
        out[i+1] = sig;
    }
}

int main(void)
{
    int num_waves = Oscillator::WAVE_LAST - 1;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    osc.Init(SAMPLE_RATE); // Init oscillator

    // This is with the GetCtrl, but it can also be done with the public members.
    //freqctrl.Init(patch.GetCtrl(daisy_patch::KNOB_1), 10.0, 110.0, Parameter::LINEAR);
    //wavectrl.Init(patch.GetCtrl(daisy_patch::KNOB_2), 0.0, num_waves, Parameter::LINEAR);
    //ampctrl.Init(patch.GetCtrl(daisy_patch::KNOB_2), 0.0, 0.5, Parameter::LINEAR);
    // Like this:
    freqctrl.Init(patch.knob1, 10.0, 110.0f, Parameter::LINEAR);
    wavectrl.Init(patch.knob2, 0.0, num_waves, Parameter::LINEAR);
    ampctrl.Init(patch.knob3, 0.0, 0.5f, Parameter::LINEAR);

    dsy_adc_start();
    patch.StartAudio(AudioCallback);

    while(1) {} // loop forever
}
