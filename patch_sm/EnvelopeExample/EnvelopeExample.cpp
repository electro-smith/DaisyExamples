#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
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
    float knob_attack = patch.GetAdcValue(CV_1);
    envelope.SetAttackTime(knob_attack);

    float knob_decay = patch.GetAdcValue(CV_2);
    envelope.SetDecayTime(knob_decay);

    float knob_sustain = patch.GetAdcValue(CV_3);
    envelope.SetSustainLevel(knob_sustain);

    float knob_release = patch.GetAdcValue(CV_4);
    envelope.SetReleaseTime(knob_release);

    for(size_t i = 0; i < size; i++)
    {
        // convert to 12-bit integer (0-4095)
        uint16_t value = (envelope.Process(env_state) * 4095.0);
        output[0][i]   = value; /**< To CV OUT 1 - Jack */
        output[1][i]   = value; /**< To CV OUT 2 - LED */
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
