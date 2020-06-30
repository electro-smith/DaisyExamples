#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

struct gate
{
    int gateType;
    
    bool Process(bool a, bool b){
        switch (gateType){
            case 0:
                return a && b; //AND
            case 1:
                return a || b; //OR
            case 2:
                return (a && !b) || (!a && b);  //XOR
            case 3:
                return !(a && b); //NAND
            case 4:
                return !(a || b); //NOR
            case 5:
                return (a && b) || (!a && !b); //XNOR
            default:
                break;
        }
        return false;
    }
};

bool cvToBool(float in) { return in > .9; }

gate gates[2];

void UpdateControls();
void UpdateOled();
void UpdateOutputs();

int main(void)
{
    patch.Init();

    gates[0].gateType = gates[1].gateType = 0;
    
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
}

void UpdateOled(){}

void UpdateOutputs()
{   
    bool a = cvToBool(patch.controls[0].Process());
    bool b = cvToBool(patch.controls[1].Process());
    bool c = cvToBool(patch.controls[2].Process());
    bool d = cvToBool(patch.controls[3].Process());

    dsy_dac_write(DSY_DAC_CHN1, gates[0].Process(a,b) * 4096);
    dsy_dac_write(DSY_DAC_CHN2, gates[1].Process(c,d) * 4096);
}
