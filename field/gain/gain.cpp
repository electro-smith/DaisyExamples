#include "daisy_field.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyField hw;

bool effectOn = true;

float gain;

float knob_vals[1];
void  AudioCallback(AudioHandle::InputBuffer  in,
                    AudioHandle::OutputBuffer out,
                    size_t                    size)
{
    hw.ProcessAllControls();

    for(int i = 0; i < 1; i++)
    {
        knob_vals[i] = hw.knob[i].Process();
    }

    gain = hw.knob[0].Process();

    effectOn ^= hw.sw[0].RisingEdge();

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = out[1][i] = effectOn ? in[0][i] * gain : in[0][i];
    }
}

int main(void)
{
    hw.Init();
    gain = 0;

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        hw.display.Fill(false);

        char cstr[11];
        sprintf(cstr, "Effect %s", effectOn ? "On" : "Off");
        char gainstr[30];
        sprintf(gainstr, "Gain %.2f", gain);

        hw.display.SetCursor(0, 0);
        hw.display.WriteString(cstr, Font_7x10, true);

        hw.display.SetCursor(0, 10);
        hw.display.WriteString(gainstr, Font_7x10, true);

        hw.display.Update();

        size_t knob_leds[] = {
            DaisyField::LED_KNOB_1
        };

        for(size_t i = 0; i < 1; i++)
        {
            hw.led_driver.SetLed(knob_leds[i], knob_vals[i]);
        }

        hw.led_driver.SwapBuffersAndTransmit();
    }
}
