// jitter module example

#include "daisysp.h"
#include "daisy_seed.h"

// Shortening long macro for sample rate
#ifndef sample_rate

#endif

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static Jitter     jitter;
static Oscillator osc;
bool              gate;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float osc_out, jitter_out;
    for(size_t i = 0; i < size; i += 2)
    {
        // Use jitter to control the amplitude of the oscillator.
        jitter_out = jitter.Process();
        osc.SetAmp(jitter_out);
        osc_out = osc.Process();

        out[LEFT]  = osc_out;
        out[RIGHT] = osc_out;
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    sample_rate = hw.AudioSampleRate();
    osc.Init(sample_rate);

    //set jitter parameters
    jitter.Init(sample_rate);
    jitter.SetAmp(1);
    jitter.SetCpsMin(1);
    jitter.SetCpsMax(25);

    // Set parameters for oscillator
    osc.SetWaveform(osc.WAVE_TRI);
    osc.SetFreq(440);
    osc.SetAmp(0.25);

    // start callback
    hw.StartAudio(AudioCallback);

    while(1) {}
}
