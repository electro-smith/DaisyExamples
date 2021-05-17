#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

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
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1)
    {
        //OLED
        patch.display.Fill(false);

        patch.display.SetCursor(0, 0);
        std::string str  = "Sequential Switch";
        char*       cstr = &str[0];
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 25);
        str = isMultiIn ? "Multi " : "Single ";
        str += "In -> ";
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.SetCursor(0, 35);
        str = isMultiIn ? "Single " : "Multi ";
        str += "Out";
        patch.display.WriteString(cstr, Font_7x10, true);

        patch.display.Update();
    }
}

void UpdateControls()
{
    patch.ProcessDigitalControls();
    if(patch.gate_input[0].Trig())
    {
        step++;
        step %= 4;
    }

    if(patch.gate_input[1].Trig())
    {
        step = 0;
    }

    if(patch.encoder.RisingEdge())
    {
        isMultiIn = !isMultiIn;
    }
}
