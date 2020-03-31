#include <string.h>
#include "daisy_seed.h"
#include "daisysp.h"
#include <math.h>

// 1MB test
#define TEST_BUFF_SIZE (1024 * 1024)

using namespace daisy;
using namespace daisysp;

static uint32_t DSY_SDRAM_BSS test_buff[TEST_BUFF_SIZE];

static DaisySeed hw;
static uint32_t start, end, dur;

void bad_callback(float *in, float *out, size_t size)
{
	start = dsy_tim_get_tick();
//	int32_t* ram = (int32_t*)0x20000000; // DTCM
//	int32_t* ram = (int32_t*)0x38000000; // D3 RAM
	int32_t *ram = (int32_t *)0xC0000000; // SDRAM
	memcpy(ram, in, sizeof(int32_t) * size);
	for(int i = 0; i < 300; i++)
	{
		memcpy((i + 1) * size + ram, ram, sizeof(int32_t) * size);
	}
	end = dsy_tim_get_tick();
	dur = (end - start) / 200; // us
	memcpy(out, in, sizeof(float) * size);
}

void passthru(float *in, float *out, size_t size)
{
	memcpy(out, in, sizeof(float) * size);
}

int main(void)
{
	// Initialize Hardware
	hw.Init();
	for(uint32_t i = 0; i < TEST_BUFF_SIZE; i++) 
	{
		test_buff[i] = i;
	}
	for(uint32_t i = 0; i < TEST_BUFF_SIZE; i++) 
	{
		if(test_buff[i] != i) 
		{
			asm("bkpt 255");
		}
	}
	dsy_tim_start();
	dsy_audio_set_callback(DSY_AUDIO_INTERNAL, bad_callback);
	dsy_audio_start(DSY_AUDIO_INTERNAL);
	while(1) {}
}

