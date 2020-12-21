#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"

using namespace daisy;

static DaisySeed hw;

uint8_t sumbuff[1024];

void UsbCallback(uint8_t* buf, uint32_t* len)
{
    for(size_t i = 0; i < *len; i++)
    {
        sumbuff[i] = buf[i];
    }
}


int main(void)
{
    hw.Configure();
    hw.Init();
    hw.usb_handle.Init(UsbHandle::FS_INTERNAL);
    int  tick_cnt = 0;
    bool ledstate = false;
    char buff[512];
    sprintf(buff, "Tick:\t%d\r\n", tick_cnt);
    hw.usb_handle.TransmitInternal((uint8_t*)buff, strlen(buff));
    System::Delay(500);
    hw.usb_handle.SetReceiveCallback(UsbCallback, UsbHandle::FS_INTERNAL);
    while(1)
    {
        System::Delay(500);
        sprintf(buff, "Tick:\t%d\r\n", tick_cnt);
        hw.usb_handle.TransmitInternal((uint8_t*)buff, strlen(buff));
        tick_cnt = (tick_cnt + 1) % 100;
        hw.SetLed(ledstate);
        ledstate = !ledstate;
    }
}
