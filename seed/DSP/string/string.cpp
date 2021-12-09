#include "daisysp.h"
#include "daisy_seed.h"
#include <algorithm>

using namespace daisysp;
using namespace daisy;

static DaisySeed hw;

// Helper Modules
static Metro      tick;
static String     str;
static Oscillator lfo;

// MIDI note numbers for a major triad
const float kArpeggio[3] = {48.0f, 52.0f, 55.0f};
uint8_t     arp_idx;

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        // When the Metro ticks:
        // advance the kArpeggio, and trigger the String.
        bool trig = tick.Process();
        if(trig)
        {
            // convert midi nn to frequency.
            float freq = mtof(kArpeggio[arp_idx]);

            // advance the kArpeggio, wrapping at the end.
            arp_idx = (arp_idx + 1) % 3;
            str.SetFreq(freq);
        }

        //modulate the string parameters using the lfo
        float sig = lfo.Process();

        str.SetBrightness(fabsf(sig));
        str.SetDamping(fabsf(sig) + .2f);
        str.SetNonLinearity(sig);

        //output the sample
        out[0][i] = out[1][i] = str.Process(trig);
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

    // Set up Metro to pulse every second
    tick.Init(2.0f, sample_rate);

    // Set up String algo
    str.Init(sample_rate);
    str.SetDamping(.8f);
    str.SetNonLinearity(.1f);
    str.SetBrightness(.5f);

    //setup lfo at .25Hz
    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(.1f);

    arp_idx = 0;

    // start callback
    hw.StartAudio(AudioCallback);


    while(1) {}
}
