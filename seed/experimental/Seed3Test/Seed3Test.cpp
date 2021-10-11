// # Seed3Test
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
UsbHandle                         usb;

#define TEST_RAM

// TEST RAM is required for testing the QSPI.
#define TEST_QSPI


#define GPIO_0       \
    {                \
        DSY_GPIOA, 8 \
    }
#define GPIO_1        \
    {                 \
        DSY_GPIOB, 12 \
    }
#define GPIO_2        \
    {                 \
        DSY_GPIOC, 11 \
    }
#define GPIO_3        \
    {                 \
        DSY_GPIOC, 10 \
    }
#define GPIO_4       \
    {                \
        DSY_GPIOC, 9 \
    }
#define GPIO_5       \
    {                \
        DSY_GPIOC, 8 \
    }
#define GPIO_6       \
    {                \
        DSY_GPIOD, 2 \
    }
#define GPIO_7        \
    {                 \
        DSY_GPIOC, 12 \
    }
#define GPIO_8        \
    {                 \
        DSY_GPIOG, 10 \
    }
#define GPIO_9        \
    {                 \
        DSY_GPIOG, 11 \
    }
#define GPIO_10      \
    {                \
        DSY_GPIOB, 4 \
    }
#define GPIO_11      \
    {                \
        DSY_GPIOB, 5 \
    }
#define GPIO_12      \
    {                \
        DSY_GPIOB, 8 \
    }
#define GPIO_13      \
    {                \
        DSY_GPIOB, 9 \
    }
#define GPIO_14      \
    {                \
        DSY_GPIOB, 6 \
    }
#define GPIO_15      \
    {                \
        DSY_GPIOB, 7 \
    }

#define GPIO_16      \
    {                \
        DSY_GPIOC, 0 \
    }
#define GPIO_17      \
    {                \
        DSY_GPIOA, 3 \
    }
#define GPIO_18      \
    {                \
        DSY_GPIOB, 1 \
    }
#define GPIO_19      \
    {                \
        DSY_GPIOA, 7 \
    }
#define GPIO_20      \
    {                \
        DSY_GPIOA, 6 \
    }
#define GPIO_21      \
    {                \
        DSY_GPIOC, 1 \
    }
#define GPIO_22      \
    {                \
        DSY_GPIOC, 4 \
    }
#define GPIO_23      \
    {                \
        DSY_GPIOA, 5 \
    }
#define GPIO_24      \
    {                \
        DSY_GPIOA, 4 \
    }
#define GPIO_25      \
    {                \
        DSY_GPIOA, 1 \
    }
#define GPIO_26      \
    {                \
        DSY_GPIOA, 0 \
    }
#define GPIO_27       \
    {                 \
        DSY_GPIOD, 11 \
    }
#define GPIO_28      \
    {                \
        DSY_GPIOG, 9 \
    }
#define GPIO_29      \
    {                \
        DSY_GPIOA, 2 \
    }
#define GPIO_30       \
    {                 \
        DSY_GPIOB, 14 \
    }
#define GPIO_31       \
    {                 \
        DSY_GPIOB, 15 \
    }

dsy_gpio_pin seedv3_pins[32] = {
    GPIO_0,  GPIO_1,  GPIO_2,  GPIO_3,  GPIO_4,  GPIO_5,  GPIO_6,  GPIO_7,
    GPIO_8,  GPIO_9,  GPIO_10, GPIO_11, GPIO_12, GPIO_13, GPIO_14, GPIO_15,
    GPIO_16, GPIO_17, GPIO_18, GPIO_19, GPIO_20, GPIO_21, GPIO_22, GPIO_23,
    GPIO_24, GPIO_25, GPIO_26, GPIO_27, GPIO_28, GPIO_29, GPIO_30, GPIO_31,
};

dsy_gpio outs[32];

// 4MB RAM Test
#define BIG_BUFF_SIZE (1024 * 1024)
uint32_t DSY_SDRAM_BSS huge_friggin_ram_buff[BIG_BUFF_SIZE];

// 4 MB QSPI Test
uint32_t DSY_QSPI_BSS qspi_buff[BIG_BUFF_SIZE];

DaisySeed hw;

uint32_t phasor = 0;
float    sig;

void AudioTest(AudioHandle::InterleavingInputBuffer in, AudioHandle::InterleavingOutputBuffer out, size_t size)
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
    hw.Init();
    hw.SetAudioBlockSize(4);

    osc.Init(DSY_AUDIO_SAMPLE_RATE);
    osc.SetAmp(0.75f);
    osc.SetFreq(100.0f);

    // Update Audio to work...
    dsy_gpio reset_pin;
    reset_pin.pin  = {DSY_GPIOB, 11};
    reset_pin.mode = DSY_GPIO_MODE_OUTPUT_PP;
    dsy_gpio_init(&reset_pin);
    hw.audio_handle.sai->device[DSY_AUDIO_INTERNAL]   = DSY_AUDIO_NONE;
    hw.audio_handle.sai->device[DSY_AUDIO_EXTERNAL]   = DSY_AUDIO_NONE;
    hw.audio_handle.sai->bitdepth[DSY_AUDIO_INTERNAL] = DSY_AUDIO_BITDEPTH_24;
    dsy_audio_init(&hw.audio_handle);

    // Before audio starts we need to reset the chip
    dsy_gpio_write(&reset_pin, 0); // Power down and reset
    dsy_system_delay(10);
    dsy_gpio_write(&reset_pin, 1); // Normal Operation

    // Init USB
    usb.Init(UsbHandle::FS_INTERNAL);
    dsy_system_delay(4000);
    usb.TransmitInternal((uint8_t*)sbuffs[TEST_START], strlen(sbuffs[TEST_START]));
    dsy_system_delay(20);

    // Init all 32 out-facing pins.
    for(size_t i = 0; i < 32; i++)
    {
        outs[i].pin  = seedv3_pins[i];
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
			usb.TransmitInternal((uint8_t *)failmessage, strlen(failmessage));
            asm("bkpt 255");
        }
    }
    usb.TransmitInternal((uint8_t*)sbuffs[TEST_RAM_PASS], strlen(sbuffs[TEST_RAM_PASS]));
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
    // Now reInit to memory mapped
    hw.qspi_handle.mode = DSY_QSPI_MODE_DSY_MEMORY_MAPPED;
    dsy_qspi_init(&hw.qspi_handle);
    // Check contents of QSPI, and crash if fails
    for(size_t i = 0; i < BIG_BUFF_SIZE; i++)
    {
        if(qspi_buff[i] != i)
        {
            sprintf(failmessage, "QSPI Test FAIL\r\n");
			usb.TransmitInternal((uint8_t *)failmessage, strlen(failmessage));
            asm("bkpt 255");
        }
    }
    msend = dsy_system_getnow();
    char qspimess_buff[128];
    sprintf(qspimess_buff,
            "QSPI Test PASS - Duration in ms: %ld\r\n",
            (msend - msstart));
    usb.TransmitInternal((uint8_t *)qspimess_buff, strlen(qspimess_buff));
#endif
    dsy_system_delay(20);

    // Then we'll start pulsing through all the gpio
    size_t gpio_idx;
    gpio_idx = 0;
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, 256);
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, AudioTest);
    usb.TransmitInternal((uint8_t*)sbuffs[TEST_GPIO_AUDIO], strlen(sbuffs[TEST_GPIO_AUDIO]));
    dsy_audio_start(DSY_AUDIO_INTERNAL);
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
