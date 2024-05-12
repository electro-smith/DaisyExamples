#include "daisy_patch_sm.h"

using namespace daisy;
using namespace patch_sm;

DaisyPatchSM hw;

int main(void)
{
    /** Initialize the hardware */
    hw.Init();
    while(1)
    {
        /** Delay half a second */
        hw.Delay(500);
        /** Get a truly random float from the hardware */
        float voltage = hw.GetRandomFloat(0.0, 5.0);
        /** Write it to both CV Outputs */
        hw.WriteCvOut(CV_OUT_BOTH, voltage);
    }
}
