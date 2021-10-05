#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Daisy patch sm object */
DaisyPatchSM hw; 

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
    for(size_t i = 0; i < size; i++)
    {
        /** set the left output to the current left input */
        OUT_L[i] = IN_L[i];

        /** set the right output to the current right input */
        OUT_R[i] = IN_R[i];
    }
}

int main(void)
{
    /** Initialize the patch_sm object */
    hw.Init();

    /** Set the samplerate to 16kHz */
    hw.SetAudioSampleRate(16000.f);

    /** Set the blocksize to 4 samples */
    hw.SetAudioBlockSize(4);

    /** Start the audio callback */
    hw.StartAudio(AudioCallback);

    /** Loop forever */
    while(1) {}
}
