#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"

#define KNOB_PIN 15

using namespace daisy;

static DaisySeed hw;

int main(void)
{
    bool ledstate = false;
    hw.Configure();
    hw.Init();
    dsy_gpio_pin knob_pin;
    AdcChannelConfig knob_cfg;
    knob_pin = hw.GetPin(KNOB_PIN);
    knob_cfg.InitSingle(knob_pin);
    hw.adc_handle.Init(&knob_cfg, 1, hw.adc_handle.OVS_32);
    hw.adc_handle.Start();
    float ms;
    while(1)
    {
        ms = 20 + (200 * hw.adc_handle.GetFloat(0)); 
        dsy_system_delay(ms);
        hw.SetLed(ledstate);
        ledstate = !ledstate;
    }
}
