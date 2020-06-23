#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Oscillator osc;
Parameter freqctrl, wavectrl, ampctrl;

static void AudioCallback(float **in, float **out, size_t size)
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
        // Assign Synthesized Waveform to all four outputs.
        for (size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = sig;
        }
    }
}

int main(void)
{
    float samplerate;
    int num_waves = Oscillator::WAVE_LAST - 1;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();
    osc.Init(samplerate); // Init oscillator
    freqctrl.Init(patch.controls[patch.CTRL_1], 10.0, 110.0f, Parameter::LINEAR);
    wavectrl.Init(patch.controls[patch.CTRL_2], 0.0, num_waves, Parameter::LINEAR);
    ampctrl.Init(patch.controls[patch.CTRL_3], 0.0, 0.5f, Parameter::LINEAR);
    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1) {} // loop forever
}
