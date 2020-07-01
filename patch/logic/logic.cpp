#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

struct gate{
    int gateType;
    bool Process(bool a, bool b)
    {
        switch(gateType)
        {
            case 0:
                return a && b;  //AND
            case 1:
                return a || b;  //OR
            case 2:
                return (a && !b) || (!a && b);  //XOR
            case 3:
                return !(a && b);  //NAND
            case 4:
                return !(a || b);  //NOR
            case 5:
                return (a && b) || (!a && !b);  //XNOR
        }
    }
};

bool inputs[4];
gate gates[2];
std::string gateNames[6];

bool cvToBool(float in) { return in > .8f; }

void InitGateNames()
{
    gateNames[0] = " AND  ";
    gateNames[1] = " OR   ";
    gateNames[2] = " XOR  ";
    gateNames[3] = " NAND ";
    gateNames[4] = " NOR  ";
    gateNames[5] = " XNOR ";
}

    void ProcessControls();
    void ProcessOled();
    void ProcessOutputs();


int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    InitGateNames();
    gates[0].gateType = gates[1].gateType = 0; //both init to AND

    patch.StartAdc();
    while(1)
    {
        ProcessControls();
        ProcessOled();
        ProcessOutputs();
    }
}

void ProcessControls()
{
    //inputs
    for (int i = 0; i < 4; i++)
    {
        inputs[i] = cvToBool(patch.controls[i].Process());
    }
}

void ProcessOutputs()
{
    dsy_dac_write(DSY_DAC_CHN1, gates[0].Process(inputs[0], inputs[1]));
    dsy_dac_write(DSY_DAC_CHN2, gates[1].Process(inputs[2], inputs[3]));
}
