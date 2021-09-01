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

ReverbSc reverb;

/** Callback for processing and synthesizing audio
 *
 *  The audio buffers are arranged as arrays of samples for each channel.
 *  For example, to access the left input you would use:
 *    in[0][n]
 *  where n is the specific sample.
 *  There are "size" samples in each array.
 *
 *  The default size is very small (just 4 samples per channel). This means the
 * callback is being called at 16kHz.
 *
 *  This size is acceptable for many applications, and provides an extremely low
 * latency from input to output. However, you can change this size by calling
 * hw.SetAudioBlockSize(desired_size). When running complex DSP it can be more
 * efficient to do the processing on larger chunks at a time.
 *
 */
void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  /** First we'll tell the hardware to process the 8 analog inputs */
  hw.ProcessAnalogControls();

  /** The first control will set the reverb time */
  float rev_time = 0.3 + (0.67 * hw.GetAdcValue(0));
  reverb.SetFeedback(rev_time);

  /** The second control will be the dampening frequency of the reverb
   *  There are several ways to get a good feeling knob for this, but
   *  surprisingly MIDI note numbers provide an easy range.
   *
   *  The range from Note 72 to Note 127 will make the knob go from
   *  523Hz to 12543Hz
   */
  float damping = mtof(72.0 + hw.GetAdcValue(1) * 55.0);
  reverb.SetLpFreq(damping);

  /** There are a number of ways to control the blend between the dry signal and
   * the reverb siganl. In this example we'll give separate controls over the 
   * dry amount and the effects send amount. */

  /** The third control will be the dry level going to the output */
  float inlevel = hw.GetAdcValue(2);

  /** The fourth controll will control how much of the input is sent to the
   * reverb */
  float sendamt = hw.GetAdcValue(3);


  /** This loop will allow us to process the individual samples of audio */
  for (size_t i = 0; i < size; i++) {
    /** Let's scale the input for the two destinations we want to send it to using multiplication. */
    float dryl = in[0][i] * inlevel;
    float dryr = in[1][i] * inlevel;
    float sendl = in[0][i] * sendamt;
    float sendr = in[1][i] * sendamt;
    float wetl, wetr;
    /** Process the send signal through the reverb */
    reverb.Process(sendl, sendr, &wetl, &wetr);

    /** Add the dry and the wet together, and assign those to the output */
    out[0][i] = dryl + wetl;
    out[1][i] = dryr + wetr;

  }
}

int main(void) {
  /** Initialize the hardware */
  hw.Init();

  /** Initialize the reverb module */
  reverb.Init(hw.AudioSampleRate());

  /** Start Processing the audio */
  hw.StartAudio(AudioCallback);
  while (1) {
  }
}
