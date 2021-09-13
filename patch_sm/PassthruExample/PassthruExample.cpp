#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPatchSM patch;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        OUT_L[i] = IN_L[i];
        OUT_R[i] = IN_R[i];
    }
}

int main(void)
{
    /** Initialize the Hardware */
    patch.Init();
    /** Set samplerate to 96kHz. Default is 48kHz */
    patch.SetAudioSampleRate(96000);
    /** Set the number of samples to handle per callback
     *  This defaults to 16 */
    patch.SetAudioBlockSize(96);
    /** Start the callback */
    patch.StartAudio(AudioCallback);
    for(;;) {}
}
