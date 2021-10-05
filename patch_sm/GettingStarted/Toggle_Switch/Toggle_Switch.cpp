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
Switch       toggle;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init(); 

    /* Initialize the switch
	 - We'll read the switch on pin B8
	 - The switch will be read at a rate of 1 kHz. (Achieved with delay in loop)
	*/
    toggle.Init(hw.B8, 1000);

    /** Loop forever */
    while(1)
    {                                 
        /** Debounce the switch */
        toggle.Debounce();           
    
        /** Get the current toggle state */
        bool state = toggle.Pressed(); 
    
        /** Set the onboard led to the current state */
        hw.SetLed(state);
        
        /** Delay 1 ms to achieve the 1000 hz update rate */
        hw.Delay(1);
    }
}
