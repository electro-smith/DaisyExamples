#include <algorithm>
#include <list>
#include "daisy_field.h"
#include "daisysp.h"
#include "fatfs.h"

//#define PLAYBACK_TEST
#define RECORDING_TEST
#define WAVETABLE_TEST

using namespace daisy;
using namespace daisysp;

static const char kPreAudioTestFileName[]  = "PreAudioTest.txt";
static const char kPostAudioTestFileName[] = "PostAudioTest.txt";
static const char kPreAudioBuff[] = "This is a test prior to starting audio.";
static const char kPostAudioBuff[]
    = "Successfully Read test file, and wrote this file while audio was "
      "playing";
static constexpr int kMaxFiles       = 32;
static constexpr int kMaxNameLength  = _MAX_LFN;
static constexpr int kLineSpacing    = 11;
static constexpr int kSdTransferSize = 16384;

static const char kLoopFileName[]     = "LoopFile.wav";
static const char kWaveformFileName[] = "ROM A.wav";

static const int kWaveTableSize  = 256;
static const int kWaveTableCount = 64;

struct FileInfo
{
    char   name[kMaxNameLength];
    size_t name_size;
    size_t filesize;
};

enum class BufferState
{
    BUSY,
    FILL0,
    FILL1,
};

/** 4MB buffer to test SDRAM */
static int32_t DSY_SDRAM_BSS rambuff[1024 * 1024];
static float DSY_SDRAM_BSS wavetables[kWaveTableSize * kWaveTableCount];

static int16_t rx_buff[kSdTransferSize * 2];
static int16_t tx_buff[kSdTransferSize * 2];

static BufferState       bstate;
static uint32_t          rptr;
static WAV_FormatTypeDef finfo;


static DaisyField   hw;
static SdmmcHandler sdcard;
static FIL          fp;
static Oscillator   osc;


typedef WavWriter<32768> MyWriter;
static MyWriter          recorder;

static WaveTableLoader wav_loader;

bool queue_rec_;


void PlaybackCallback(float **in, float **out, size_t size)
{
    for(size_t i = 0; i < size; i++)
    {
        float samp = ((float)rx_buff[rptr] / 32767.f);
        rptr       = rptr + 1;
        if(rptr > (kSdTransferSize * 2) - 1)
            rptr = 0;
        if(rptr == kSdTransferSize)
            bstate = BufferState::FILL0;
        else if(rptr == 0)
            bstate = BufferState::FILL1;
        float sig = samp;
        out[0][i] = sig;
        out[1][i] = sig;
    }
}

void RecordingCallback(float **in, float **out, size_t size)
{
    hw.ProcessDigitalControls();
    /** Set flag for toggling rec state. */
    if(hw.sw[DaisyField::SW_1].FallingEdge())
    {
        queue_rec_ = true;
    }
    for(size_t i = 0; i < size; i++)
    {
        float rec[2];
        rec[0] = in[0][i];
        rec[1] = in[1][i];
        // And passthru because why not
        out[0][i] = rec[0];
        out[1][i] = rec[1];
    }
}

float wavetable_ptr;
float freq;
void  WaveTableCallback(float **in, float **out, size_t size)
{
    hw.ProcessAllControls();
    float  increment;
    float *t;
    freq = mtof(12.f + hw.GetKnobValue(DaisyField::KNOB_2) * 84.f);
    t    = wav_loader.GetTable(
        hw.GetKnobValue(DaisyField::KNOB_1) * kWaveTableCount - 1);
    if(hw.sw[DaisyField::SW_1].FallingEdge())
    {
        queue_rec_ = true;
    }
    increment = (freq / hw.AudioSampleRate()) * kWaveTableSize;
    if(t != nullptr)
    {
        for(size_t i = 0; i < size; i++)
        {
            uint32_t pos;
            float    now, next, frac, sig;
            pos = (uint32_t)wavetable_ptr;
            /** Lerp */
            now  = t[pos];
            next = t[pos + 1];
            frac = wavetable_ptr - pos;
            sig  = now + (next - now) * frac;
            /** Handle Read Phasor */
            wavetable_ptr += increment;
            if(wavetable_ptr > kWaveTableSize - 1)
                wavetable_ptr -= kWaveTableSize;

            /** Record to Wav */
            float rec[2];
            rec[0] = rec[1] = sig;
            if(recorder.IsRecording())
                recorder.Sample(rec);
            /** And output */
            out[0][i] = out[1][i] = sig;
        }
    }
}

int main(void)
{
    uint32_t now, dt;
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    hw.StartAdc();
    //hw.SetAudioBlockSize(48);
    hw.display.WriteString("SDMMC DMA Test", Font_7x10, true);
    now = dt == System::GetNow();

    osc.Init(hw.AudioSampleRate());


    /** Init and Mount */
    SdmmcHandler::Config sdconfig;
    sdconfig.clock_powersave = false;
    sdconfig.speed           = SdmmcHandler::Speed::FAST;
    sdconfig.width           = SdmmcHandler::BusWidth::BITS_4;
    sdcard.Init(sdconfig);
    dsy_fatfs_init();
    f_mount(&SDFatFS, SDPath, 1);


    /** Open Root directory */
    hw.display.SetCursor(0, kLineSpacing * 1);
    DIR dir;
    //RingBuffer<FileInfo, kMaxFiles> filelist;
    std::list<FileInfo> filelist;
    if(f_opendir(&dir, SDPath) != FR_OK)
    {
        hw.display.WriteString("Cannot open root folder", Font_6x8, true);
    }
    else
    {
        hw.display.WriteString("Opened /", Font_6x8, true);

        /** Identify Files (ignoring folders for now) */
        FRESULT res;
        FILINFO fno;
        char *  fn;
        do
        {
            res = f_readdir(&dir, &fno);
            if(res != FR_OK || fno.fname[0] == 0)
                break;
            if(fno.fattrib & (AM_HID | AM_DIR))
                continue;
            FileInfo f;
            strcpy(f.name, fno.fname);
            f.name_size = strlen(f.name);
            f.filesize  = fno.fsize;
            filelist.push_back(f);
        } while(res == FR_OK);
        /** Close Directory; we're done here */
        f_closedir(&dir);
    }

    /** Print File Count on Display */
    size_t num_files = filelist.size();
    hw.display.SetCursor(0, kLineSpacing * 2);
    char sbuff[64];
    sprintf(sbuff, "Found %d files", num_files);
    hw.display.WriteString((const char *)sbuff, Font_6x8, true);

    /** Write a test file before Audio starts */
    hw.display.SetCursor(0, kLineSpacing * 3);
    if(f_open(&fp, kPreAudioTestFileName, (FA_WRITE | FA_CREATE_ALWAYS))
       == FR_OK)
    {
        unsigned int byteswritten = 0;
        if(f_write(
               &fp, (void *)kPreAudioBuff, strlen(kPreAudioBuff), &byteswritten)
           != FR_OK)
        {
            hw.display.WriteString("Could not write test file", Font_6x8, true);
        }
        else
        {
            hw.display.WriteString("Pass: wrote pretest", Font_6x8, true);
        }
        f_close(&fp);
    }
    else
    {
        hw.display.WriteString(
            "Could not open file for writing", Font_6x8, true);
    }

    for(size_t i = 0; i < 1024 * 1024; i++)
    {
        rambuff[i] = i;
    }
    for(size_t i = 0; i < 1024 * 1024; i++)
    {
        if(rambuff[i] != i)
            asm("bkpt 255"); // uh-oh
    }


/** Start Audio now */
#ifdef PLAYBACK_TEST
    hw.StartAudio(PlaybackCallback);
#endif
#ifdef RECORDING_TEST
    MyWriter::Config wcfg;
    wcfg.samplerate    = 96000;
    wcfg.channels      = 2;
    wcfg.bitspersample = 16;
    recorder.Init(wcfg);
    //hw.StartAudio(RecordingCallback);
#endif
#ifdef WAVETABLE_TEST
    wav_loader.Init(wavetables, kWaveTableCount * kWaveTableSize);
    wav_loader.SetWaveTableInfo(kWaveTableSize, kWaveTableCount);
    wav_loader.Import(kWaveformFileName);
    hw.StartAudio(WaveTableCallback);
#endif


#ifdef PLAYBACK_TEST
    /** Read back PreAudioTest */
    hw.display.SetCursor(0, kLineSpacing * 4);
    bool read_while_audio_test, write_while_audio_test;
    read_while_audio_test  = false;
    write_while_audio_test = false;
    if(f_open(&fp, kPreAudioTestFileName, (FA_READ | FA_OPEN_EXISTING))
       == FR_OK)
    {
        // Fill buff with contents and compare
        unsigned int bytesread = 0;
        for(size_t i = 0; i < 64; i++)
            sbuff[i] = 0;
        if(f_read(&fp, sbuff, strlen(kPreAudioBuff), &bytesread) == FR_OK)
        {
            // Compare to written message
            int notsame = 0;
            notsame     = strcmp(sbuff, kPreAudioBuff);
            if(!notsame)
            {
                read_while_audio_test = true;
            }
            else
            {
                hw.display.WriteString("Failed read test", Font_6x8, true);
            }
        }
    }
    else
    {
        hw.display.WriteString("Failed read test", Font_6x8, true);
    }

    /** If matched, write post audio test */
    if(read_while_audio_test)
    {
        if(f_open(&fp, kPostAudioTestFileName, (FA_WRITE | FA_CREATE_ALWAYS))
           == FR_OK)
        {
            unsigned int byteswritten = 0;
            if(f_write(&fp,
                       (void *)kPostAudioBuff,
                       strlen(kPostAudioBuff),
                       &byteswritten)
               != FR_OK)
            {
                hw.display.WriteString(
                    "Could not write test file", Font_6x8, true);
            }
            else
            {
                hw.display.WriteString("Pass: wrote posttest", Font_6x8, true);
            }
            f_close(&fp);
        }
        else
        {
            hw.display.WriteString(
                "Could not open file for writing", Font_6x8, true);
        }
    }

    /** Find first wav file */
    FileInfo sndfile;
    for(std::list<FileInfo>::iterator it = filelist.begin();
        it != filelist.end();
        it++)
    {
        if(strstr(it->name, ".wav") || strstr(it->name, ".WAV"))
        {
            sndfile = *it;
            break;
        }
    }

    /** Open Wave file */
    if(f_open(&fp, sndfile.name, (FA_READ | FA_OPEN_EXISTING)) != FR_OK)
    {
        // Bad
    }

    /** Read Info */
    unsigned int bytesread = 0;
    if(f_read(&fp, &finfo, sizeof(finfo), &bytesread) != FR_OK)
    {
        // More bad
    }
#endif

    while(1)
    {
        now = System::GetNow();

#ifdef PLAYBACK_TEST
        if(bstate != BufferState::BUSY)
        {
            uint32_t offset
                = bstate == BufferState::FILL0 ? 0 : kSdTransferSize;
            bstate    = BufferState::BUSY;
            bytesread = 0;
            f_read(&fp,
                   rx_buff + offset,
                   sizeof(rx_buff[0]) * kSdTransferSize,
                   &bytesread);
            if(f_eof(&fp) || bytesread == 0)
            {
                f_lseek(&fp, sizeof(finfo));
            }
        }
#endif

#ifdef RECORDING_TEST
        if(recorder.IsRecording())
            recorder.Write();
        if(queue_rec_)
        {
            queue_rec_ = false;
            if(recorder.IsRecording())
            {
                recorder.SaveFile();
            }
            else
            {
                recorder.OpenFile(kLoopFileName);
            }
        }


        char rbuff[32];
        sprintf(rbuff, "Recording: %d", recorder.IsRecording() ? 1 : 0);
        hw.display.SetCursor(0, kLineSpacing * 4);
        hw.display.WriteString(rbuff, Font_6x8, true);

#endif

        /** Update Display */
        if(now - dt > 16)
        {
            dt = now;
            hw.display.Update();
        }
    }
}
