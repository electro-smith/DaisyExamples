// # WavPlayer
// ## Description
//
// Play .wav file from the SD Card.
//
// Behavior:
//
// - Encoder Will select the file
// - File name will be indicated on an OLED.
// - A push button will be used to toggle the playback.
//
// Hardware Details connected to Seed:
// Essentially the Pod, but with some stuff wired out for the OLED.
//
// - Oled connected via SPI, DC and RST are connected via GPIOB8, and GPIOB9 respectively.
// - SDMMC connected to all 6 SDMMC Pins on Seed.
// - Push button wired to GPIO 29
// - Encoder wired with:
//		- A: GPIO 27
//		- B: GPIO 26
//		- Click: GPIO 1
//
#include <stdio.h>
#include <string.h>
#include "daisy_seed.h"
#include "fatfs.h"

using namespace daisy;

// TODO MOVE THIS STUFF TO ANOTHER FILE/ADD TO LIBDAISY:


typedef enum
{
    AUDIO_ERROR_NONE = 0,
    AUDIO_ERROR_IO,
    AUDIO_ERROR_EOF,
    AUDIO_ERROR_INVALID_VALUE,
} Audio_ErrorTypeDef;

typedef struct
{
    uint32_t ChunkId;
    uint32_t FileSize;
    uint32_t FileFormat;
    uint32_t SubChunk1ID;
    uint32_t SubChunk1Size;
    uint16_t AudioFormat;
    uint16_t NbrChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitPerSample;
    uint32_t SubChunk2ID;
    uint32_t SubCHunk2Size;
} WAV_FormatTypeDef;

class WavFileList
{
  public:
    struct WavFileInfo
    {
        char              name[FILENAME_MAX];
        WAV_FormatTypeDef info;
    };

    enum BufferState
    {
        BUFFER_IDLE,
        BUFFER_PREPARE_0,
        BUFFER_PREPARE_1,
    };
    WavFileList() {}
    ~WavFileList() {}

    void Init()
    {
        // First check for all .wav files, and add them to the list until its full or there are no more.
        // Only checks '/'
        FRESULT result = FR_OK;
        FILINFO fno;
        DIR     dir;
        char *  fn;
        file_sel_ = 0;
        file_cnt_ = 0;
        if(f_opendir(&dir, SDPath) != FR_OK)
        {
            return;
        }
        do
        {
            result = f_readdir(&dir, &fno);
            // Skip if its a directory or a hidden file.
            if(result != FR_OK || (fno.fattrib & (AM_HID | AM_DIR)))
                continue;
            // Now we'll check if its .wav and add to the list.
            fn = fno.fname;
            if(file_cnt_ < kMaxWavFiles - 1)
            {
                if(strstr(fn, ".wav") || strstr(fn, ".WAV"))
                {
                    strcpy(file_info_[file_cnt_].name, fn);
                    file_cnt_++;
                    // For now lets break anyway to test.
                    break;
                }
            }
            else
            {
                break;
            }
        } while(result == FR_OK);
        f_closedir(&dir);
        // Now we'll go through each file and load the WavInfo.
        for(size_t i = 0; i < file_cnt_; i++)
        {
            size_t bytesread;
            if(f_open(&SDFile, file_info_[i].name, (FA_OPEN_EXISTING | FA_READ))
               == FR_OK)
            {
                // Populate the WAV Info
                if(f_read(&SDFile,
                          (void *)&file_info_[i].info,
                          sizeof(WAV_FormatTypeDef),
                          &bytesread)
                   != FR_OK)
                {
                    // Maybe add return type
                    return;
                }
                f_close(&SDFile);
            }
        }
        // fill buffer with first file preemptively.
        buff_state_ = BUFFER_PREPARE_0;
        Open(0);
        Prepare();
        rptr_ = 0;
    }

    int Open(size_t sel)
    {
        return (f_open(&SDFile,
                       file_info_[sel < file_cnt_ ? sel : 0].name,
                       (FA_OPEN_EXISTING | FA_READ)));
    }

    int Close() { return f_close(&SDFile); }

    // Returns next sample in buffer
    // Updates read ptr, and sets flag for buffer collection
    int16_t Stream()
    {
        int16_t samp;
        samp = buff[rptr_];
        // Increment rpo
        rptr_ = (rptr_ + 1) % kRxBufferSize;
        if(rptr_ == 0)
            buff_state_ = BUFFER_PREPARE_1;
        else if(rptr_ == kRxBufferSize / 2)
            buff_state_ = BUFFER_PREPARE_0;
        return samp;
    }

    void Prepare()
    {
        if(buff_state_ != BUFFER_IDLE)
        {
            size_t offset, bytesread, rxsize;
            bytesread = 0;
            rxsize    = (kRxBufferSize / 2) * sizeof(buff[0]);
            offset    = buff_state_ == BUFFER_PREPARE_1 ? kRxBufferSize / 2 : 0;
            f_read(&SDFile, &buff[offset], rxsize, &bytesread);
            if(bytesread < rxsize)
            {
                // Perma-looping for now.
                f_lseek(&SDFile,
                        sizeof(WAV_FormatTypeDef)
                            + file_info_[file_sel_].info.SubChunk1Size);
                f_read(&SDFile, &buff[offset + (bytesread / 2)], rxsize - bytesread, &bytesread);
            }
            buff_state_ = BUFFER_IDLE;
        }
    }

  private:
    static constexpr size_t kMaxWavFiles  = 16;
    static constexpr size_t kRxBufferSize = 1024;
    WavFileInfo             file_info_[kMaxWavFiles];
    size_t                  file_cnt_, file_sel_;
    size_t                  rptr_;
    int16_t                 buff[kRxBufferSize];
    BufferState             buff_state_;
};


static daisy_handle hw;
OledDisplay         display;
SdmmcHandler        sdcard;
WavFileList         flist;

void AudioCallback(float *in, float *out, size_t size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        out[i] = out[i + 1] = s162f(flist.Stream()) * 0.5f;
    }
}

int main(void)
{
    // Init hardware
    size_t blocksize = 12;
    daisy_seed_init(&hw);
    display.Init();
    sdcard.Init();
    dsy_fatfs_init();
    // Let's get the first wav file we find!
    // First Mount the SD card and look for a file.
    f_mount(&SDFatFS, SDPath, 1);
    flist.Init();

    // Init Audio
    dsy_audio_set_blocksize(DSY_AUDIO_INTERNAL, blocksize);
    dsy_audio_set_callback(DSY_AUDIO_INTERNAL, AudioCallback);
    dsy_audio_start(DSY_AUDIO_INTERNAL);
    // Otherwise the onboard LED will begin to blink.
    for(;;)
    {
        flist.Prepare();
    }
}
