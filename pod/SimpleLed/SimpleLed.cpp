#include "daisy_pod.h"

using namespace daisy;

DaisyPod  hw;
Parameter p_knob1, p_knob2;

int main(void)
{
    hw.Init();
    float r = 0, g = 0, b = 0;
    p_knob1.Init(hw.knob1, 0, 1, Parameter::LINEAR);
    p_knob2.Init(hw.knob2, 0, 1, Parameter::LINEAR);

    hw.StartAdc();

    while(1)
    {
        r = p_knob1.Process();
        g = p_knob2.Process();

        hw.led1.Set(r, g, b);

        hw.UpdateLeds();
    }
}
