#pragma once
#ifndef EXTENDED_DELAY_LINE_H
#define EXTENDED_DELAY_LINE_H

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

namespace daisysp
{

/** ExtendedDelayLine implements a simple circular buffer delay line,
    similar to DaisySP's DelayLine, but with added support for reverse
    reading.  It holds an internal buffer of type T with maximum size max_size.
*/
template <typename T, size_t max_size>
class ExtendedDelayLine
{
  public:
    ExtendedDelayLine() { Reset(); }
    ~ExtendedDelayLine() {}

    /** Resets the delay line by clearing the buffer and setting the write
        pointer to 0, and delay to 1 sample.
    */
    void Reset()
    {
        for (size_t i = 0; i < max_size; i++)
        {
            buffer_[i] = T(0);
        }
        write_ptr_ = 0;
        delay_     = 1;
        frac_      = 0.0f;
    }

    /** Sets the delay time in samples.
        If a float is passed, the fractional part is stored for interpolation.
    */
    inline void SetDelay(float delay)
    {
        int32_t int_delay = static_cast<int32_t>(delay);
        frac_ = delay - static_cast<float>(int_delay);
        delay_ = (int_delay < (int32_t)max_size) ? int_delay : max_size - 1;
    }

    /** Writes a sample into the delay line and advances the write pointer.
        (This implementation writes forward, unlike the original DaisySP version.)
    */
    inline void Write(const T sample)
    {
        buffer_[write_ptr_] = sample;
        write_ptr_ = (write_ptr_ + 1) % max_size;
    }

    /** Forward read:
        Reads the delayed sample using linear interpolation.
    */
    inline T Read() const
    {
        // Calculate read pointer for forward mode:
        size_t index1 = (write_ptr_ + max_size - delay_) % max_size;
        size_t index2 = (index1 + 1) % max_size;
        T a = buffer_[index1];
        T b = buffer_[index2];
        return a + (b - a) * frac_;
    }

    /** Reverse read:
        Reads the delayed sample "backwards" from the write pointer.
        The effective read pointer is calculated as (write_ptr_ + delay_) modulo max_size.
        Linear interpolation is used between two adjacent samples.
    */
    inline T ReverseRead() const
    {
        size_t index1 = (write_ptr_ + delay_) % max_size;
        size_t index2 = (index1 + max_size - 1) % max_size;
        T a = buffer_[index1];
        T b = buffer_[index2];
        return a + (b - a) * frac_;
    }

  private:
    float  frac_;       // fractional part of the delay time for interpolation
    size_t write_ptr_;  // current write pointer index
    size_t delay_;      // integer delay in samples
    T      buffer_[max_size];
};

} // namespace daisysp

#endif // EXTENDED_DELAY_LINE_H
