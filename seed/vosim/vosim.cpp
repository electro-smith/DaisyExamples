#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed       hw;
VosimOscillator vosim;
Oscillator      lfo;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float mod = lfo.Process();
        vosim.SetForm2Freq(817.2f * (mod + 1.f));
        vosim.SetShape(mod);

        out[0][i] = out[1][i] = vosim.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    vosim.Init(sample_rate);
    vosim.SetFreq(105.f);
    vosim.SetForm1Freq(1390.7);

    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(.5f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
