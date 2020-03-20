#include "daisy_pod.h"

using namespace daisy;

static DaisyPod hw;
static uint8_t led_sel;
static int32_t inc;

static void callback(float *in, float *out, size_t size)
{
    // Debounce the encoder at a steady, fixed rate.
    hw.encoder.Debounce();
    inc = hw.encoder.Increment();
    // Change the selected LED based on the increment.
    if (inc > 0)
    {
        led_sel += 1;
        // Wrap around
        if (led_sel > DaisyPod::LED_LAST - 1)
        {
            led_sel = 0;
        }
    }
    else if (inc < 0)
    {
        // Wrap around
        if (led_sel == 0)
        {
            led_sel = DaisyPod::LED_LAST - 1;
        }
        else
        {
            led_sel -= 1;
        }
    }
    if (hw.encoder.RisingEdge())
    {
        led_sel = 0;
    }
}

int main(void)
{
    hw.Init();
    led_sel = 0;
    inc = 0;
    // until we have fixed timer callbacks we'll use the audio callback
    hw.StartAudio(callback);
    while(1) 
    { 
        hw.ClearLeds();
        hw.SetLed(static_cast<DaisyPod::Led>(led_sel), 1);
    }
}
