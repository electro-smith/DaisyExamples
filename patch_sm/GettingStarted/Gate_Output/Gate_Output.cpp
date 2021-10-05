#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw; // hardware object for the patch_sm

int main(void)
{
	hw.Init(); // initialize the patch_sm hardware object

	while(1) { // loop forever
		dsy_gpio_write(&hw.gate_out_1, true); // Set the gate high
		hw.Delay(250); // wait 250 ms
		dsy_gpio_write(&hw.gate_out_1, false); // Set the gate low
		hw.Delay(250); // wait 250 ms
	}
}
