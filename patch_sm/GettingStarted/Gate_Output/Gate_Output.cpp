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
        /** Set the gate high */
        dsy_gpio_write(&patch.gate_out_1, true);

        /** Wait 250 ms */
        patch.Delay(250);

        /** Set the gate low */
        dsy_gpio_write(&patch.gate_out_1, false);

        /** Wait 250 ms */
        patch.Delay(250);
    }
}