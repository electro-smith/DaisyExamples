#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM patch;

int main(void)
{
    /** Initialize the hardware */
    patch.Init();
    while(1)
    {
        /** Delay half a second */
        patch.Delay(500);
        /** Get a truly random float from the hardware */
        float voltage = patch.GetRandomFloat(0.0, 5.0);
        /** Write it to both CV Outputs */
        patch.WriteCvOut(CV_OUT_BOTH, voltage);
    }
}
