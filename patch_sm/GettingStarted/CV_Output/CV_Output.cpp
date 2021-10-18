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

    /** Create signal variable */
    float signal = 0;

    /** Loop forever */
    while(1)
    {
        /** Increment the signal */
        signal += .01;

        /** If the signal gets too large... */
        if(signal >= 5)
        {
            /** Wrap around back to 0 */
            signal = signal - 5;
        }

        /** Write to CV_OUT_1 */
        patch.WriteCvOut(1, signal);

        /** Wait for 1 ms */
        patch.Delay(1);
    }
}
