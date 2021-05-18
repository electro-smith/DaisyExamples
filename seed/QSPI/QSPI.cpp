#include <string.h>
#include "daisy_seed.h"
#include "daisysp.h"
#include "test_const.h"
#include <math.h>

// 1MB test
#define TEST_BUFF_SIZE (1024 * 8)

using namespace daisy;
using namespace daisysp;

static uint32_t DSY_QSPI_BSS test_buff[TEST_BUFF_SIZE];
static uint32_t __attribute__((section(".dtcmram_bss")))
outbuff[TEST_BUFF_SIZE];
static uint32_t axi_outbuff[TEST_BUFF_SIZE];
uint32_t        inbuff[TEST_BUFF_SIZE];

static DaisySeed hw;
static uint32_t  start, end;
static uint32_t  dur_erase, dur_write_4k, dur_write_4m, dur_read_qspi,
    dur_read_flash, dur_read_axi;


int main(void)
{
    // Initialize Hardware
    hw.Configure();
    hw.Init();
    //	for(uint32_t i = 0; i < TEST_BUFF_SIZE; i++)
    //	{
    //		test_buff[i] = i;
    //	}
    //	for(uint32_t i = 0; i < TEST_BUFF_SIZE; i++)
    //	{
    //		if(test_buff[i] != i)
    //		{
    //			asm("bkpt 255");
    //		}
    //	}
    uint32_t res;
    //uint32_t val = 0;
    //uint32_t small_buff[1024];
    //uint32_t writesize = 1024 * sizeof(test_buff[0]);
    dur_write_4m = 0;
    // Get into write mode.
    hw.qspi_handle.mode = DSY_QSPI_MODE_INDIRECT_POLLING;
    dsy_qspi_init(&hw.qspi_handle);
    // Erase
    uint32_t base = 0x90000000;
    for(uint32_t i = 0; i < TEST_BUFF_SIZE; i++)
    {
        inbuff[i] = i;
    }
    start = System::GetTick();
    dsy_qspi_erase(base, base + (TEST_BUFF_SIZE * sizeof(test_buff[0])));
    end       = System::GetTick();
    dur_erase = (end - start) / 200;
    start     = System::GetTick();
    res       = dsy_qspi_write(
        base, TEST_BUFF_SIZE * sizeof(test_buff[0]), (uint8_t*)inbuff);
    end          = System::GetTick();
    dur_write_4k = (end - start) / 200;

    hw.qspi_handle.mode = DSY_QSPI_MODE_DSY_MEMORY_MAPPED;
    dsy_qspi_init(&hw.qspi_handle);

    start = System::GetTick();
    memcpy(outbuff, test_buff, sizeof(test_buff[0]) * TEST_BUFF_SIZE);
    end           = System::GetTick();
    dur_read_qspi = (end - start) / 200;

    start = System::GetTick();
    memcpy(
        outbuff, test_flash_buff, sizeof(test_flash_buff[0]) * TEST_BUFF_SIZE);
    end            = System::GetTick();
    dur_read_flash = (end - start) / 200;

    start = System::GetTick();
    memcpy(axi_outbuff,
           test_flash_buff,
           sizeof(test_flash_buff[0]) * TEST_BUFF_SIZE);
    end          = System::GetTick();
    dur_read_axi = (end - start) / 200;

    if(res)
        asm("bkpt 255");

    // READ test
    // Write 4kb chunks at a time
    //	for(uint32_t i = 0; i < 1024; i++)
    //	{
    //		for(uint32_t j = 0; j < 1024; j++)
    //		{
    //			small_buff[j] = val;
    //			val++;
    //		}
    //		start = System::GetTick();
    //		res = dsy_qspi_write(base + (i * writesize), writesize, (uint8_t*)small_buff);
    //		end = System::GetTick();
    //		dur_write_4k = (end - start) / 200;
    //		dur_write_4m += dur_write_4k;
    //	}
    bool ledstate;
    ledstate = true;
    while(1)
    {
        System::Delay(250);
        hw.SetLed(ledstate);
        ledstate = !ledstate;
    }
}
