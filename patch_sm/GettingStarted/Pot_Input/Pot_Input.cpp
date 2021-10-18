#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Hardware object for the patch_sm */
DaisyPatchSM patch;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    patch.Init();

    /** Loop forever */
    while(1)
    {
        /** Update all cv inputs */
        patch.ProcessAllControls();

        /** Get CV_1 Input (0, 1) */
        float value = patch.GetAdcValue(CV_1);

        /** Here the pot is wired to GND and 5V
          * So 2.5V is the pot's halfway point.
          * 
          * If the Adc value is greater than .5 (2.5V)... 
        */
        if(value > .5)
        {
            /** Turn the led on */
            patch.SetLed(true);
        }
        else
        {
            /** Otherwise, turn the led off */
            patch.SetLed(false);
        }
    }
}