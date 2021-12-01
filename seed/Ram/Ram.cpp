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
static uint32_t  start, end, dur;

void bad_callback(AudioHandle::InterleavingInputBuffer  in,
                  AudioHandle::InterleavingOutputBuffer out,
                  size_t                                size)
{
    start = System::GetTick();
    //	int32_t* ram = (int32_t*)0x20000000; // DTCM
    //	int32_t* ram = (int32_t*)0x38000000; // D3 RAM
    int32_t *ram = (int32_t *)0xC0000000; // SDRAM
    memcpy(ram, in, sizeof(int32_t) * size);
    for(int i = 0; i < 300; i++)
    {
        memcpy((i + 1) * size + ram, ram, sizeof(int32_t) * size);
    }
    end = System::GetTick();
    dur = (end - start) / 200; // us
    memcpy(out, in, sizeof(float) * size);
}

void passthru(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    memcpy(out, in, sizeof(float) * size);
}

int main(void)
{
    // Initialize Hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
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
    hw.StartAudio(bad_callback);
    while(1) {}
}
