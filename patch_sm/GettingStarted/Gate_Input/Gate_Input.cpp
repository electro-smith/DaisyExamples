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
        /** Get the current gate in state */
        bool state = hw.gate_in_1.State();

        /** Set the led to the gate state */
        hw.SetLed(state);
    }
}
