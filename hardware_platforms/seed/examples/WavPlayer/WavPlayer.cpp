// # WavPlayer
// ## Description
// 
// Play .wav file from the SD Card.
//
// Behavior:
// 
// - Encoder Will select the file
// - File name will be indicated on an OLED.
// - A push button will be used to toggle the playback.
// 
// Hardware Details connected to Seed:
// Essentially the Pod, but with some stuff wired out for the OLED.
//
// - Oled connected via SPI, DC and RST are connected via GPIOB8, and GPIOB9 respectively.
// - SDMMC connected to all 6 SDMMC Pins on Seed.
// - Push button wired to GPIO 29 
// - Encoder wired with:
//		- A: GPIO 27
//		- B: GPIO 26
//		- Click: GPIO 1
//
#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "fatfs.h"

using namespace daisy;

static daisy_handle hw;
OledDisplay display;
SdmmcHandler sdcard;

void AudioCallback(float *in, float *out, size_t size)
{
	for (size_t i = 0; i < size; i += 2)
	{
		
	}
}

int main(void)
{
	// Init hardware
	size_t blocksize = 12;
	daisy_seed_init(&hw);
	display.Init();
	sdcard.Init();
	// Let's get the first wav file we find!
	// Init Audio
	dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, blocksize);
	dsy_audio_set_callback(DSY_AUDIO_INTERNAL, AudioCallback);
	dsy_audio_start(DSY_AUDIO_INTERNAL);
	// Otherwise the onboard LED will begin to blink.
	for(;  ;)
	{
	}
}
