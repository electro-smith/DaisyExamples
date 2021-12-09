#include <stdio.h>
#include <string.h>
#include "daisy_petal.h"
#include "daisysp.h"
#include "fatfs.h"

#define KNOB_ID "KNOB_"
#define SW_ID "SWITCH_"
#define ENC_ID "ENCODER_"

#define TEST_FILE_NAME "DaisyTestFile.txt"
#define TEST_FILE_CONTENTS \
    "This file is used for testing the Daisy breakout boards. Happy Hacking!"

using namespace daisy;
using namespace daisysp;

int  TestSdCard();
void UpdateUsb(int sd_sta);
void UpdateLeds();

// Declare a local daisy_petal for hardware access
DaisyPetal hw;
// Handler for SD Card Hardware
SdmmcHandler   sd;
FatFSInterface fsi;

// Variable for tracking encoder increments since there is it's not a continuous control like the rest.
int32_t enc_tracker;

// This runs at a fixed rate to prepare audio samples
void callback(AudioHandle::InterleavingInputBuffer  in,
              AudioHandle::InterleavingOutputBuffer out,
              size_t                                size)
{
    int32_t inc;
    hw.ProcessDigitalControls();
    hw.ProcessAnalogControls();

    // Handle Enc
    inc = hw.encoder.Increment();
    if(inc > 0)
        enc_tracker += 1;
    else if(inc < 0)
        enc_tracker -= 1;
    // Clip
    if(enc_tracker > 100)
        enc_tracker = 100;
    else if(enc_tracker < 0)
        enc_tracker = 0;

    // Audio Rate Loop
    for(size_t i = 0; i < size; i += 2)
    {
        out[i]     = in[i];
        out[i + 1] = in[i + 1];
    }
}

int main(void)
{
    int      sdfail;
    uint32_t led_period, usb_period, now;
    uint32_t last_led_update, last_usb_update;

    // LED Freq = 60Hz
    // USB Freq= 10Hz
    led_period = 5;
    usb_period = 100;

    hw.Init();
    hw.SetAudioBlockSize(4);

    last_led_update = last_usb_update = now = System::GetNow();

    // Init USB as VCOM for sending Serial Data
    hw.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);

    sdfail = TestSdCard();

    hw.StartAdc();
    hw.StartAudio(callback);


    while(1)
    {
        // Do Stuff InfInitely Here
        now = System::GetNow();

        // Update LEDs (Vegas Mode)
        if(now - last_led_update > led_period)
        {
            last_led_update = now;
            UpdateLeds();
        }

        if(now - last_usb_update > usb_period)
        {
            last_usb_update = now;
            // opposite of sdfail because
            // sdfail is 1 when fail, 0 when success
            // we want to display 1 for success in the tester
            UpdateUsb(!sdfail);
        }
    }
}

/** Global File object for working with test file */
FIL SDFile;

// Mounts SD card and reads TEST_FILE
// returns 0 if successful, else failure.
int TestSdCard()
{
    int      sta;
    char     inbuff[2048];
    char     refbuff[2048];
    char *   buff;
    uint32_t len, bytesread;

    memset(inbuff, 0, 2048);
    memset(refbuff, 0, 2048);

    // Init SDMMC
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);
    fsi.Init(FatFSInterface::Config::MEDIA_SD);
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    // Fill reference buffer with test contents
    sprintf(refbuff, "%s", TEST_FILE_CONTENTS);
    len = strlen(refbuff);

    // Read from file and compare
    buff = inbuff;
    if(f_open(&SDFile, TEST_FILE_NAME, FA_READ) == FR_OK)
    {
        f_read(&SDFile, buff, len, (UINT *)&bytesread);
    }
    if(len == bytesread && strcmp(buff, refbuff) == 0)
    {
        sta = 0;
    }
    else
    {
        sta = 1;
    }
    return sta;
}

// Send Control Data
// Format:
// { ctrl_id, ctrl_data }, {ctrl_id, ctrl_data}, etc.
void UpdateUsb(int sd_sta)
{
    char catbuff[4095];
    char buff[128];
    memset(catbuff, 0, 4095);
    memset(buff, 0, 128);
    // Get Ready to send:
    // Knobs
    for(size_t i = 0; i < hw.KNOB_LAST; i++)
    {
        sprintf(
            buff,
            "{%s%d, %d},",
            KNOB_ID,
            i + 1,
            (int)(hw.GetKnobValue(static_cast<DaisyPetal::Knob>(i)) * 1000.f));
        strcat(catbuff, buff);
    }
    // Expression
    sprintf(buff, "{EXP_0, %d},", (int)(hw.GetExpression() * 1000.f));
    strcat(catbuff, buff);
    // Switches
    for(size_t i = 0; i < hw.SW_LAST; i++)
    {
        sprintf(buff, "{%s%d, %d},", SW_ID, i + 1, hw.switches[i].Pressed());
        strcat(catbuff, buff);
    }
    // Enc
    sprintf(buff, "{%s_SW, %d},", ENC_ID, hw.encoder.Pressed());
    strcat(catbuff, buff);
    sprintf(buff, "{%s_INC, %ld},", ENC_ID, enc_tracker);
    strcat(catbuff, buff);
    // SD Card Status
    sprintf(buff, "{SDMMC, %d}", sd_sta);
    strcat(catbuff, buff);
    // EndTx
    sprintf(buff, "\r\n");
    strcat(catbuff, buff);
    hw.seed.usb_handle.TransmitInternal((uint8_t *)catbuff, strlen(catbuff));
}

void UpdateLeds()
{
    uint32_t now;
    now = System::GetNow();
    hw.ClearLeds();
    // Use now as a source for time so we don't have to use any global vars
    // First gradually pulse all 4 Footswitch LEDs
    for(size_t i = 0; i < hw.FOOTSWITCH_LED_LAST; i++)
    {
        size_t total, base;
        total        = 511;
        base         = total / hw.FOOTSWITCH_LED_LAST;
        float bright = (float)((now + (i * base)) & total) / (float)total;
        hw.SetFootswitchLed(static_cast<DaisyPetal::FootswitchLed>(i), bright);
    }
    // And now the ring
    for(size_t i = 0; i < hw.RING_LED_LAST; i++)
    {
        float    rb, gb, bb;
        uint32_t total, base;
        uint32_t col;
        col = (now >> 10) % 6;
        //        total = 8191;
        //        base  = total / (hw.RING_LED_LAST);
        total        = 1023;
        base         = total / hw.RING_LED_LAST;
        float bright = (float)((now + (i * base)) & total) / (float)total;
        bright       = 1.0f - bright;
        switch(col)
        {
            case 0:
                rb = bright;
                gb = 0.0f;
                bb = 0.0f;
                break;
            case 1:
                rb = 0.6f * bright;
                gb = 0.0f;
                bb = 0.7f * bright;
                break;
            case 2:
                rb = 0.0f;
                gb = bright;
                bb = 0.0f;
                break;
            case 3:
                rb = 0.0f;
                gb = 0.0f;
                bb = bright;
                break;
            case 4:
                rb = 0.75f * bright;
                gb = 0.75f * bright;
                bb = 0.0f;
                break;
            case 5:
                rb = 0.0f;
                bb = 0.85f * bright;
                gb = 0.85f * bright;
                break;

            default: rb = gb = bb = bright; break;
        }

        hw.SetRingLed(static_cast<DaisyPetal::RingLed>(i), rb, gb, bb);
    }
    hw.UpdateLeds();
}
