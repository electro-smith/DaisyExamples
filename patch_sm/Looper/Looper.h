#pragma once
#ifndef DSY_LOOPER_H
#define DSY_LOOPER_H
#include <stdlib.h>
#include <stdint.h>

namespace daisysp
{
/** Simple Looper. 
October 2021
By: beserge

TODO:
- Add fractional speed control
- Allow for reverse playback
- Allow user to read / write wherever they like in the buffer
*/
template <typename T, size_t max_size>
class Looper
{
  public:
    Looper(){};
    ~Looper(){};

    /** Initialize the looper
        \param sr - sample rate of the audio engine being run, and the frequency that the Process function will be called.
    */
    void Init(float sr)
    {
        Reset();

        looplen_     = max_size;
        max_size_ms_ = max_size / sr * 1000.f;

        idx_ = 0;
        sr_  = sr;

        rec_arm_ = false;
    }

    /** Clear the loop buffer and set the pointer back to the start */
    void Reset()
    {
        idx_ = 0;
        for(size_t i = 0; i < max_size; i++)
        {
            buff[i] = 0.f;
        }
    }

    /** Read the next sample from the buffer and advance the buffer index */
    T Read()
    {
        idx_++;
        if(idx_ >= looplen_ || idx_ >= max_size)
        {
            if(rec_arm_)
            {
                EndLoop();
            }

            idx_ = 0;
        }
        return buff[idx_];
    }

    /** Write a sample into the current buffer index
        \param input Sample to write
    */
    void Write(T input) { buff[idx_] = input; }

    /** Read and write in one function. Obeys the Start/EndLoop functions
        \param input Sample to write (only written if we're creating a new loop via StartLoop)
    */
    float Process(float input)
    {
        if(rec_arm_)
        {
            Write(input);
        }
        return Read();
    }

    /** Set the loop length manually
        \param length Loop length in ms
    */
    void SetLoopLengthMs(float length)
    {
        length   = fclamp(length, 1.f, max_size_ms_);
        looplen_ = length * .001f * sr_;
    }

    /** Start recording a new loop. Calling this means Process() will start recording inputs. */
    void StartLoop()
    {
        Reset();
        rec_arm_ = true;
        looplen_ = max_size;
    }

    /** End the loop, setting the length. Calling this means Process() will stop recording inputs. */
    void EndLoop()
    {
        rec_arm_ = false;
        looplen_ = std::min(max_size, idx_);
    }

    /** Automatically call either StartLoop() or EndLoop() as needed */
    void ToggleLoop()
    {
        if(rec_arm_)
        {
            EndLoop();
        }
        else
        {
            StartLoop();
        }
    }

  private:
    T      buff[max_size];
    size_t looplen_;
    float  max_size_ms_;
    size_t idx_;
    float  sr_;
    bool   _;
    bool   rec_arm_;
};
} // namespace daisysp
#endif