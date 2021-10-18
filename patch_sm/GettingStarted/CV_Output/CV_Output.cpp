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
        /** Write 0V to CV_OUT_2 */
        patch.WriteCvOut(CV_OUT_2, 0);

        /** Wait for 1000 ms (1 second) */
        patch.Delay(1000);

        /** Write 2.5V to CV_OUT_2 */
        patch.WriteCvOut(CV_OUT_2, 2.5);

        /** Wait for 1000 ms (1 second) */
        patch.Delay(1000);

        /** Write 5V to CV_OUT_2 */
        patch.WriteCvOut(CV_OUT_2, 5);

        /** Wait for 1000 ms (1 second) */
        patch.Delay(1000);
    }
}
