#include <string.h>
#include "daisy_seed.h"

using namespace daisy;

DaisySeed hw;

static void Callback(AudioHandle::InterleavingInputBuffer  in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t                                size)
{
    memcpy(out, in, size * sizeof(float));
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.StartAudio(Callback);
    while(1) {}
}
