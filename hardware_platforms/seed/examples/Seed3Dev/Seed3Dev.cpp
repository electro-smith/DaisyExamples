// # Seed3Dev
// ## Description
// Tests the Seed Hardware
//
// - IF TEST_RAM is defined: 4MB buffer is written to and read from the RAM.
// - IF TEST_QSPI is defined:  RAM buff is then written to QSPI, and then read back.
// - Pulses each pin 4 times, for 20ms high, 20ms low.
//
#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "daisysp.h"
using namespace daisy;

enum
{
    TEST_START,
    TEST_RAM_PASS,
    TEST_QSPI_PASS,
    TEST_GPIO_AUDIO,
};

char  startmessage[64] = "Daisy Test Beginning\r\n";
char  rammessage[64] = "SDRAM Test PASS.\r\n";
char  qspimessage[64] = "QSPI Test PASS\r\n";
char  restmessage[64] = "TESTING 32x GPIO and Audio I/O\r\n";
char *sbuffs[4] = {startmessage, rammessage, qspimessage,restmessage};

daisysp::Oscillator DSY_SDRAM_BSS osc;

//#define TEST_RAM

// TEST RAM is required for testing the QSPI.
//#define TEST_QSPI

dsy_gpio outs[32];

// 4MB RAM Test
#define BIG_BUFF_SIZE (1024 * 1024)
uint32_t DSY_SDRAM_BSS huge_friggin_ram_buff[BIG_BUFF_SIZE];

// 4 MB QSPI Test
uint32_t DSY_QSPI_BSS qspi_buff[BIG_BUFF_SIZE];

//daisy_handle hw;
DaisySeed hw;

uint32_t phasor = 0;
float    sig;

void AudioTest(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        //        phasor = (phasor + 1) % 256;
        //        sig    = ((phasor / 256.0f) * 2.0f) - 1.0f;
        //        sig *= 0.25f;

        //        out[i]     = in[i];
        //        out[i + 1] = in[i + 1];
        sig        = osc.Process();
        out[i]     = sig;
        out[i + 1] = in[i + 1];
    }
}


int main(void)
{
    hw.Configure();
    hw.audio_handle.sai->device[DSY_AUDIO_INTERNAL]   = DSY_AUDIO_NONE;
    hw.audio_handle.sai->device[DSY_AUDIO_EXTERNAL]   = DSY_AUDIO_NONE;
    hw.audio_handle.sai->bitdepth[DSY_AUDIO_INTERNAL] = DSY_AUDIO_BITDEPTH_24;
    hw.Init();
    dsy_audio_init(&hw.audio_handle);
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, 256);
	// Init Synthesis
    osc.Init(DSY_AUDIO_SAMPLE_RATE);
    osc.SetAmp(0.75f);
    osc.SetFreq(100.0f);
	// 4 second time to boot just to have to time to open a COM terminal.
    dsy_system_delay(4000);

	// TODO: move this to audio.h
    // Update Audio to work...
    dsy_gpio reset_pin;
    reset_pin.pin  = {DSY_GPIOB, 11};
    reset_pin.mode = DSY_GPIO_MODE_OUTPUT_PP;
    dsy_gpio_init(&reset_pin);
//    dsy_audio_init(&hw.audio_handle);

    // Before audio starts we need to reset the chip
    dsy_gpio_write(&reset_pin, 0); // Power down and reset
    dsy_system_delay(10);
    dsy_gpio_write(&reset_pin, 1); // Normal Operation

    // Init USB
//    hw.usb_handle.TransmitInternal((uint8_t*)sbuffs[TEST_START], strlen(sbuffs[TEST_START]));
    dsy_system_delay(20);

    // Init all 32 out-facing pins.
    for(size_t i = 0; i < 32; i++) 
    {
        outs[i].pin = hw.GetPin(i);
		outs[i].mode = DSY_GPIO_MODE_OUTPUT_PP;
		outs[i].pull = DSY_GPIO_NOPULL;
		dsy_gpio_init(&outs[i]);
		dsy_gpio_write(&outs[i], 0);
    }

    char failmessage[64];
#ifdef TEST_RAM
    // RAM
    // First let's double check the RAM works:
    for(size_t i = 0; i < BIG_BUFF_SIZE; i++)
    {
        huge_friggin_ram_buff[i] = i;
    }

    // Check contents of RAM, and crash if fails
    for(size_t i = 0; i < BIG_BUFF_SIZE; i++)
    {
        if(huge_friggin_ram_buff[i] != i)
        {
            sprintf(failmessage, "RAM Test FAIL\r\n");
			hw.usb_handle.TransmitInternal((uint8_t *)failmessage, strlen(failmessage));
            asm("bkpt 255");
        }
    }
    hw.usb_handle.TransmitInternal((uint8_t*)sbuffs[TEST_RAM_PASS], strlen(sbuffs[TEST_RAM_PASS]));
#endif

#ifdef TEST_QSPI
    // QSPI
    // TEST: Write RAM Buff (4MB to the QSPI, and then read it back).
    // First set to Indirect Polling Mode
    uint32_t msstart, msend;
    uint32_t baseaddr, writesize;
    msstart             = dsy_system_getnow();
    baseaddr            = 0x90000000;
    hw.qspi_handle.mode = DSY_QSPI_MODE_INDIRECT_POLLING;
    dsy_qspi_init(&hw.qspi_handle);
    writesize = (sizeof(qspi_buff[0]) * BIG_BUFF_SIZE);
    dsy_qspi_erase(baseaddr, baseaddr + writesize);
    dsy_qspi_write(baseaddr, writesize, (uint8_t *)huge_friggin_ram_buff);
    // Now reinit to memory mapped
    hw.qspi_handle.mode = DSY_QSPI_MODE_DSY_MEMORY_MAPPED;
    dsy_qspi_init(&hw.qspi_handle);
    // Check contents of QSPI, and crash if fails
    for(size_t i = 0; i < BIG_BUFF_SIZE; i++)
    {
        if(qspi_buff[i] != i)
        {
            sprintf(failmessage, "QSPI Test FAIL\r\n");
			hw.usb_handle.TransmitInternal((uint8_t *)failmessage, strlen(failmessage));
            asm("bkpt 255");
        }
    }
    msend = dsy_system_getnow();
    char qspimess_buff[128];
    sprintf(qspimess_buff,
            "QSPI Test PASS - Duration in ms: %ld\r\n",
            (msend - msstart));
    hw.usb_handle.TransmitInternal((uint8_t *)qspimess_buff, strlen(qspimess_buff));
#endif
    dsy_system_delay(20);

    // Then we'll start pulsing through all the gpio
    size_t gpio_idx;
    gpio_idx = 0;
//    hw.usb_handle.TransmitInternal((uint8_t *)sbuffs[TEST_GPIO_AUDIO],
//                         strlen(sbuffs[TEST_GPIO_AUDIO]));
    hw.StartAudio(AudioTest);
    for(;;)
    {
        // Pulse Count
        for(size_t i = 0; i < 4; i++)
        {
            dsy_gpio_write(&outs[gpio_idx], 1);
            dsy_system_delay(5);
            dsy_gpio_write(&outs[gpio_idx], 0);
            dsy_system_delay(5);
        }
        gpio_idx = (gpio_idx + 1) % 32;
    }
}
