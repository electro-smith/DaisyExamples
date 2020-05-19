#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;

int main(void)
{
    bool state1 = 0, state2 = 0;
    hw.Init();

    while(1)
    {
        hw.DebounceControls();

        // using button1 as momentary switch for turning on/off led1
        if(hw.button1.Pressed())
            state1 = 1;
        else
            state1 = 0;

        // using button2 as latching switch for toggling led2
        if(hw.button2.RisingEdge())
        {
            state2 = !state2;
        }
        
        hw.led1.Set(state1, state1, state1);
        hw.led2.Set(state2, state2, state2);
        hw.UpdateLeds();
    }
}
