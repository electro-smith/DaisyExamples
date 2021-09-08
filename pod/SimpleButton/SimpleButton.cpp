#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;

int main(void)
{
    bool brightness1, brightness2;
    brightness1 = false;
    brightness2 = false;
    hw.Init();

    while(1)
    {
        hw.ProcessDigitalControls();

        // using button1 as momentary switch for turning on/off led1
        brightness1 = hw.button1.Pressed();

        // using button2 as latching switch for toggling led2
        if(hw.button2.RisingEdge())
            brightness2 = !brightness2;

        // assign brightness levels to each led (R, G, B)
        hw.led1.Set(brightness1, brightness1, brightness1);
        hw.led2.Set(brightness2, brightness2, brightness2);
        hw.UpdateLeds();
    }
}
