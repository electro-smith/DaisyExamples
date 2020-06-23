#include <stdio.h>
#include <string.h>
#include "daisy_petal.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

void UpdateUsb();
void UpdateLeds();

// Declare a local daisy_petal for hardware access
DaisyPetal hw;

// This runs at a fixed rate to prepare audio samples
void callback(float *in, float *out, size_t size)
{
    hw.DebounceControls();
    hw.UpdateAnalogControls();
    // Audio Rate Loop
    for(size_t i = 0; i < size; i += 2)
    {
        out[i]     = in[i];
        out[i + 1] = in[i + 1];
    }
}

int main(void)
{
    uint32_t led_period, usb_period, now;
    uint32_t last_led_update, last_usb_update;

    // LED Freq = 60Hz
    // USB Freq= 10Hz
    led_period      = 5;
    usb_period      = 100;
    last_led_update = last_usb_update = now = dsy_system_getnow();

    hw.Init();

    // Init USB as VCOM for sending Serial Data
    hw.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);

    hw.StartAdc();
    hw.StartAudio(callback);


    while(1)
    {
        // Do Stuff InfInitely Here
        now = dsy_system_getnow();

        // Update LEDs (Vegas Mode)
        if(now - last_led_update > led_period)
        {
            last_led_update = now;
            UpdateLeds();
        }

        if(now - last_usb_update > usb_period)
        {
            last_usb_update = now;
            UpdateUsb();
        }
    }
}

#define KNOB_ID "KNOB_"
#define SW_ID "SWITCH_"
#define ENC_ID "ENCODER_"
// Send Control Data
// Format:
// { ctrl_id, ctrl_data }, {ctrl_id, ctrl_data}, etc.
void UpdateUsb()
{
    char catbuff[4095];
    char buff[128];
    memset(catbuff, 0, 4095);
    memset(buff, 0, 128);
    // Get Ready to send:
    // Knobs
    for(size_t i = 0; i < hw.KNOB_LAST; i++)
    {
        sprintf(buff,
                "{%s%d, %.3f}",
                KNOB_ID,
                i + 1,
                hw.GetKnobValue(static_cast<DaisyPetal::Knob>(i)));
        strcat(catbuff, buff);
    }
    // Expression
    sprintf(buff, "{EXP_0, %.3f}", hw.GetExpression());
    strcat(catbuff, buff);
    // Switches
    for(size_t i = 0; i < hw.SW_LAST; i++)
    {
        sprintf(buff, "{%s%d, %d}", SW_ID, i + 1, hw.switches[i].Pressed());
        strcat(catbuff, buff);
    }
    // Enc
    sprintf(buff, "{ENC_1, %d}", hw.encoder.Pressed());
    strcat(catbuff, buff);
    // EndTx
    sprintf(buff, "\r\n");
    strcat(catbuff, buff);
    hw.seed.usb_handle.TransmitInternal((uint8_t *)catbuff, strlen(catbuff));
}

void UpdateLeds()
{
    uint32_t now;
    now = dsy_system_getnow();
    hw.ClearLeds();
    // Use now as a source for time so we don't have to use any global vars
    // First gradually pulse all 4 Footswitch LEDs
    for(size_t i = 0; i < hw.FOOTSWITCH_LED_LAST; i++)
    {
        size_t total, base;
        total        = 511;
        base         = total / hw.FOOTSWITCH_LED_LAST;
        float bright = (float)((now + (i * base)) & total) / (float)total;
        hw.SetFootswitchLed(static_cast<DaisyPetal::FootswitchLed>(i), bright);
    }
    // And now the ring
    for(size_t i = 0; i < hw.RING_LED_LAST; i++)
    {
        float    rb, gb, bb;
        uint32_t total, base;
        uint32_t col;
        col = (now >> 10) % 6;
        //        total = 8191;
        //        base  = total / (hw.RING_LED_LAST);
        total        = 1023;
        base         = total / hw.RING_LED_LAST;
        float bright = (float)((now + (i * base)) & total) / (float)total;
        bright       = 1.0f - bright;
        switch(col)
        {
            case 0:
                rb = bright;
                gb = 0.0f;
                bb = 0.0f;
                break;
            case 1:
                rb = 0.6f * bright;
                gb = 0.0f;
                bb = 0.7f * bright;
                break;
            case 2:
                rb = 0.0f;
                gb = bright;
                bb = 0.0f;
                break;
            case 3:
                rb = 0.0f;
                gb = 0.0f;
                bb = bright;
                break;
            case 4:
                rb = 0.75f * bright;
                gb = 0.75f * bright;
                bb = 0.0f;
                break;
            case 5:
                rb = 0.0f;
                bb = 0.85f * bright;
                gb = 0.85f * bright;
                break;

            default: rb = gb = bb = bright; break;
        }

        hw.SetRingLed(static_cast<DaisyPetal::RingLed>(i), rb, gb, bb);
    }
    hw.UpdateLeds();
}
