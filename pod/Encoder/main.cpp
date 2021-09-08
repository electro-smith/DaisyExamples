#include "daisy_pod.h"

using namespace daisy;

static DaisyPod hw;
static uint8_t  led_sel;
static int32_t  inc;

#define NUM_COLORS 5

Color my_colors[5];

static void callback(AudioHandle::InterleavingInputBuffer  in,
                     AudioHandle::InterleavingOutputBuffer out,
                     size_t                                size)
{
    // Debounce the Encoder at a steady, fixed rate.
    hw.encoder.Debounce();
    inc = hw.encoder.Increment();
    // Change the selected LED based on the increment.
    if(inc > 0)
    {
        led_sel += 1;
        // Wrap around
        if(led_sel > NUM_COLORS - 1)
        {
            led_sel = 0;
        }
    }
    else if(inc < 0)
    {
        // Wrap around
        if(led_sel == 0)
        {
            led_sel = NUM_COLORS - 1;
        }
        else
        {
            led_sel -= 1;
        }
    }
    if(hw.encoder.RisingEdge())
    {
        led_sel = 4;
    }
    hw.ClearLeds();
    hw.led1.SetColor(my_colors[led_sel]);
    hw.led2.SetColor(my_colors[led_sel]);
    hw.UpdateLeds();
}

int main(void)
{
    hw.Init();
    led_sel = 0;
    inc     = 0;
    my_colors[0].Init(Color::PresetColor::RED);
    my_colors[1].Init(Color::PresetColor::GREEN);
    my_colors[2].Init(Color::PresetColor::BLUE);
    my_colors[3].Init(Color::PresetColor::WHITE);
    my_colors[4].Init(Color::PresetColor::OFF);
    // until we have fixed timer callbacks we'll use the audio callback
    hw.StartAudio(callback);
    while(1)
    {
        //hw.ClearLeds();
        //hw.SetLed(static_cast<DaisyPod::Led>(led_sel), 1);
    }
}
