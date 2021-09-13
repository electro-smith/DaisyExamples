#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace daisysp;

/** Our hardware board class handles the interface to the actual DaisyPatchSM
 * hardware. */
DaisyPatchSM patch;

/** Create ADSR envelope module */
Adsr envelope;

/** Create a button to control the ADSR activation */
Switch button;

/** Similar to the audio callback, you can generate audio rate CV signals out of the CV outputs. 
 *  These signals are 12-bit DC signals that range from 0-5V out of the Patch SM
*/
void EnvelopeCallback(uint16_t **output, size_t size)
{
    /** Process the controls */
    patch.ProcessAnalogControls();
    button.Debounce();

    /** Set the input value of the ADSR */
    bool env_state;
    if(button.Pressed() || patch.gate_in_1.State())
        env_state = true;
    else
        env_state = false;

    /** Assign four controls to the times and level of the envelope segments 
     *  Attack, Decay, and Release will be set between instantaneous to 1 second
     *  Sustain will be set between 0 and 1 
     */
    envelope.SetTime(ADSR_SEG_ATTACK, patch.GetAdcValue(patch.CV_1));
    envelope.SetTime(ADSR_SEG_DECAY, patch.GetAdcValue(patch.CV_2));
    envelope.SetSustainLevel(patch.GetAdcValue(patch.CV_3));
    envelope.SetTime(ADSR_SEG_RELEASE, patch.GetAdcValue(patch.CV_4));

    /** Loop through the samples, and output the ADSR signal */
    for(size_t i = 0; i < size; i++)
    {
        uint16_t value = (envelope.Process(env_state) * 4095.0);
        /** We could use OUT_L, and OUT_R here as well, _but_ 
         *  these correspond to CV_OUT_1, and CV_OUT_2 so that
         *  would be a bit misleading */
        output[0][i] = value;
        output[1][i] = value;
    }
}

int main(void)
{
    /** Initialize the hardware */
    patch.Init();

    /** Initialize the button input to pin B7 (Button on the MicroPatch Eval board) */
    button.Init(patch.B7, 1000);

    /** Initialize the ADSR */
    envelope.Init(48000);

    /** Configure the DAC to generate audio-rate signals in a callback */
    patch.StartDac(EnvelopeCallback);

    while(1) {}
}
