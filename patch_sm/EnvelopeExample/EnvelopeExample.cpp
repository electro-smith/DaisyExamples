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
DaisyPatchSM hw;

/** Create a buffer for the CV output */
uint16_t DMA_BUFFER_MEM_SECTION cv_output_buffer[16];

/** Create ADSR envelope module */
Adsr envelope;

/** Create a Gate input and a button to control the ADSR activation */
GateIn gate;
Switch button;

/** Similar to the audio callback, you can generate audio rate CV signals out of the CV outputs. 
 *  These signals are 12-bit DC signals that range from 0-5V out of the Patch SM
*/
void EnvelopeCallback(uint16_t **output, size_t size) {

  /** Process the controls */
  hw.ProcessAnalogControls();
  button.Debounce();

  /** Set the input value of the ADSR */
  bool env_state;
  if (button.Pressed() || gate.State())
  {
    env_state = true;
  } else {
    env_state = false;
  }

  /** Assign four controls to the times and level of the envelope segments */
  envelope.SetTime(ADSR_SEG_ATTACK, 0.01 + (hw.GetAdcValue(0)));
  envelope.SetTime(ADSR_SEG_DECAY, 0.01 + (hw.GetAdcValue(1)));
  envelope.SetSustainLevel(hw.GetAdcValue(2));
  envelope.SetTime(ADSR_SEG_RELEASE, 0.01 + (hw.GetAdcValue(3)));

  /** Loop through the samples, and output the ADSR signal */
  for (size_t i = 0; i < size; i++) {
    uint16_t value = (envelope.Process(env_state) * 4095.0);
    output[0][i] = value;
  }
}

int main(void) {
  /** Initialize the hardware */
  hw.Init();

  /** Initialize the gate input to pin B10 (Gate 1 on the MicroPatch Eval board) */
  dsy_gpio_pin gate_pin = hw.GetPin(B10);
  gate.Init(&gate_pin);

  /** Initialize the button input to pin B7 (Button on the MicroPatch Eval board) */
  button.Init(hw.GetPin(B7), 48000 / 16);

  /** Initialize the ADSR */
  envelope.Init(48000);

  /** Configure the DAC to generate audio-rate signals in a callback */
  DacHandle::Config dac_config;
  dac_config.mode = DacHandle::Mode::DMA;
  dac_config.bitdepth = DacHandle::BitDepth::BITS_12; /**< Sets the output value to 0-4095 */
  dac_config.chn = DacHandle::Channel::ONE; /** CV Output 1 */
  dac_config.buff_state = DacHandle::BufferState::ENABLED;
  dac_config.target_samplerate = 48000;

  DacHandle dac;
  dac.Init(dac_config);

  dac.Start(cv_output_buffer, 16, EnvelopeCallback);

  while (1) {
  }
}
