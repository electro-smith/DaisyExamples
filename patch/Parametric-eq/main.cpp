#include "daisy_patch.h"
#include "daisysp.h"
#include <cmath>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;
float sample_rate;

// -------------------------
// Custom Parametric EQ
// -------------------------
struct ParametricEQ
{
    float a0, a1, a2, b0, b1, b2;
    float z1 = 0.0f, z2 = 0.0f;
    float sample_rate = 48000.0f;

    void SetSampleRate(float sr)
    {
        sample_rate = sr;
    }

    void SetParams(float freq, float q, float gain_db)
    {
        float A     = powf(10.0f, gain_db / 40.0f);
        float omega = 2.0f * M_PI * freq / sample_rate;
        float alpha = sinf(omega) / (2.0f * q);
        float cosw  = cosf(omega);
    
        float b0 = 1 + alpha * A;
        float b1 = -2 * cosw;
        float b2 = 1 - alpha * A;
        float a0 = 1 + alpha / A;
        float a1 = -2 * cosw;
        float a2 = 1 - alpha / A;
    
        // Normalize
        this->b0 = b0 / a0;
        this->b1 = b1 / a0;
        this->b2 = b2 / a0;
        this->a1 = a1 / a0;
        this->a2 = a2 / a0;
    }
    

    float Process(float in)
    {
        float out = b0 * in + b1 * z1 + b2 * z2 - a1 * z1 - a2 * z2;
        z2 = z1;
        z1 = out;
        return out;
    }
};

ParametricEQ eq_left, eq_right;

// -------------------------
// EQ Initialization & Update
// -------------------------

void InitEQ(ParametricEQ& eq)
{
    eq.SetSampleRate(sample_rate);
    eq.SetParams(1000.0f, 1.0f, 0.0f); // center, Q, gain
}

void UpdateEQParams()
{
    patch.ProcessAnalogControls();

    float freq = patch.controls[DaisyPatch::CTRL_1].Process();
    float gain = patch.controls[DaisyPatch::CTRL_2].Process();
    float q    = patch.controls[DaisyPatch::CTRL_3].Process();

    // float mappedFreq = 20.0f * powf(10.0f, freq * 2.7f); // 20Hz–10kHz
    // float mappedGain = (gain * 24.0f) - 12.0f;           // -12dB to +12dB
    // float mappedQ    = 0.1f + q * 9.9f;                  // 0.1 to 10.0

    float mappedFreq = 100.0f + freq * 9900.0f; // 100Hz–10kHz (linear)
    float mappedGain = (gain * 40.0f) - 20.0f;  // -20 to +20 dB
    float mappedQ    = 0.1f + q * 9.9f;

    eq_left.SetParams(mappedFreq, mappedQ, mappedGain);
    eq_right.SetParams(mappedFreq, mappedQ, mappedGain);

    // eq_left.SetParams(1000.0f, 0.5f, 12.0f);  // 1kHz, narrow, +12dB
    // eq_right.SetParams(1000.0f, 0.5f, 12.0f);
}

// -------------------------
// Audio Callback
// -------------------------

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    patch.ProcessAllControls();
    UpdateEQParams();

    for (size_t i = 0; i < size; i++)
    {
        float left = in[0][i];
        float right = in[1][i];

        // out[0][i] = eq_left.Process(left);
        // out[1][i] = eq_right.Process(right);
        out[0][i] = in[0][i]; // bypass
        out[1][i] = in[1][i];
    }
}

// -------------------------
// Main Entry Point
// -------------------------

int main(void)
{
    patch.Init();
    sample_rate = patch.AudioSampleRate();

    InitEQ(eq_left);
    InitEQ(eq_right);

    patch.StartAdc();
    patch.StartAudio(AudioCallback);

    while (1)
    {
        patch.DelayMs(10);
    }
}
