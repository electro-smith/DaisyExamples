#include "daisysp.h"
#include "daisy_patch.h"

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;


int main(void)
{
    float samplerate;
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1) {} // loop forever
}
