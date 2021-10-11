#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed  hw;
Oscillator osc, subosc;
Metro      tick;
Flanger    flanger0, flanger1;

float seq[] = {
    130.81f,
    155.56f,
    116.54f,
    98.f,
};
int pos = 0;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        if(tick.Process())
        {
            pos = (pos + 1) % 4;
            osc.SetFreq(seq[pos]);
            subosc.SetFreq(seq[pos] * .5f);
        }

        out[0][i] = flanger0.Process(osc.Process());
        out[1][i] = flanger1.Process(subosc.Process());
    }
}

int main(void)
{
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    float sample_rate = hw.AudioSampleRate();

    flanger0.Init(sample_rate);
    flanger0.SetLfoDepth(.5f);
    flanger0.SetFeedback(.83f);

    flanger1.Init(sample_rate);
    flanger1.SetLfoDepth(0.8f);
    flanger1.SetLfoFreq(.33f);
    flanger1.SetFeedback(.83f);

    osc.Init(sample_rate);
    osc.SetWaveform(Oscillator::WAVE_SAW);

    subosc.Init(sample_rate);
    subosc.SetWaveform(Oscillator::WAVE_SAW);


    tick.Init(3.f, sample_rate);

    hw.StartAudio(AudioCallback);
    while(1) {}
}
