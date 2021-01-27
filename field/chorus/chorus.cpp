#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField hw;
Chorus<>   ch; //2 channels with 2 delay lines each

bool  effectOn;
float wet;

void Controls()
{
    hw.ProcessAllControls();

    //knobs
    float spread = hw.knob[0].Process() * 1.f;

    ch.SetLfoFreq(hw.knob[1].Process() * 2.5f, 0, 0);
    ch.SetLfoFreq(hw.knob[1].Value() * 2.5f + spread, 1, 1);
    ch.SetLfoFreq(hw.knob[1].Process() * 2.f, 0, 1);
    ch.SetLfoFreq(hw.knob[1].Value() * 3.f + spread, 1, 1);

    ch.SetLfoDepth(hw.knob[2].Process() * 6.5f, 0);
    ch.SetLfoDepth(hw.knob[2].Process() * 6.5f + 2.f * spread, 1);
    ch.SetLfoDepth(hw.knob[2].Process() * 6.f, 0, 1);
    ch.SetLfoDepth(hw.knob[2].Process() * 6.f + 1.5f * spread, 1, 1);

    ch.SetDelay(hw.knob[3].Process() * 10.f, 0);
    ch.SetDelay(hw.knob[3].Value() * 10.f + 5.f * spread, 1);
    ch.SetDelay(hw.knob[3].Process() * 9.f, 0, 1);
    ch.SetDelay(hw.knob[3].Value() * 9.f + 6.f * spread, 1, 1);

	wet = hw.knob[4].Process();

	//activate / deactivate effect
    effectOn ^= hw.sw[0].RisingEdge();
}

void AudioCallback(float **in, float **out, size_t size)
{
    Controls();

    for(size_t i = 0; i < size; i++)
    {
        float sigl = effectOn ? ch.Process(in[0][i], 0) : in[0][i];
        float sigr = effectOn ? ch.Process(in[0][i], 1) : in[0][i];

        out[0][i] = sigl * wet + in[0][i] * (1.f - wet);
        out[1][i] = sigr * wet + in[0][i] * (1.f - wet);
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    ch.Init(sample_rate);

    effectOn = true;
    wet      = .5f;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1) {}
}
