#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

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

bool isNegated[6]; //we include extra spots to deal with cursorLoc

int cursorLocation;
bool inSubMenu;  //true if we're in a submenu

gate gates[2];
std::string gateNames[6];

bool cvToBool(float in) { return in > .9; }

void UpdateControls();
void UpdateOled();
void UpdateOutputs();

void InitGateNames()
{
    gateNames[0] = " AND  ";
    gateNames[1] = "  OR  ";
    gateNames[2] = " XOR  ";
    gateNames[3] = " NAND ";
    gateNames[4] = " NOR  ";
    gateNames[5] = " XNOR ";
}

int main(void)
{
    patch.Init();

    InitGateNames();

    gates[0].gateType = 1;
    gates[1].gateType = 3;
    
    isNegated[0] = isNegated[1] = false;
    isNegated[2] = isNegated[3] = false;
    isNegated[4] = isNegated[5] = false;

    cursorLocation = 0;
    inSubMenu = false;

    patch.StartAdc();
    
    while(1) 
    {
        patch.DelayMs(1);
        UpdateControls();
        UpdateOled();
        UpdateOutputs();
    }
}

void UpdateControls()
{
    if (inSubMenu){
        int gateNum = (cursorLocation == 4) * 3 + 1;
        gates[gateNum].gateType += patch.encoder.Increment();
        gates[gateNum].gateType = (gates[gateNum].gateType % 6 + 6) % 6;
    }

    else {
        cursorLocation += patch.encoder.Increment();
        cursorLocation = (cursorLocation % 5 + 5) % 5;
    }
     
    //if we're on a number and the encoder is pressed, flip the negation
    if (cursorLocation != 1 && cursorLocation != 4 && patch.encoder.RisingEdge())
    {
        isNegated[cursorLocation] = !isNegated[cursorLocation];
    }
    //if not, enter or leave the submenu
    else if (patch.encoder.RisingEdge())
    {
        inSubMenu = !inSubMenu;
    }
    
}

void UpdateOled()
{
    patch.display.Fill(false);

    patch.display.SetCursor(0,0);
    std::string str = "Logic";
    char* cstr = &str[0];
    patch.display.WriteString(cstr, Font_7x10, true);

    //gates
    patch.display.SetCursor(0,35);
    str = "1" + gateNames[gates[0].gateType] + "2";
    patch.display.WriteString(cstr, Font_6x8, true);

    patch.display.SetCursor(70,35);
    str = "3" + gateNames[gates[1].gateType] + "4";
    patch.display.WriteString(cstr, Font_6x8, true);

    //cursor

    patch.display.Update();
}

void UpdateOutputs()
{   
    bool a = cvToBool(patch.controls[0].Process());
    bool b = cvToBool(patch.controls[1].Process());
    bool c = cvToBool(patch.controls[2].Process());
    bool d = cvToBool(patch.controls[3].Process());

    dsy_dac_write(DSY_DAC_CHN1, gates[0].Process(a,b) * 4095);
    dsy_dac_write(DSY_DAC_CHN2, gates[1].Process(c,d) * 4095);
}