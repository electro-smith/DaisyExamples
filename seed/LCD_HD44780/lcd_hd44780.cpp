#include "daisy_seed.h"

using namespace daisy;


#define PIN_LCD_RS 15 // LCD: pin 8
#define PIN_LCD_EN 16 // LCD: pin 9
#define PIN_LCD_D4 20 // LCD: D4
#define PIN_LCD_D5 19 // LCD: D5
#define PIN_LCD_D6 18 // LCD: D6
#define PIN_LCD_D7 17 // LCD: D7


DaisySeed hardware;


int main(void)
{
    int i = 0;

    hardware.Configure();
    hardware.Init();

    LcdHD44780 lcd;

    LcdHD44780::Config lcd_config;

    lcd_config.cursor_on    = true;
    lcd_config.cursor_blink = false;
    lcd_config.rs           = hardware.GetPin(PIN_LCD_RS);
    lcd_config.en           = hardware.GetPin(PIN_LCD_EN);
    lcd_config.d4           = hardware.GetPin(PIN_LCD_D4);
    lcd_config.d5           = hardware.GetPin(PIN_LCD_D5);
    lcd_config.d6           = hardware.GetPin(PIN_LCD_D6);
    lcd_config.d7           = hardware.GetPin(PIN_LCD_D7);

    lcd.Init(lcd_config);

    while(1)
    {
        hardware.DelayMs(2000);

        lcd.Clear();

        switch(i % 2)
        {
            case 0:
                lcd.SetCursor(0, 0);
                lcd.Print("Test 1");
                break;
            case 1:
                lcd.SetCursor(0, 10);
                lcd.Print("Test 2");
                break;
            default: break;
        }

        lcd.SetCursor(1, 0);
        lcd.PrintInt(i);

        i++;
    }
}
