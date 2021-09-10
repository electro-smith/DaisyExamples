#include "daisy_patch_sm.h"
#include "daisysp.h"

#define LEFT 0
#define RIGHT 1

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace daisysp;

/** Our hardware board class handles the interface to the actual DaisyPatchSM
 * hardware. */
DaisyPatchSM patch;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    /** Ways to passthru: */
    /*****************************************
     ** 1. Block based - standard CPP  *******
     *****************************************/
    std::copy(in[0], in[0] + size, out[0]);
    std::copy(in[1], in[1] + size, out[1]);
    /** Or with channel name macro macro */
    std::copy(in[LEFT], in[LEFT] + size, out[LEFT]);
    std::copy(in[RIGHT], in[RIGHT] + size, out[RIGHT]);

    /*****************************************
     ** 2. Loop based - c-style addressing ***
     *****************************************/

    for(size_t i = 0; i < size; i++)
    {
        out[LEFT][i]  = in[LEFT][i];
        out[RIGHT][i] = in[RIGHT][i];
    }

    /** or nested loop style */
    for(size_t i = 0; i < size; i++)
        for(int channel = 0; channel < 2; channel++)
            out[channel][i] = in[channel][i];

    /******************************************
     ** 3. Loop based - update Buffer types ***
     ******************************************/

    for(size_t i = 0; i < size; i++)
    {
        // A few ways we could do this.
        out.left.Get(i)  = in.left.Get(i);
        out.right.Get(i) = in.right.Get(i);
        // or
        out.left[i]  = in.left[i];
        out.right[i] = in.right[i];
        // or
        out.Left(i)  = in.Left(i);  /** Not 100% sure this is valid */
        out.Right(i) = in.Right(i); /** Not 100% sure this is valid */
    }
}
