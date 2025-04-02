#include "daisysp.h"
#include "daisy_petal.h"
#include "terrarium.h"

using namespace daisy;
using namespace daisysp;
using namespace terrarium;

DaisyPetal petal;

Parameter param;

Led led1, led2;

void InitParam()
{ // Initialize param
    // Curves: LINEAR, EXPONENTIAL, LOGARITHMIC, CUBE
    param.Init(petal.knob[Terrarium::KNOB_1], 0.0, 1.0, Parameter::LINEAR);  // swell is knob 6
}

void InitLeds(void)
{
    // Initialize the leds - these are using LED objects
    led1.Init(petal.seed.GetPin(Terrarium::LED_1),false);
    led2.Init(petal.seed.GetPin(Terrarium::LED_2),false);
}

int main(void)
{
    petal.Init();

    InitParam();
    InitLeds();

    petal.StartAdc();

    bool ledOn = false;
    
    while (1)
    {
        petal.ProcessAnalogControls();
        petal.ProcessDigitalControls();

        float val = param.Process(); // swell_val = powf(swellParam.Process(),1/1.5);
        led1.Set(val > 0.5f);
        led1.Update();

        if(petal.switches[Terrarium::FOOTSWITCH_2].RisingEdge()) {
            ledOn = !ledOn;
        }

        led2.Set(ledOn);
        led2.Update();
    }
}

