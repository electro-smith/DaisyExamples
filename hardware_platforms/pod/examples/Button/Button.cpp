#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;
bool     on = 0;

static void callback(float *in, float *out, size_t size)
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
    if(hw.button2.RisingEdge())
    {
        if(on)
            hw.led2.Set(0, 0, 0);
        else
            hw.led2.Set(.5, .5, .5);

        on = !on;
    }
    hw.UpdateLeds();
    
    for(size_t i = 0; i < size; i += 2)
    {
        // Process audio
    }
}
int main(void)
{
    hw.Init();
    hw.StartAudio(callback);

    while(1)
    {
        //        hw.DebounceControls();
        //
        //        // using button1 as momentary switch for turning on/off led1
        //        if(hw.button1.Pressed())
        //        {
        //            hw.led1.Set(.5, .5, .5);
        //        }
        //        else
        //        {
        //            hw.led1.Set(0, 0, 0);
        //        }
        //
        //        // using button2 as latching switch for toggling led2
        //        if(hw.button2.RisingEdge())
        //        {
        //            if(on)
        //                hw.led2.Set(0, 0, 0);
        //            else
        //                hw.led2.Set(.5, .5, .5);
        //
        //            on = !on;
        //        }
        //        hw.UpdateLeds();
        //        //hw.DelayMs(10);
    }
}
