#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

int  values[5];
bool trigs[5];
int  stepNumber;
bool trigOut;

int  menuPos;
bool inSubMenu;

void UpdateControls();
void UpdateOled();
void UpdateOutputs();

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    //init global vars
    stepNumber = 0;
    trigOut    = false;
    menuPos    = 0;
    inSubMenu  = false;

    for(int i = 0; i < 5; i++)
    {
        values[i] = 0.f;
        trigs[i]  = false;
    }

    patch.StartAdc();
    while(1)
    {
        UpdateControls();
        UpdateOled();
        UpdateOutputs();
    }
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    //encoder
    //can we simplify the menu logic?
    if(!inSubMenu)
    {
        menuPos += patch.encoder.Increment();
        menuPos = (menuPos % 10 + 10) % 10;

        if(menuPos < 5)
        {
            inSubMenu = patch.encoder.RisingEdge() ? true : false;
        }
        else
        {
            trigs[menuPos % 5] = patch.encoder.RisingEdge()
                                     ? !trigs[menuPos % 5]
                                     : trigs[menuPos % 5];
        }
    }

    else
    {
        values[menuPos] += patch.encoder.Increment();
        values[menuPos] = values[menuPos] < 0.f ? 0.f : values[menuPos];
        values[menuPos] = values[menuPos] > 60.f ? 60.f : values[menuPos];
        inSubMenu       = patch.encoder.RisingEdge() ? false : true;
    }

    //gate in
    if(patch.gate_input[0].Trig() || patch.gate_input[1].Trig())
    {
        stepNumber++;
        stepNumber %= 5;
        trigOut = trigs[stepNumber];
    }
}

void UpdateOled()
{
    patch.display.Fill(false);

    std::string str  = "!";
    char*       cstr = &str[0];
    patch.display.SetCursor(25 * stepNumber, 45);
    patch.display.WriteString(cstr, Font_7x10, true);

    //values and trigs
    for(int i = 0; i < 5; i++)
    {
        sprintf(cstr, "%d", values[i]);
        patch.display.SetCursor(i * 25, 10);
        patch.display.WriteString(cstr, Font_7x10, true);

        str = trigs[i % 5] ? "X" : "O";
        patch.display.SetCursor(i * 25, 30);
        patch.display.WriteString(cstr, Font_7x10, true);
    }

    //cursor
    str = inSubMenu ? "@" : "o";
    patch.display.SetCursor((menuPos % 5) * 25, (menuPos > 4) * 20);
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.Update();
}

void UpdateOutputs()
{
    patch.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              round((values[stepNumber] / 12.f) * 819.2f));
    patch.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              round((values[stepNumber] / 12.f) * 819.2f));

    dsy_gpio_write(&patch.gate_output, trigOut);
    trigOut = false;
}
