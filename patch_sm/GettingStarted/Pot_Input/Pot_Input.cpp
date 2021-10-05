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
Oscillator   osc; // Oscillator object

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
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls(); // update all cv inputs

    float pitch = hw.GetAdcValue(0); // Get CV_1 Input (0, 1)
    pitch       = pitch * 1000.f;    // scale to hz (0, 1) -> (0, 1000)
    osc.SetFreq(pitch); // set osc frequency to pitch variable value

    for(size_t i = 0; i < size; i++) // loop over the whole audio buffer
    {
        float signal = osc.Process(); // get the next sample from the oscillator

        OUT_L[i] = signal; // set the left output to the signal value
        OUT_R[i] = signal; // set the right output to the signal value
    }
}

int main(void)
{
    hw.Init(); // initialize the patch_sm hardware object
    float sample_rate
        = hw.AudioSampleRate(); // Audio engine speed in samples / second

    osc.Init(sample_rate); // initialize the oscillator object

    hw.StartAudio(AudioCallback); // start the audio callback

    while(1) {} // loop forever
}