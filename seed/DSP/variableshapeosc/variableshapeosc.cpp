#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed               hw;
VariableShapeOscillator variosc;
Oscillator              lfo, lfo2;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float mod  = lfo.Process();
        float mod2 = lfo2.Process();
        variosc.SetSyncFreq(110.f * (mod + 3));
        variosc.SetPW(mod * .8f);
        variosc.SetWaveshape(mod2);

        out[0][i] = out[1][i] = variosc.Process();
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    variosc.Init(sample_rate);
    variosc.SetSync(true);
    variosc.SetFreq(110.f);

    lfo.Init(sample_rate);
    lfo.SetAmp(1.f);
    lfo.SetFreq(.25f);

    lfo2.Init(sample_rate);
    lfo2.SetAmp(1.f);
    lfo2.SetFreq(.125f);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
