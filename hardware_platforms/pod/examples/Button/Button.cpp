#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;

int main(void)
{
    hw.Init();
    int  flag = 0;
    bool on   = 0;

    while(1)
    {
        hw.DebounceControls();

        // using button1 as momentary switch for turning on/off led1
        if(hw.button1.Pressed())
        {
            hw.led1.Set(.5, .5, .5);
        }
        else
        {
            hw.led1.Set(0, 0, 0);
        }

        // using button2 as latching switch for toggling led2
        if(hw.button2.Pressed() && flag == 0)
        {
            if(on)
                hw.led2.Set(0, 0, 0);
            else
                hw.led2.Set(.5, .5, .5);

            flag = 1;
            on   = !on;
        }
        else
        {
            flag = 0;
        }

        hw.UpdateLeds();
    }
}
