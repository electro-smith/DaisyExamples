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

/** Object to do PWM brightness on the onboard led */
Led led;

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();

    /** Initialize led object
    - Led will be on the onboard_led pin
    - Won't need to invert the signal
    */
    led.Init(hw.user_led.pin, false);

    /** Loop forever */
    while(1)
    {
        /** Update all cv inputs */
        hw.ProcessAllControls();

        /** Get CV_1 Input (0, 1) */
        float val = hw.GetAdcValue(0);

        /** Set led brightness */
        led.Set(val);

        /** Update led pwm */
        led.Update();
    }
}