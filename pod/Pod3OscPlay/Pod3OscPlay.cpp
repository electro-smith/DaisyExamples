// # Petal3OscPlay
// ## Description
// Dev Playground for Seed3
//
#include <stdio.h>
#include <string.h>
//#include "daisy_seed.h"
#include "daisy_petal.h"
#include "daisysp.h"
using namespace daisy;

// Globals
DaisyPetal            hw;
daisysp::Oscillator DSY_SDRAM_BSS osc;
uint8_t                           wave;

float bright, amp, targetamp;

//void AudioTest(float *in, float *out, size_t size)
//{
//    hw.UpdateAnalogControls();
//    hw.DebounceControls();
//    float sig, note;
//    // One way to get value
//    note = hw.GetKnobValue(DaisyPetal::KNOB_1) * 127.0f;
//    // Or
//    bright = hw.knob2.Value();
//    osc.SetFreq(daisysp::mtof(note));
//
//	// Handle Encoder for waveform switching test.
//    int32_t inc;
//    inc = hw.encoder.Increment();
//    if(inc > 0)
//    {
//        wave = (wave + 1) % daisysp::Oscillator::WAVE_LAST;
//        osc.SetWaveform(wave);
//    }
//    else if(inc < 0)
//    {
//        wave = (wave + daisysp::Oscillator::WAVE_LAST - 1)
//               % daisysp::Oscillator::WAVE_LAST;
//        osc.SetWaveform(wave);
//    }
//
//    // Turn amplitude on when pressing button.
//    targetamp = hw.encoder.Pressed() ? 1.0f : 0.0f;
//
//    for(size_t i = 0; i < size; i += 2)
//
//    {
//        daisysp::fonepole(amp, targetamp, 0.005f);
//        osc.SetAmp(amp);
//        sig    = osc.Process();
//        out[i] = out[i + 1] = sig;
//    }
//}

void AudioTest(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = in[i + 1];
        out[i + 1] = in[i];
    }
}

daisysp::ReverbSc  DSY_SDRAM_BSS rev;

float mix, damp, time;


// This runs at a fixed rate, to prepare audio samples
void callback(float *in, float *out, size_t size)
{
    float dryl, dryr, wetl, wetr, sendl, sendr;
    // Audio is interleaved stereo by default
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    mix  = hw.GetKnobValue(DaisyPetal::KNOB_1);
    time = hw.GetKnobValue(DaisyPetal::KNOB_2);
    damp = 1000.0f + (hw.GetKnobValue(DaisyPetal::KNOB_3) * 12000.0f);
    rev.SetFeedback(time);
    rev.SetLpFreq(damp);
    for(size_t i = 0; i < size; i += 2)
    {
        dryl  = in[i];
        dryr  = in[i + 1];
        sendl = dryl * mix;
        sendr = dryr * mix;
        rev.Process(sendl, sendr, &wetl, &wetr);
        if(hw.switches[DaisyPetal::SW_6].Pressed())
        {
            out[i + 1] = dryl + wetl;
            out[i]     = dryr + wetr;
        }
        else
        {
            out[i + 1] = in[i];
            out[i]     = in[i + 1];
        }
    }
}

int main(void)
{
    float samplerate;
    hw.Init();
    samplerate = hw.AudioSampleRate();
    // Init Synthesis
    osc.Init(samplerate);
    rev.Init(samplerate);
    // Start
    hw.StartAdc();
    hw.SetAudioBlockSize(128);
//    hw.SetAudioBlockSize(6);
    hw.StartAudio(callback);
    hw.ClearLeds();
    bright = 0.0f;
    //    hw.led1.Set(0.0f, 0.0f, bright);
    //    hw.led2.Set(0.0f, 0.0f, bright);
    for(;;)
    {
//        hw.led1.Set(bright, bright, bright);
//        hw.led2.Set(bright, bright, bright);
//        hw.UpdateLeds();
    }
    }
