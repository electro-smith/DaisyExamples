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
        /** Update the control ins */
        hw.ProcessAllControls();

        /** Read from CV_1 (-1, 1) */
        float value = hw.GetAdcValue(CV_5);

        /** If the Adc value is greater than 0V... */
        if(value > 0)
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
