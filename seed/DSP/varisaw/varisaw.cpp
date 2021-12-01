#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed             hw;
VariableSawOscillator varisaw;
Oscillator            lfo;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float sig = lfo.Process();
        varisaw.SetPW(sig);

        out[0][i] = out[1][i] = varisaw.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    varisaw.Init(sample_rate);
    varisaw.SetFreq(62.5f);
    varisaw.SetWaveshape(1.f);

    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(1.f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
