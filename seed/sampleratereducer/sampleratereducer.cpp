#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed         hw;
Oscillator        osc, lfo;
SampleRateReducer sr;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        sr.SetFreq(fabsf(lfo.Process()));
        out[0][i] = out[1][i] = sr.Process(osc.Process());
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    osc.Init(sample_rate);
    osc.SetFreq(440.f);

    lfo.Init(sample_rate);
    lfo.SetFreq(.1f);
    lfo.SetAmp(.25f);

    sr.Init();

    hw.StartAudio(AudioCallback);
    while(1) {}
}
