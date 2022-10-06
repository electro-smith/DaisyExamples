#include "daisy_legio.h"
#include "daisysp.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyLegio hw;

Oscillator modulator;
Oscillator carrier;
AdEnv      env;

float carrierBaseFreq;
float modAmount;
float envVal;
float inLedVal;
float outLedVal;
float pitchOffset = 0;

// This runs at a fixed rate, to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    hw.ProcessAllControls();

    float encInc = hw.encoder.Increment();

    pitchOffset += hw.encoder.Pressed() ? encInc / 12 : (encInc / 12) * 0.05;

    carrierBaseFreq
        = pow(2,
              hw.controls[DaisyLegio::CONTROL_PITCH].Value() * 8
                  + hw.sw[DaisyLegio::SW_RIGHT].Read() + pitchOffset)
          * 8;

    modAmount = pow(hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_TOP), 2) * 10000;

    float modFreq
        = carrierBaseFreq
          * pow(2, hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_BOTTOM) * 5);

    modulator.SetFreq(modFreq);

    if(hw.gate.Trig())
    {
        env.SetTime(ADENV_SEG_ATTACK, 0.005);
        env.SetTime(ADENV_SEG_DECAY,
                    0.25 * (hw.sw[DaisyLegio::SW_LEFT].Read() + 1));
        env.Trigger();
    }

    // Audio is interleaved stereo by default
    for(size_t i = 0; i < size; i += 2)
    {
        envVal   = env.Process();
        inLedVal = abs((in[i] + in[i + 1]) / 2.0);

        float modA = modulator.Process() * modAmount * (envVal + 1);
        float modB = (in[i] + in[i + 1]) * modAmount
                     * hw.sw[DaisyLegio::SW_LEFT].Read() / 2.0;

        carrier.SetFreq(carrierBaseFreq + modA + modB);

        float output = carrier.Process();
        outLedVal    = abs(output);
        // left out
        out[i] = output;
        // right out
        out[i + 1] = output;
    }
}

int main(void)
{
    // Initialize Legio hardware and start audio, ADC
    hw.Init();

    modulator.Init(hw.AudioSampleRate());
    modulator.SetFreq(0);

    carrier.Init(hw.AudioSampleRate());
    carrier.SetFreq(0);

    env.Init(hw.AudioSampleRate());

    hw.StartAudio(callback);
    hw.StartAdc();

    while(1)
    {
        hw.SetLed(DaisyLegio::LED_LEFT, envVal, inLedVal, outLedVal);
        hw.SetLed(DaisyLegio::LED_RIGHT,
                  hw.controls[DaisyLegio::CONTROL_PITCH].Value(),
                  hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_TOP),
                  hw.GetKnobValue(DaisyLegio::CONTROL_KNOB_BOTTOM));
        hw.UpdateLeds();
    }
}
