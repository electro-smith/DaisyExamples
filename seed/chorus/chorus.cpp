#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
Chorus<2> chorus;

HiHat<>        hat;
AnalogBassDrum kick;

Metro tick;

uint8_t h_patt = 0B11111111;
uint8_t k_patt = 0B10001000;
uint8_t pos    = 0B10000000;

float ProcessDrums()
{
    bool t = tick.Process();
    pos    = pos >> t;

    if(pos == 0 && t)
    {
        pos = 0B10000000;
    }

    float sig = hat.Process(t && pos & (h_patt)) * .1f;
    sig += kick.Process(t && pos & (k_patt)) * 1.4f;

    return sig;
}

void AudioCallback(float **in, float **out, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        chorus.Process(ProcessDrums(), out[0][i], out[1][i]);
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    chorus.Init(sample_rate);

    chorus.SetLfoFreq(.33f, 0);
    chorus.SetLfoFreq(.2f, 1);

    chorus.SetLfoDepthAll(1.f);

    chorus.SetDelay(.75f, 0);
    chorus.SetDelay(.9f, 1);

    tick.Init(8.f, sample_rate);

    hat.Init(sample_rate);
    hat.SetDecay(.6f);

    kick.Init(sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
