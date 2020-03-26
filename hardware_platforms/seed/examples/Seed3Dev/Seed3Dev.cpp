// # Seed3Dev
// ## Description
// Dev Playground for Seed3
//
#include <stdio.h>
#include <string.h>
//#include "daisy_seed.h"
//#include "daisy_pod.h"
#include "daisy_petal.h"
#include "daisysp.h"
using namespace daisy;

// Globals
DaisyPetal          hw;
daisysp::Oscillator DSY_SDRAM_BSS osc;
uint8_t                           wave;

float amp, targetamp;

void AudioTest(float *in, float *out, size_t size)
{
    hw.UpdateAnalogControls();
    hw.DebounceControls();
    float sig, note;
    // One way to get value
    note = hw.GetKnobValue(DaisyPetal::KNOB_6) * 127.0f;
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
    targetamp
        = (hw.encoder.Pressed() || hw.switches[DaisyPetal::SW_5].Pressed())
              ? 1.0f
              : 0.0f;

    for(size_t i = 0; i < size; i += 2)

    {
        daisysp::fonepole(amp, targetamp, 0.005f);
        osc.SetAmp(amp);
        if(hw.switches[DaisyPetal::SW_6].Pressed())
        {
            sig    = osc.Process();
            out[i] = out[i + 1] = sig;
        }
        else
        {
            out[i]     = in[i];
            out[i + 1] = in[i + 1];
        }
    }
}

float r, g, b, fsbright;
int   main(void)
{
    float    samplerate;
    uint32_t tick, now;
    uint8_t  active_led;
    hw.Init();
    samplerate = hw.AudioSampleRate();
    // Init Synthesis
    osc.Init(samplerate);
    // Start
    hw.StartAdc();
    hw.StartAudio(AudioTest);
    hw.ClearLeds();
    for(;;)
    {
        now = dsy_system_getnow();

        r        = hw.GetKnobValue(DaisyPetal::KNOB_1);
        g        = hw.GetKnobValue(DaisyPetal::KNOB_2);
        b        = hw.GetKnobValue(DaisyPetal::KNOB_3);
        fsbright = hw.GetKnobValue(DaisyPetal::KNOB_4);

        // Led Ring goes in a circle
        if(now - tick > 250)
        {
            tick       = now;
            active_led = (active_led + 1) % DaisyPetal::RING_LED_LAST;
        }
        for(size_t i = 0; i < DaisyPetal::RING_LED_LAST; i++)
        {
            if(i == active_led)
            {
                hw.SetRingLed(static_cast<DaisyPetal::RingLed>(i), r, g, b);
            }
            else
            {
                hw.SetRingLed(
                    static_cast<DaisyPetal::RingLed>(i), 0.0f, 0.0f, 0.0f);
            }
        }
        hw.SetFootswitchLed(DaisyPetal::FOOTSWITCH_LED_1, fsbright);
        hw.SetFootswitchLed(DaisyPetal::FOOTSWITCH_LED_2, fsbright);
        hw.SetFootswitchLed(DaisyPetal::FOOTSWITCH_LED_3, fsbright);
        hw.SetFootswitchLed(DaisyPetal::FOOTSWITCH_LED_4, fsbright);

        hw.UpdateLeds();
        hw.DelayMs(20);
    }
}
