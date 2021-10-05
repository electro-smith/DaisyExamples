#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;        // hardware object for the patch_sm
Switch       momentary; // switch object

int main(void)
{
    hw.Init(); // initialize the patch_sm hardware object

    /* initialize the switch
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

    while(1)
    {                                     // loop forever
        momentary.Debounce();             // debounce the switch
        bool state = momentary.Pressed(); // get the current momentary state
        hw.SetLed(state); // set the onboard led to the current state
        hw.Delay(1); // delay 1 millisecond to achieve the 1000 hz update rate
    }
}
