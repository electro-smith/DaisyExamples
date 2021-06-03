#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
Oscillator osc;
Parameter  freqctrl, wavectrl, ampctrl, finectrl;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    float  sig, freq, amp;
    size_t wave;

    patch.ProcessAnalogControls();

    for(size_t i = 0; i < size; i++)
    {
        // Read Knobs
        freq = mtof(freqctrl.Process() + finectrl.Process());
        wave = wavectrl.Process();
        amp  = ampctrl.Process();
        // Set osc params
        osc.SetFreq(freq);
        osc.SetWaveform(wave);
        osc.SetAmp(amp);
        //.Process
        sig = osc.Process();
        // Assign Synthesized Waveform to all four outputs.
        for(size_t chn = 0; chn < 4; chn++)
        {
            out[chn][i] = sig;
        }
    }
}

int main(void)
{
    float samplerate;
    int   num_waves = Oscillator::WAVE_LAST - 1;
    patch.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = patch.AudioSampleRate();

    osc.Init(samplerate); // Init oscillator

    freqctrl.Init(
        patch.controls[patch.CTRL_1], 10.0, 110.0f, Parameter::LINEAR);
    finectrl.Init(patch.controls[patch.CTRL_2], 0.f, 7.f, Parameter::LINEAR);
    wavectrl.Init(
        patch.controls[patch.CTRL_3], 0.0, num_waves, Parameter::LINEAR);
    ampctrl.Init(patch.controls[patch.CTRL_4], 0.0, 0.5f, Parameter::LINEAR);

    //briefly display module name
    std::string str  = "VCO";
    char*       cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);
    patch.display.Update();
    patch.DelayMs(1000);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
        patch.DisplayControls(false);
    }
}
