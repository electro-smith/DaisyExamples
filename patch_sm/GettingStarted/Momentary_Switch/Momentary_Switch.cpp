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

/** Switch object */
Switch       momentary;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /* Initialize the switch
	 - We'll read the switch on pin B7
	 - The switch will be read at a rate of 1000hz
	 - We're using a momentary switch
	 - We'll need to invert the signal
	 - We'll use the internal pullup
	*/
    momentary.Init(hw.B7,
                   1000,
                   Switch::Type::TYPE_MOMENTARY,
                   Switch::Polarity::POLARITY_INVERTED,
                   Switch::Pull::PULL_UP);

    /** loop forever */
    while(1)
    {
        /** Debounce the switch */
        momentary.Debounce();

        /** Get the current momentary state */
        bool state = momentary.Pressed();

        /** Set the onboard led to the current state */
        hw.SetLed(state);

        /** Delay 1 ms to achieve the 1000 hz update rate */
        hw.Delay(1);
    }
}
