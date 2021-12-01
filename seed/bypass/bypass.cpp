#include <string.h>
#include "daisy_seed.h"

using namespace daisy;

DaisySeed seed;

static void Callback(AudioHandle::InterleavingInputBuffer  in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t                                size)
{
    memcpy(out, in, size * sizeof(float));
}

int main(void)
{
    seed.Configure();
    seed.Init();
    seed.SetAudioBlockSize(4);
    seed.StartAudio(Callback);
    while(1) {}
}
