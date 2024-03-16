#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Hardware object for the patch_sm */
DaisyPatchSM hw;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /** Loop forever */
    while(1)
    {
        /** Update all cv inputs */
        hw.ProcessAllControls();

        /** Get CV_1 Input (0, 1) */
        float value = hw.GetAdcValue(CV_1);

        /** Here the pot is wired to GND and 5V
          * So 2.5V is the pot's halfway point.
          * 
          * If the Adc value is greater than .5 (2.5V)... 
        */
        if(value > .5)
        {
            /** Turn the led on */
            hw.SetLed(true);
        }
        else
        {
            /** Otherwise, turn the led off */
            hw.SetLed(false);
        }
    }
}