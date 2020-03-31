// # Pod3OscPlay
// ## Description
// Dev Playground for Seed3
//
#include <stdio.h>
#include <string.h>
//#include "daisy_seed.h"
#include "daisy_pod.h"
#include "daisysp.h"
using namespace daisy;

// Globals
DaisyPod            hw;
daisysp::Oscillator DSY_SDRAM_BSS osc;
uint8_t                           wave;

float bright, amp, targetamp;

void AudioTest(float *in, float *out, size_t size)
{
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    float sig, note;
    // One way to get value
    note = hw.GetKnobValue(DaisyPod::KNOB_1) * 127.0f;
    // Or
    bright = hw.knob2.Value();
    osc.SetFreq(daisysp::mtof(note));

	// Handle Encoder for waveform switching test.
    int32_t inc;
    inc = hw.encoder.Increment();
    if(inc > 0)
    {
        wave = (wave + 1) % daisysp::Oscillator::WAVE_LAST;
        osc.SetWaveform(wave);
    }
    else if(inc < 0)
    {
        wave = (wave + daisysp::Oscillator::WAVE_LAST - 1)
               % daisysp::Oscillator::WAVE_LAST;
        osc.SetWaveform(wave);
    }

    // Turn amplitude on when pressing button.
    targetamp = hw.encoder.Pressed() ? 1.0f : 0.0f;

    for(size_t i = 0; i < size; i += 2)

    {
        daisysp::fonepole(amp, targetamp, 0.005f);
        osc.SetAmp(amp);
        sig    = osc.Process();
        out[i] = out[i + 1] = sig;
    }
}

int main(void)
{
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();
    // Init Synthesis
    osc.Init(samplerate);
    // Start
    hw.StartAdc();
    hw.StartAudio(AudioTest);
    hw.ClearLeds();
    bright = 0.0f;
    hw.led1.Set(0.0f, 0.0f, bright);
    hw.led2.Set(0.0f, 0.0f, bright);
    for(;;)
    {
        hw.led1.Set(bright, bright, bright);
        hw.led2.Set(bright, bright, bright);
        hw.UpdateLeds();
    }
}
