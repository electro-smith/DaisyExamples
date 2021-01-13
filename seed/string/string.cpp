#include "daisysp.h"
#include "daisy_seed.h"
#include <algorithm>

// Shortening long macro for sample rate
#ifndef sample_rate

#endif

// Interleaved audio definitions
#define LEFT (i)
#define RIGHT (i + 1)

using namespace daisysp;
using namespace daisy;

static DaisySeed seed;

// Helper Modules
static Metro tick;
static String str;

// MIDI note numbers for a major triad
const float kArpeggio[3] = {48.0f, 52.0f, 55.0f};
uint8_t     arp_idx;

static void AudioCallback(float *in, float *out, size_t size)
{
    float sig_out, freq, trig;
    for(size_t i = 0; i < size; i += 2)
    {
        // When the Metro ticks:
        // advance the kArpeggio, and trigger the Pluck.
        trig = 0.0f;
        if(tick.Process())
        {
            freq    = mtof(kArpeggio[arp_idx]); // convert midi nn to frequency.
            arp_idx = (arp_idx + 1)
                      % 3; // advance the kArpeggio, wrapping at the end.
            str.SetFreq(freq);
            trig = 1.0f;
        }
        sig_out = str.Process(trig);
        // Output
        out[LEFT]  = sig_out;
        out[RIGHT] = sig_out;
    }
}

int main(void)
{
    // initialize seed hardware and daisysp modules
    float sample_rate;
    seed.Configure();
    seed.Init();
    sample_rate = seed.AudioSampleRate();

    // Set up Metro to pulse every second
    tick.Init(1.0f, sample_rate);
	
    // Set up String algo
    str.Init(sample_rate);
    str.SetDamping(.8f);
	str.SetNonLinearity(.1f);
	str.SetBrightness(.5f);

    arp_idx = 0;

    // start callback
    seed.StartAudio(AudioCallback);


    while(1) {}
}
