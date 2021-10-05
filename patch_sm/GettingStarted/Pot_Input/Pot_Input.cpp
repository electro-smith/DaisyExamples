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

/** Oscillator object */
Oscillator   osc;

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
    /** Update all cv inputs */
    hw.ProcessAllControls();

    /** Get CV_1 Input (0, 1) */
    float pitch = hw.GetAdcValue(0);

    /** Scale to hz (0, 1) -> (0, 1000) */
    pitch       = pitch * 1000.f;

    /** Set osc frequency to pitch variable value */
    osc.SetFreq(pitch);

    /** Loop over the whole audio buffer */
    for(size_t i = 0; i < size; i++)
    {

        /** Get the next sample from the oscillator */
        float signal = osc.Process();

        /** Set the left output to the signal value */
        OUT_L[i] = signal;
    
        /** Set the right output to the signal value */
        OUT_R[i] = signal;
    }
}

int main(void)
{
    /** Initialize the patch_sm hardware object */
    hw.Init();
    
    /** Audio engine speed in samples / second */
    float sample_rate = hw.AudioSampleRate();

    /** Initialize the oscillator object */
    osc.Init(sample_rate);

    /** Start the audio callback */
    hw.StartAudio(AudioCallback);

    /** Loop forever */
    while(1) {}
}