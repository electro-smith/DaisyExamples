#include "daisysp.h"
#include "daisy_petal.h"
#include "terrarium.h"
#include <cmath>

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

// Knob 1 = Frequency
// Knob 2 = Gain
// Knob 3 = Q

DaisyPetal petal;
Parameter knob_1, knob_2, knob_3, knob_4, knob_5, knob_6;
Led led1, led2;

float sample_rate;

struct ParametricEQ
{
    float sample_rate = 48000.0f;
    float a1, a2, b0, b1, b2;
    float x1, x2, y1, y2;

    void SetSampleRate(float sr) { sample_rate = sr; }

    void SetParams(float freq, float q, float gain_db)
    {
        float A     = powf(10.0f, gain_db / 40.0f);
        float omega = 2.0f * M_PI * freq / sample_rate;
        float alpha = sinf(omega) / (2.0f * q);
        float cosw  = cosf(omega);

        float a0 = 1.0f + alpha / A;
        a1 = -2.0f * cosw;
        a2 = 1.0f - alpha / A;
        b0 = 1.0f + alpha * A;
        b1 = -2.0f * cosw;
        b2 = 1.0f - alpha * A;

        // Normalize
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    float Process(float in)
    {
        float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = in;
        y2 = y1; y1 = out;
        return out;
    }
};

ParametricEQ eq;



// Initialize the LEDs
void InitLeds()
{
    led1.Init(petal.seed.GetPin(Terrarium::LED_1), false);
    led2.Init(petal.seed.GetPin(Terrarium::LED_2), false);
}

void InitParams()
{
    // Initialize parameters for Delay1 -----------------------
    knob_1.Init(petal.knob[Terrarium::KNOB_2], 0.0f, 1.0f, Parameter::LINEAR);
    knob_2.Init(petal.knob[Terrarium::KNOB_3], 0.0f, 1.0f, Parameter::LINEAR);
    knob_3.Init(petal.knob[Terrarium::KNOB_4], 0.0f, 1.0f, Parameter::LINEAR);
    knob_4.Init(petal.knob[Terrarium::KNOB_5], 0.0f, 1.0f, Parameter::LINEAR);
    knob_5.Init(petal.knob[Terrarium::KNOB_6], 0.0f, 1.0f, Parameter::LINEAR);
    knob_6.Init(petal.knob[Terrarium::KNOB_1], 0.0f, 1.0f, Parameter::LINEAR);
}

// ////////////////////////////////////////////////////////////////////
// Audio callback function

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    petal.ProcessAnalogControls();

    float freq_val = petal.knob[Terrarium::KNOB_1].Process();
    float gain_val = petal.knob[Terrarium::KNOB_2].Process();
    float q_val    = petal.knob[Terrarium::KNOB_3].Process();

    const float minGain = -0.0f;
    const float maxGain = 24.0f;

    const float minFreq = 80.0f;
    const float maxFreq = 3000.0f;

    const float minQ = 0.1f;
    const float maxQ = 10.0f;

    float gain = (gain_val * (maxGain - minGain)) + minGain;
    float freq = minFreq * powf(maxFreq / minFreq, freq_val);
    float q    = (q_val * (maxQ - minQ)) + minQ;

    eq.SetParams(freq, q, gain);
    // eq.SetParams(1000.0f, 6.0f, 24.0f); // Default values for testing

    for (size_t i = 0; i < size; i++)
    {
        float input = in[0][i];
        float out_sample = eq.Process(input);
        out[0][i] = out_sample;
        out[1][i] = out_sample;
    }
}

int main(void)
{
    petal.Init();
    InitLeds();
    petal.StartAdc();
    InitParams();

    led1.Set(1.0f);  // Turn off LED1
    led2.Set(1.0f);  // Turn off LED2
    led1.Update();  // Apply to LED pin
    led2.Update();  // Apply to LED pin

    sample_rate = petal.AudioSampleRate();

    eq.SetSampleRate(sample_rate);
    eq.SetParams(1000.0f, 1.0f, 0.0f); // Default

    petal.StartAudio(AudioCallback);

    while (1) {
        // petal.ProcessAnalogControls();

        // float k1 = petal.knob[Terrarium::KNOB_1].Process();
        // led1.Set(k1 > 0.5f); // LED1 on if knob1 > 50%
        // led1.Update();

        // float k2 = petal.knob[Terrarium::KNOB_2].Process();
        // led2.Set(k2 < 0.5f); // LED2 on if knob2 > 50%
        // led2.Update();

        // petal.DelayMs(10); // allow time for hardware to respond
    }
}
