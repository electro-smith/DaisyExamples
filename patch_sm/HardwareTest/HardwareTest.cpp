#include "daisy_patch_sm.h"
#include "daisysp.h"

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM   hw;
Switch         button, toggle;
FIL            file; /**< Can't be made on the stack (DTCMRAM) */
FatFSInterface fsi;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    hw.ProcessAllControls();
    button.Debounce();
    toggle.Debounce();
    for(size_t i = 0; i < size; i++)
    {
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

int main(void)
{
    hw.Init();
    hw.StartLog(true);
    hw.PrintLine("Daisy Patch SM started. Test Beginning");

    /** Memory tests first to keep test short if everything else fails */
    bool sdram_pass = hw.ValidateSDRAM();
    hw.PrintLine("SDRAM Test\t-\t%s", sdram_pass ? "PASS" : "FAIL");
    bool qspi_pass = hw.ValidateQSPI();
    hw.PrintLine("QSPI Test\t-\t%s", qspi_pass ? "PASS" : "FAIL");

    /** SD card next */
    SdmmcHandler::Config sd_config;
    SdmmcHandler         sdcard;
    sd_config.Defaults();
    sdcard.Init(sd_config);

    fsi.Init(FatFSInterface::Config::MEDIA_SD);


    /** Write/Read text file */
    const char *test_string = "Testing Daisy Patch SM";
    const char *test_fname  = "DaisyPatchSM-Test.txt";
    FRESULT     fres = FR_DENIED; /**< Unlikely to actually experience this */
    if(f_mount(&fsi.GetSDFileSystem(), "/", 0) == FR_OK)
    {
        /** Write Test */
        if(f_open(&file, test_fname, (FA_CREATE_ALWAYS | FA_WRITE)) == FR_OK)
        {
            UINT   bw  = 0;
            size_t len = strlen(test_string);
            fres       = f_write(&file, test_string, len, &bw);
        }
        f_close(&file);
        if(fres == FR_OK)
        {
            /** Read Test only if Write passed */
            if(f_open(&file, test_fname, (FA_OPEN_EXISTING | FA_READ)) == FR_OK)
            {
                UINT   br = 0;
                char   readbuff[32];
                size_t len = strlen(test_string);
                fres       = f_read(&file, readbuff, len, &br);
            }
            f_close(&file);
        }
    }
    bool sdmmc_pass = fres == FR_OK;
    hw.PrintLine("SDMMC Test\t-\t%s", sdmmc_pass ? "PASS" : "FAIL");

    /** 5 second delay before starting streaming test. */
    System::Delay(5000);


    /** Initialize Button/Toggle for rest of test. */
    button.Init(DaisyPatchSM::B7, hw.AudioCallbackRate());
    toggle.Init(DaisyPatchSM::B8, hw.AudioCallbackRate());

    daisysp::Phasor dacphs;
    dacphs.Init(500);
    dacphs.SetFreq(1.f);

    hw.StartAudio(AudioCallback);

    uint32_t now, dact, usbt, gatet;
    now = dact = usbt = System::GetNow();
    gatet             = now;

    while(1)
    {
        now = System::GetNow();
        // 500Hz samplerate for DAC output test
        if(now - dact > 2)
        {
            hw.WriteCvOut(CV_OUT_BOTH, dacphs.Process() * 5.f);
            dact = now;
        }

        if(now - usbt > 100)
        {
            hw.PrintLine("Streaming Test Results");
            hw.PrintLine("######################");
            hw.PrintLine("Analog Inputs:");
            for(int i = 0; i < ADC_LAST; i++)
            {
                hw.Print("%s_%d: " FLT_FMT3,
                         i < ADC_9 ? "CV" : "ADC",
                         i + 1,
                         FLT_VAR3(hw.GetAdcValue(i)));
                if(i != 0 && (i + 1) % 4 == 0)
                    hw.Print("\n");
                else
                    hw.Print("\t");
            }
            hw.PrintLine("######################");
            hw.PrintLine("Digital Inputs:");
            hw.Print("Button: %s\t", button.Pressed() ? "ON" : "OFF");
            hw.Print("Toggle: %s\t", toggle.Pressed() ? "UP" : "DOWN");
            hw.Print("\nGATE_IN_1: %s\t",
                     hw.gate_in_1.State() ? "HIGH" : "LOW");
            hw.PrintLine("GATE_IN_2: %s",
                         hw.gate_in_2.State() ? "HIGH" : "LOW");
            hw.PrintLine("######################");
            usbt = now;
        }
        if(now - gatet > 1000)
        {
            dsy_gpio_toggle(&hw.gate_out_1);
            dsy_gpio_toggle(&hw.gate_out_2);
            gatet = now;
        }

        /** short 60ms blip off on builtin LED */
        hw.SetLed((now & 2047) > 60);
    }
}
