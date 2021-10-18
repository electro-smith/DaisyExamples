#include "daisy_patch_sm.h"
#include "daisysp.h"
#include "Looper.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

/** Maximum loop length in samples. 
 * At a sample rate of 48khz, 48000 * 10 = 10 second loop length max 
*/
#define MAX_LOOP_LEN 48000 * 10

/** Daisy Patch SM hardware object */
DaisyPatchSM patch;

/** Looper object to handle looping for us */
static Looper<float, MAX_LOOP_LEN> DSY_SDRAM_BSS looper;

/** Switch object to handle the switch we'll control the looper with */
Switch sw;

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
 * patch.SetAudioBlockSize(desired_size). When running complex DSP it can be more
 * efficient to do the processing on larger chunks at a time.
 *
 */
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    /** Make sure the switch triggers cleanly */
    sw.Debounce();

    /** If the switch is pressed... */
    if(sw.RisingEdge())
    {
        /** Toggle looping (start a new loop or stop the current one) 
        * This will automatically set your loop length
        */
        looper.ToggleLoop();
    }

    /** For every sample in the current audio buffer... */
    for(size_t i = 0; i < size; i++)
    {
        /** Mix the two inputs into a single signal */
        float in_mixed = (IN_L[i] + IN_R[i]) * .5f;

        /** Write the current input, and get the current looper output 
        * Process will only actually write the input if we're recording a new loop
        */
        float loop_sig = looper.Process(in_mixed);

        /** Mix the looper signal and the input signals */
        float output_sig = (loop_sig + in_mixed) * .5f;

        /** Send that signal to both audio outs */
        OUT_L[i] = OUT_R[i] = output_sig;
    }
}

int main(void)
{
    /** Initialize the patch_sm hardware */
    patch.Init();

    /** Get the rate at which we'll call the audio callback function
    * At 48khz with a blocksize of 48 samples this is 48000 / 48 = 1000khz
    */
    int   cbrate     = patch.AudioCallbackRate();
 
    /** Get the rate at which we'll output samples */
    float samplerate = patch.AudioSampleRate();

    /* initialize the switch
	 - We'll read the switch on pin B7
	 - The switch will be read at the callback rate
	*/
    sw.Init(patch.B7, cbrate);

    /** Initilize the looper. It'll run at the samplerate */
    looper.Init(samplerate);

    /** Start the audio callback */
    patch.StartAudio(AudioCallback);

    /** Loop forever */
    while(1) {}
}
