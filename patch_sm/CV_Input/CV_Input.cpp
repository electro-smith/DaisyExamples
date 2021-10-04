#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw; // hardware object for the patch_sm

int main(void)
{
    hw.Init(); //initialize the patch_sm hardware object

    while(1)
    {                            // loop forever
        hw.ProcessAllControls(); // update the control ins

        float value = hw.GetAdcValue(0); // read from CV_1 (-1, 1)
        hw.SetLed(
            value
            > 0.f); //turn the onboard led on for positive CV, and off for negative
    }
}
