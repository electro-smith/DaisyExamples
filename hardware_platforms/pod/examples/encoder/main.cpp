#include "daisy_pod.h"

static daisy_pod hw;
static uint8_t led_sel;
static int32_t inc;

static void callback(float *in, float *out, size_t size)
{
    // Debounce the encoder at a steady, fixed rate.
    //dsy_encoder_debounce(&hw.encoder);
    // check the increment -- 
    // it's important to do this at the same rate as the debouncing
    //inc = dsy_encoder_inc(&hw.encoder);
    hw.encoder.Debounce();
    inc = hw.encoder.Increment();
    // Change the selected LED based on the increment.
    if (inc > 0)
    {
        led_sel += 1;
        // Wrap around
        if (led_sel > LED_LAST - 1)
        {
            led_sel = 0;
        }
    }
    else if (inc < 0)
    {
        // Wrap around
        if (led_sel == 0)
        {
            led_sel = LED_LAST - 1;
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
    daisy_pod_init(&hw);
    led_sel = 0;
    inc = 0;
    // until we have fixed timer callbacks we'll use the audio callback
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, callback);
    dsy_audio_start(DSY_AUDIO_INTERNAL);
    while(1) 
    { 
        // Clear all LEDs
        for (uint8_t i = 0; i < LED_LAST; i++)
        {
            if (i == led_sel) 
            {
                dsy_gpio_write(&hw.leds[i], 0); 
            }
            else
            {
                dsy_gpio_write(&hw.leds[i], 1); 
            }
        }
    }
}
