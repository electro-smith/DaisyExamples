#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

Flanger    flanger;
DaisyField hw;

bool effectOn = true;

float del, deltarget;
float lfo, lfotarget;

float knob_vals[4];
void  AudioCallback(AudioHandle::InputBuffer  in,
                    AudioHandle::OutputBuffer out,
                    size_t                    size)
{
    hw.ProcessAllControls();

    for(int i = 0; i < 4; i++)
    {
        knob_vals[i] = hw.knob[i].Process();
    }

    deltarget = hw.knob[0].Process();
    flanger.SetFeedback(hw.knob[1].Process());
    float val = hw.knob[2].Process();
    flanger.SetLfoFreq(val * val * 10.f);
    lfotarget = hw.knob[3].Process();

    effectOn ^= hw.sw[0].RisingEdge();

    for(size_t i = 0; i < size; i++)
    {
        fonepole(del, deltarget, .0001f); //smooth at audio rate
        flanger.SetDelay(del);

        fonepole(lfo, lfotarget, .0001f); //smooth at audio rate
        flanger.SetLfoDepth(lfo);

        out[0][i] = out[1][i] = effectOn ? flanger.Process(in[0][i]) : in[0][i];
    }
}

int main(void)
{
    hw.Init();
    float sample_rate = hw.AudioSampleRate();

    flanger.Init(sample_rate);

    del = deltarget = 0.f;
    lfo = lfotarget = 0.f;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.display.Fill(false);

        char cstr[11];
        sprintf(cstr, "Effect %s", effectOn ? "On" : "Off");
        hw.display.SetCursor(0, 0);
        hw.display.WriteString(cstr, Font_7x10, true);

        hw.display.Update();

        size_t knob_leds[] = {
            DaisyField::LED_KNOB_1,
            DaisyField::LED_KNOB_2,
            DaisyField::LED_KNOB_3,
            DaisyField::LED_KNOB_4,
        };

        for(size_t i = 0; i < 4; i++)
        {
            hw.led_driver.SetLed(knob_leds[i], knob_vals[i]);
        }

        hw.led_driver.SwapBuffersAndTransmit();
    }
}
