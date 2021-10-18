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

/** Switch object */
Switch button;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    patch.Init();

    /* Initialize the switch
	 - We'll read the switch on pin B7
	*/
    button.Init(patch.B7);

    /** loop forever */
    while(1)
    {
        /** Debounce the switch */
        button.Debounce();

        /** Get the current button state */
        bool state = button.Pressed();

        /** Set the onboard led to the current state */
        patch.SetLed(state);

        /** Delay 1 ms to achieve the 1000 hz update rate */
        patch.Delay(1);
    }
}
