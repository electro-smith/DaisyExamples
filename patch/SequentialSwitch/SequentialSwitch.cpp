#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

bool    isMultiIn;
uint8_t step;

void UpdateControls();

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    UpdateControls();

    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = out[1][i] = out[2][i] = out[3][i]
            = 0.f; //reset non active outs
        if(isMultiIn)
            out[0][i] = in[step][i];
        else
            out[step][i] = in[0][i];
    }
}

int main(void)
{
    hw.Init(); // Initialize hardware (daisy seed, and patch)

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    while(1)
    {
        //OLED
        hw.display.Fill(false);

        hw.display.SetCursor(0, 0);
        std::string str  = "Sequential Switch";
        char*       cstr = &str[0];
        hw.display.WriteString(cstr, Font_7x10, true);

        hw.display.SetCursor(0, 25);
        str = isMultiIn ? "Multi " : "Single ";
        str += "In -> ";
        hw.display.WriteString(cstr, Font_7x10, true);

        hw.display.SetCursor(0, 35);
        str = isMultiIn ? "Single " : "Multi ";
        str += "Out";
        hw.display.WriteString(cstr, Font_7x10, true);

        hw.display.Update();
    }
}

void UpdateControls()
{
    hw.ProcessDigitalControls();
    if(hw.gate_input[0].Trig())
    {
        step++;
        step %= 4;
    }

    if(hw.gate_input[1].Trig())
    {
        step = 0;
    }

    if(hw.encoder.RisingEdge())
    {
        isMultiIn = !isMultiIn;
    }
}
