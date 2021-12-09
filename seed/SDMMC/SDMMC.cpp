// This example uses fatfs to access files via SD Card.
//
// A File test.txt is created with the contentt of outbuff.
// The file is then read back in to inbuff.
// If there is any failure, or what was read does not match
// what was written, execution will halt at the breakpoint.
// Otherwise, the Seed's onboard LED will begin to blink.
//
// FATFS is not bundled into the libdaisy, and therefore needs
// to be included in the project to work
//
// The SD implementation of libdaisy is subject to change,
// however wrapping fatfs into libdaisy would be a substantial
// chunk of work, and FatFs itself is a pretty familiar interface.
//
// This particular project will work as long as the following Middleware
// files are included in the project (alongside libdaisy).
// Sources/headers within: libdaisy/Middlewares/Third_Party/FatFs/src/
// options: ccsbcs.c (in src/options within the above path)
//
// The SD card is currently configured to run with 4-bits and a 12MHz clock frequency.
//
#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "fatfs.h"

#define TEST_FILE_NAME "test.txt"

using namespace daisy;

static DaisySeed hw;

SdmmcHandler   sd;
FatFSInterface fsi;
FIL            SDFile;

int main(void)
{
    // Vars and buffs.
    char   outbuff[512];
    char   inbuff[512];
    size_t len, failcnt, byteswritten;
    sprintf(outbuff, "Daisy...Testing...\n1...\n2...\n3...\n");
    memset(inbuff, 0, 512);
    len     = strlen(outbuff);
    failcnt = 0;

    // Init hardware
    hw.Init();

    // Init SD Card
    SdmmcHandler::Config sd_cfg;
    sd_cfg.Defaults();
    sd.Init(sd_cfg);

    // Links libdaisy i/o to fatfs driver.
    fsi.Init(FatFSInterface::Config::MEDIA_SD);

    // Mount SD Card
    f_mount(&fsi.GetSDFileSystem(), "/", 1);

    // Open and write the test file to the SD Card.
    if(f_open(&SDFile, TEST_FILE_NAME, (FA_CREATE_ALWAYS) | (FA_WRITE))
       == FR_OK)
    {
        f_write(&SDFile, outbuff, len, &byteswritten);
        f_close(&SDFile);
    }

    // Read back the test file from the SD Card.
    if(f_open(&SDFile, TEST_FILE_NAME, FA_READ) == FR_OK)
    {
        f_read(&SDFile, inbuff, len, &byteswritten);
        f_close(&SDFile);
    }

    // Check for sameness.
    for(size_t i = 0; i < len; i++)
    {
        if(inbuff[i] != outbuff[i])
        {
            failcnt++;
        }
    }
    // If what was read does not match
    // what was written execution will stop.
    if(failcnt)
    {
        asm("bkpt 255");
    }
    bool ledstate;
    ledstate = true;
    // Otherwise the onboard LED will begin to blink.
    for(;;)
    {
        System::Delay(250);
        hw.SetLed(ledstate);
        ledstate = !ledstate;
    }
}
