#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;  // hardware object for the patch_sm

int main(void)
{
	hw.Init(); // initialize the patch_sm hardware object

	float signal = 0.f; // create signal variable
	while(1) { // loop forever
		signal += .01f; // increment the signal

		// if the signal gets too large...
		if (signal >= 5.f){
			signal = signal - 5.f; // wrap around back to 0
		}

		hw.WriteCvOut(1, signal); // write to CV_OUT_1

		hw.Delay(1);
	}
}
