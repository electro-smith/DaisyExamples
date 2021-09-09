#include "daisy_patch_sm.h"

using namespace daisy;

DaisyPatchSM patch;

int main(void)
{
    patch.Init();

    /** If we want to keep this, we can move this into the System::Init.. */
    Random::Init();

    float output_voltage = 0.f;
    while(1)
    {
        /** Delay half a second */
        System::Delay(500);

        /** C-Style - Generate a random voltage between 0.0 and 5.0 */
        //output_voltage = rand() / (RAND_MAX / 5);

        /** OR use the internal TRNG */
        output_voltage = Random::GetFloat(0.0, 5.0);

        /** channel 0 means both CV Outputs */
        patch.WriteCvOut(0, output_voltage);
    }
}
