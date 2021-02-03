#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"

using namespace daisy;

DaisySeed   hw;
OledDisplay display;

int main(void)
{
    uint8_t      message_idx;
    dsy_gpio_pin oled_pins[OledDisplay::NUM_PINS];
    hw.Configure();
    hw.Init();
    oled_pins[OledDisplay::DATA_COMMAND] = hw.GetPin(10);
    oled_pins[OledDisplay::RESET]        = hw.GetPin(31);
    display.Init(oled_pins);
    message_idx = 0;
    char strbuff[128];
    while(1)
    {
        System::Delay(500);
        switch(message_idx)
        {
            case 0: sprintf(strbuff, "Testing. . ."); break;
            case 1: sprintf(strbuff, "Daisy. . ."); break;
            case 2: sprintf(strbuff, "1. . ."); break;
            case 3: sprintf(strbuff, "2. . ."); break;
            case 4: sprintf(strbuff, "3. . ."); break;
            default: break;
        }
        message_idx = (message_idx + 1) % 5;
        display.Fill(true);
        display.SetCursor(0, 0);
        display.WriteString(strbuff, Font_11x18, false);
        display.Update();
    }
}
