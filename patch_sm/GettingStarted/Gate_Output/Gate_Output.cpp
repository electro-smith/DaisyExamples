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
        /** Set the gate high */
        patch.gate_out_1.Write(true);

        /** Wait 250 ms */
        patch.Delay(250);

        /** Set the gate low */
        patch.gate_out_1.Write(false);

        /** Wait 250 ms */
        patch.Delay(250);
    }
}
