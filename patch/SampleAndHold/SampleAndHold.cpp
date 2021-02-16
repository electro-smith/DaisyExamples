#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

struct sampHoldStruct
{
    SampleHold       sampHold;
    SampleHold::Mode mode;
    float            output;

    void Process(bool trigger, float input)
    {
        output = sampHold.Process(trigger, input, mode);
    }
};

sampHoldStruct sampHolds[2];

int menuPos;

void UpdateControls();
void UpdateOutputs();
void UpdateOled();

int main(void)
{
    patch.Init();

    patch.StartAdc();
    while(1)
    {
        UpdateControls();
        UpdateOutputs();
        UpdateOled();
    }
}

void UpdateControls()
{
    patch.ProcessAnalogControls();
    patch.ProcessDigitalControls();

    //read ctrls and gates, then update sampleholds
    sampHolds[0].Process(patch.gate_input[0].State(),
                         patch.controls[0].Process());
    sampHolds[1].Process(patch.gate_input[1].State(),
                         patch.controls[1].Process());

    //encoder
    menuPos += patch.encoder.Increment();
    menuPos = (menuPos % 2 + 2) % 2;

    //switch modes
    if(patch.encoder.RisingEdge())
    {
        sampHolds[menuPos].mode
            = (SampleHold::Mode)((sampHolds[menuPos].mode + 1) % 2);
    }
}

void UpdateOutputs()
{
    patch.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              sampHolds[0].output * 4095);
    patch.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              sampHolds[1].output * 4095);
}

void UpdateOled()
{
    patch.display.Fill(false);

    std::string str  = "Sample/Track and Hold";
    char*       cstr = &str[0];
    patch.display.SetCursor(0, 0);
    patch.display.WriteString(cstr, Font_6x8, true);

    //Cursor
    patch.display.SetCursor(menuPos * 60, 25);
    str = "o";
    patch.display.WriteString(cstr, Font_7x10, true);

    //two circuits
    patch.display.SetCursor(0, 35);
    str = sampHolds[0].mode == 0 ? "S&H" : "T&H";
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.SetCursor(60, 35);
    str = sampHolds[1].mode == 0 ? "S&H" : "T&H";
    patch.display.WriteString(cstr, Font_7x10, true);

    patch.display.Update();
}
