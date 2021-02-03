#include "daisy_seed.h"
#include <string.h>

using namespace daisy;

static DaisySeed hw;
static uint8_t   sumbuff[1024];
static uint32_t  rx_size;
static bool      update;

void UsbCallback(uint8_t* buf, uint32_t* len)
{
    memcpy(sumbuff, buf, *len);
    rx_size = *len;
    update  = true;
}

int main(void)
{
    hw.Configure();
    hw.Init();
    update  = false;
    rx_size = 0;
    hw.usb_handle.Init(UsbHandle::FS_INTERNAL);
    // Todo: replace delay with while(!usb_handle.IsInitialized()) {} or similar
    System::Delay(500);
    hw.usb_handle.SetReceiveCallback(UsbCallback, UsbHandle::FS_INTERNAL);
    for(;;)
    {
        if(update && rx_size > 0)
        {
            hw.usb_handle.TransmitInternal(sumbuff, rx_size);
            update = false;
        }
        System::Delay(2);
    }
}
