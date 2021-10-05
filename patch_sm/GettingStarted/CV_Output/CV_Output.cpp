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
DaisyPatchSM hw;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /** Create signal variable */
    float signal = 0.f;

    /** Loop forever */
    while(1)
    {
        /** Increment the signal */
        signal += .01f;

        /** If the signal gets too large... */
        if(signal >= 5.f)
        {
            /** Wrap around back to 0 */
            signal = signal - 5.f;
        }

        /** Write to CV_OUT_1 */
        hw.WriteCvOut(1, signal);

        /** Wait for 1 ms */
        hw.Delay(1);
    }
}
