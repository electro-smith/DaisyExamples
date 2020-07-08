#include "daisysp.h"
#include "daisy_patch.h"
#include <string>

using namespace daisy;
using namespace daisysp;

DaisyPatch patch;

int main(void)
{
    patch.Init(); // Initialize hardware (daisy seed, and patch)

    patch.StartAdc();
    patch.StartAudio(AudioCallback);
    while(1) 
    {
    }
}
