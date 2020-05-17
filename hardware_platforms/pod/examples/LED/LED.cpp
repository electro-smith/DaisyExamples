#include "daisy_pod.h"

using namespace daisy;

DaisyPod hw;
parameter knob1, knob2;

int main(void)
{
    hw.Init();
    float     r = 0, g = 0, b = 0;
    knob1.init(hw.knob1, 0, 1, parameter::LINEAR);
    knob2.init(hw.knob2, 0, 1, parameter::LINEAR);

    hw.StartAdc();

    while(1)
    {
        r = knob1.process();
        g = knob2.process();

        hw.led1.Set(r, g, b);
        
        hw.UpdateLeds();
    }
}
