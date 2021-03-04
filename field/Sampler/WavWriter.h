#pragma once
#include "daisy.h"
#include "fatfs.h"

/** Audio Recording Module
 ** 
 ** Record audio into a working buffer that is gradually written to a file on an SD Card. 
 **
 ** Recordings are made with floating point input, and will be converted to the 
 ** specified bits per sample internally 
 **
 ** For now only 16-bit and 32-bit (signed int) formats are supported
 ** f32 and s24 formats will be added next.
 ** */
template <size_t transfer_size>
class WavWriter
{
  public:
    WavWriter() {}
    ~WavWriter() {}

    enum class Result
    {
        OK,
        ERROR,
    };

    struct Config
    {
        float   samplerate;
        int32_t channels;
        int32_t bitspersample;
    };

    enum class BufferState
    {
        IDLE,
        DUMP0,
        DUMP1,
    };

    void Init(const Config &cfg)
    {
        cfg_       = cfg;
        num_samps_ = 0;
        // Prep the wav header according to config.
        // Certain things (i.e. Size, etc. will have to wait until the finalization of the file, or be updated while streaming).
        wavheader_.ChunkId       = 0x46464952; /** "RIFF" */
        wavheader_.FileFormat    = 0x45564157; /** "WAVE" */
        wavheader_.SubChunk1ID   = 0x20746d66; /** "fmt " */
        wavheader_.SubChunk1Size = 16;         // for PCM
        wavheader_.AudioFormat   = 1;          // linear quant
        wavheader_.NbrChannels   = cfg.channels;
        wavheader_.SampleRate    = static_cast<int>(cfg.samplerate);
        wavheader_.ByteRate      = CalcByteRate();
        wavheader_.BlockAlign    = cfg_.channels * cfg_.bitspersample / 8;
        wavheader_.BitPerSample  = cfg_.bitspersample;
        wavheader_.SubChunk2ID   = 0x61746164; /** "data" */
        /** Also calcs SubChunk2Size */
        wavheader_.FileSize = CalcFileSize();
        // This is calculated as part of the subchunk size
    }

    /** Records the current sample into the working buffer,
     ** queues writes to media when necessary. 
     ** 
     ** \param in should be a pointer to an array of samples */
    void Sample(const float *in)
    {
        for(size_t i = 0; i < cfg_.channels; i++)
        {
            switch(cfg_.bitspersample)
            {
                case 16:
                {
                    int16_t *tp;
                    tp            = (int16_t *)transfer_buff;
                    tp[wptr_ + i] = f2s16(in[i]);
                }
                break;
                case 32: transfer_buff[wptr_ + i] = f2s32(in[i]); break;
                default: break;
            }
        }
        num_samps_++;
        wptr_ += cfg_.channels;
        size_t cap_point
            = cfg_.bitspersample == 16 ? kTransferSamps * 2 : kTransferSamps;
        if(wptr_ == cap_point)
        {
            bstate_ = BufferState::DUMP0;
        }
        if(wptr_ >= cap_point * 2)
        {
            wptr_   = 0;
            bstate_ = BufferState::DUMP1;
        }
    }

    /** Check buffer state and write */
    void Write()
    {
        if(bstate_ != BufferState::IDLE && IsRecording())
        {
            uint32_t     offset;
            unsigned int bw = 0;
            //offset          = bstate_ == BufferState::DUMP0 ? 0 : transfer_size;
            offset  = bstate_ == BufferState::DUMP0 ? 0 : kTransferSamps;
            bstate_ = BufferState::IDLE;
            f_write(&fp_, &transfer_buff[offset], transfer_size, &bw);
        }
    }

    /** Finalizes the writing of the WAV file.
	 ** This overwrites the WAV Header with the correct
	 ** final size, and closes the fptr. */
    void SaveFile()
    {
        unsigned int bw = 0;
        recording_      = false;
        // We _should_ flush whatever's left in the transfer buff
        // TODO: that.
        wavheader_.FileSize = CalcFileSize();
        f_lseek(&fp_, 0);
        f_write(&fp_, &wavheader_, sizeof(wavheader_), &bw);
        f_close(&fp_);
    }

    /** Opens a file for writing. Writes the initial WAV Header, and gets ready for stream-based recording. */
    void OpenFile(const char *name)
    {
        if(f_open(&fp_, name, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
        {
            unsigned int bw = 0;
            if(f_write(&fp_, &wavheader_, sizeof(wavheader_), &bw) == FR_OK)
            {
                recording_ = true;
                num_samps_ = 0;
            }
        }
    }

    inline const bool IsRecording() const { return recording_; }

    inline uint32_t GetLengthSamps() { return num_samps_; }
    inline float    GetLengthSeconds()
    {
        return (float)num_samps_ / (float)cfg_.samplerate;
    }

  private:
    inline uint32_t CalcFileSize()
    {
        wavheader_.SubCHunk2Size
            = num_samps_ * cfg_.channels * cfg_.bitspersample / 8;
        return 36 + wavheader_.SubCHunk2Size;
    }

    inline uint32_t CalcByteRate()
    {
        return cfg_.samplerate * cfg_.channels * cfg_.bitspersample / 8;
    }
    static constexpr int kTransferSamps = transfer_size / sizeof(int32_t);
    WAV_FormatTypeDef    wavheader_;
    uint32_t             num_samps_, wptr_;
    Config               cfg_;
    int32_t              transfer_buff[kTransferSamps * 2];
    BufferState          bstate_;
    bool                 recording_;
    FIL                  fp_;
};
