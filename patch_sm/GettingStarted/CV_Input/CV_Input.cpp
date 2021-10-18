#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
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
        /** Update the control ins */
        patch.ProcessAllControls();

        /** Read from CV_1 (-1, 1) */
        float value = patch.GetAdcValue(0);

        /** Turn the onboard led on for positive CV, and off for negative */
        patch.SetLed(value > 0.f);
    }
}
