#include <stdio.h>
#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

DaisySeed hw;
Pluck     string;
Metro     clk;

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig_out, freq, trig;
    for(size_t i = 0; i < size; i += 2)
    {
        trig = 0.0f;
        if(clk.Process())
        {
            freq = rand() % 1000;
            string.SetFreq(freq);
            trig = 1;
        }

        sig_out = string.Process(trig);

        // Output
        out[LEFT]  = sig_out;
        out[RIGHT] = sig_out;
    }
}

int main(void)
{
    float init_buff[256]; // buffer for Pluck impulse

    float sample_rate;
    hw.Configure();
    hw.Init();
    sample_rate = hw.AudioSampleRate();

    // Set up Metro to pulse every second
    clk.Init(1.0f, sample_rate);

    // Set up Pluck algo
    string.Init(sample_rate, init_buff, 256, PLUCK_MODE_RECURSIVE);
    string.SetDecay(0.95f);
    string.SetDamp(0.9f);
    string.SetAmp(0.3f);

    // start callback
    hw.StartAudio(AudioCallback);

    while(1) {}
}
