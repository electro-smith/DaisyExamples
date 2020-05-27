// # Daisy Board
// ## Description
// Base Class for any Daisy-Seed based breakout board.
//
// ## Credit
//
// **Author**: shensley
//
// **Date**: May 2020
//
#pragma once
#ifndef DSY_BOARD_H
#define DSY_BOARD_H
#include "daisy_seed.h"

namespace daisy
{
class DaisyBoard
{
  public:

    // Pure Virtual Functions that require implementation in derived class:
    void Init()
    {
        seed.Configure();
        seed.Init();
        InitControls();
    }

    // Typical Functions that can be overridden if necessary.
    void DebounceControls() {}
    void UpdateAnalogControls()
    {
        for(size_t i = 0; i < c_data_.num_controls; i++)
        {
            control_[i].Process();
        }
    }


    // Generic Functions
    void StartAudio(dsy_audio_callback cb) { seed.StartAudio(cb); }
    void DelayMs(uint32_t delay) { dsy_system_delay(delay); }
    void StartAdc() { dsy_adc_start(); }
    void SetAudioBlockSize(size_t size)
    {
        a_data_.block_size    = size;
        a_data_.callback_rate = a_data_.sample_rate / a_data_.block_size;
        seed.SetAudioBlockSize(a_data_.block_size);
    }
    float AudioSampleRate() { return a_data_.sample_rate; }
    float AudioBlockSize() { return a_data_.block_size; }
    float AudioCallbackRate() { return a_data_.callback_rate; }


    float GetControlValue(size_t idx)
    {
        return idx < c_data_.num_controls ? control_[idx].Value() : 0.0f;
    }

    DaisySeed seed;

  protected:
    // Private Virtual Functions that can be overwritten by a derived class
    virtual void InitControls() {}
    // Internal Data
    const static size_t kMaxAnalogControls = 64;
    struct AudioData
    {
        // Needs to be tweaked in Clock settings to be exact.
        float  sample_rate   = 48014.0f;
        size_t block_size    = 48;
        float  callback_rate = sample_rate / block_size;
        size_t num_channels  = 2;
        float  sr_recip      = 1.0f / sample_rate;
    };
    struct ControlData
    {
        size_t num_controls = 0;
    }; 
    AudioData   a_data_;
    ControlData   c_data_;
    AnalogControl control_[kMaxAnalogControls];
};
} // namespace daisy

#endif
