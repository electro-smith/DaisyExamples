#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

static DaisySeed  hw;
static WhiteNoise nse;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    float sig;

    for(size_t i = 0; i < size; i += 2)
    {
        sig = nse.Process();

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    // initialize seed hardware and WhiteNoise daisysp module
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    nse.Init();

    // start callback
    hw.StartAudio(AudioCallback);

    while(1) {}
}
