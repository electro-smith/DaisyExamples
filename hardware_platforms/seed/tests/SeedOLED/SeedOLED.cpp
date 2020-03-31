#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "hid_oled_display.h"

using namespace daisy;

static DaisySeed hw;
OledDisplay         display;

bool    state;
uint8_t xcnt, ycnt;
uint8_t message_idx;

int     main(void)
{
	hw.Init();
	display.Init();
	state = true;
	xcnt = ycnt = 0;
	message_idx = 0;
	char strbuff[128];
	while (1)
	{
		dsy_system_delay(500);
		switch (message_idx)
		{
		case 0: sprintf(strbuff, "Testing. . ."); break;
		case 1: sprintf(strbuff, "Daisy. . ."); break;
		case 2: sprintf(strbuff, "1. . ."); break;
		case 3: sprintf(strbuff, "2. . ."); break;
		case 4: sprintf(strbuff, "3. . ."); break;
		default: break;
		}
		message_idx = (message_idx + 1) % 5;
		display.Fill(false);
		display.SetCursor(0, 0);
		display.WriteString(strbuff, Font_11x18, true);
		display.Update();
	}
}
