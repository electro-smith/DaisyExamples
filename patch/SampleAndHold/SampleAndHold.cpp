#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch hw;

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
    hw.Init();

    hw.StartAdc();
    while(1)
    {
        UpdateControls();
        UpdateOutputs();
        UpdateOled();
    }
}

void UpdateControls()
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    //read ctrls and gates, then update sampleholds
    sampHolds[0].Process(hw.gate_input[0].State(),
                         hw.controls[0].Process());
    sampHolds[1].Process(hw.gate_input[1].State(),
                         hw.controls[1].Process());

    //encoder
    menuPos += hw.encoder.Increment();
    menuPos = (menuPos % 2 + 2) % 2;

    //switch modes
    if(hw.encoder.RisingEdge())
    {
        sampHolds[menuPos].mode
            = (SampleHold::Mode)((sampHolds[menuPos].mode + 1) % 2);
    }
}

void UpdateOutputs()
{
    hw.seed.dac.WriteValue(DacHandle::Channel::ONE,
                              sampHolds[0].output * 4095);
    hw.seed.dac.WriteValue(DacHandle::Channel::TWO,
                              sampHolds[1].output * 4095);
}

void UpdateOled()
{
    hw.display.Fill(false);

    std::string str  = "Sample/Track and Hold";
    char*       cstr = &str[0];
    hw.display.SetCursor(0, 0);
    hw.display.WriteString(cstr, Font_6x8, true);

    //Cursor
    hw.display.SetCursor(menuPos * 60, 25);
    str = "o";
    hw.display.WriteString(cstr, Font_7x10, true);

    //two circuits
    hw.display.SetCursor(0, 35);
    str = sampHolds[0].mode == 0 ? "S&H" : "T&H";
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.SetCursor(60, 35);
    str = sampHolds[1].mode == 0 ? "S&H" : "T&H";
    hw.display.WriteString(cstr, Font_7x10, true);

    hw.display.Update();
}
