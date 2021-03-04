#pragma once
#include "daisy.h"
#include "fatfs.h"

/** Loads a bank of wavetables into memory. 
 ** Pointers to the start of each waveform will be provided, 
 ** but the user can do whatever they want with the data once
 ** it's imported. 
 **
 ** A internal 4kB workspace is used for reading from the file, and conveting to the correct memory location. 
 ** */
class WaveTableLoader
{
  public:
    enum class Result
    {
        OK,
        ERR_TABLE_INFO_OVERFLOW,
        ERR_FILE_READ,
        ERR_GENERIC,
    };
    WaveTableLoader() {}
    ~WaveTableLoader() {}

    /** Initializes the Loader */
    void Init(float *mem, size_t mem_size)
    {
        buf_             = mem;
        buf_size_        = mem_size;
        samps_per_table_ = 256;
        num_tables_      = 1;
    }

    /** Sets the size of the tables to allow access to the specific waveforms */
    Result SetWaveTableInfo(size_t samps, size_t count)
    {
        if(samps * count > buf_size_)
            return Result::ERR_TABLE_INFO_OVERFLOW;
        samps_per_table_ = samps;
        num_tables_      = count;
        return Result::OK;
    }

    /** Opens and loads the file 
     ** The data will be converted from its original type to float
     ** And the wavheader data will be stored internally to the class, 
     ** but will not be stored in the user-provided buffer.
     **
     ** Currently only 16-bit and 32-bit data is supported.
     ** The importer also assumes data is mono
     ** */
    Result Import(const char *filename)
    {
        if(f_open(&fp_, filename, FA_READ | FA_OPEN_EXISTING) == FR_OK)
        {
            // First Grab the Wave header info
            unsigned int br;
            f_read(&fp_, &header_, sizeof(header_), &br);
            uint32_t wptr = 0;
            do
            {
                f_read(&fp_,
                       workspace,
                       kWorkspaceSize * sizeof(workspace[0]),
                       &br);
                // Fill mem
                switch(header_.BitPerSample)
                {
                    case 16:
                    {
                        int16_t *wsp;
                        wsp = (int16_t *)workspace;
                        for(size_t i = 0; i < kWorkspaceSize*2; i++)
                        {
                            buf_[wptr] = s162f(wsp[i]);
                            wptr++;
                        }
                    }
                    break;
                    case 32:
                    {
                        float *wsp;
                        wsp = (float *)workspace;
                        for(size_t i = 0; i < kWorkspaceSize; i++)
                        {
                            buf_[wptr] = wsp[i];
                        }
                    }
                    break;
                    default: break;
                }
            } while(!f_eof(&fp_) || wptr <= buf_size_ - 1);
            f_close(&fp_);
        }
        else
        {
            return Result::ERR_FILE_READ;
        }
        return Result::OK;
    }

    /** Returns pointer to specific table start or nullptr if invalid idx */
    float *GetTable(size_t idx)
    {
        return idx < num_tables_ ? &buf_[idx * samps_per_table_] : nullptr;
    }

  private:
    static constexpr int kWorkspaceSize = 1024;
    float *              buf_;
    size_t               buf_size_;
    WAV_FormatTypeDef    header_;
    size_t               samps_per_table_;
    size_t               num_tables_;
    int32_t              workspace[kWorkspaceSize];
    FIL     fp_;
};
