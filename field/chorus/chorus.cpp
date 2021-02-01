#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField hw;
Chorus     ch;

bool effectOn;

void Controls()
{
    hw.ProcessAllControls();

    //knobs
    float k = hw.knob[0].Process() * .5f;
    ch.SetPan(.5f - k, .5f + k);

    ch.SetLfoFreq(hw.knob[1].Process() * 6.f);
    ch.SetLfoDepth(hw.knob[2].Process());
    ch.SetDelay(hw.knob[3].Process(), hw.knob[4].Process());
    ch.SetFeedback(hw.knob[5].Process());

    //activate / deactivate effect
    effectOn ^= hw.sw[0].RisingEdge();
}

void AudioCallback(float **in, float **out, size_t size)
{
    Controls();

    for(size_t i = 0; i < size; i++)
    {
        ch.Process(in[0][i]);
        out[0][i] = effectOn ? ch.GetLeft() : in[0][i];
        out[1][i] = effectOn ? ch.GetRight() : in[0][i];
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    ch.Init(sample_rate);

    effectOn = true;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.display.Fill(false);

        char cstr[15];
        sprintf(cstr, "Effect: %s", effectOn ? "On" : "Off");
        hw.display.SetCursor(0, 0);
        hw.display.WriteString(cstr, Font_7x10, true);

        hw.display.Update();
    }
}
