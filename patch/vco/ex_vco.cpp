#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;
Oscillator osc;
Parameter  freqctrl, wavectrl, ampctrl, finectrl;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    float  sig, freq, amp;
    size_t wave;

    hw.ProcessAnalogControls();

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
    hw.Init(); // Initialize hardware (daisy seed, and patch)
    samplerate = hw.AudioSampleRate();

    osc.Init(samplerate); // Init oscillator

    freqctrl.Init(
        hw.controls[hw.CTRL_1], 10.0, 110.0f, Parameter::LINEAR);
    finectrl.Init(hw.controls[hw.CTRL_2], 0.f, 7.f, Parameter::LINEAR);
    wavectrl.Init(
        hw.controls[hw.CTRL_3], 0.0, num_waves, Parameter::LINEAR);
    ampctrl.Init(hw.controls[hw.CTRL_4], 0.0, 0.5f, Parameter::LINEAR);

    //briefly display module name
    std::string str  = "VCO";
    char*       cstr = &str[0];
    hw.display.WriteString(cstr, Font_7x10, true);
    hw.display.Update();
    hw.DelayMs(1000);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.DisplayControls(false);
    }
}
