/*
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __FIR_FILTER__H__
#define __FIR_FILTER__H__

#include <vector>
#include <cmath>
#include "filters.hpp" //for M_PI
#include <limits>

/**
 * @brief 
 * 
 * @tparam input_t Type of input array.  Either matrix_i16_t, or matrix_i32_t
 * @tparam acc_t Accumulator size that matches above.  64bit for i16 
 */
template <class input_t, class acc_t>
class fir_filter
{
private:
    /**
     * @brief Set the taps lowpass object
     * 
     * @param cutoff_normalized Should be in the range 0..0.5 (0.5 being the nyquist)
     */
    void set_taps_lowpass(float cutoff_normalized, std::vector<float> &f_taps)
    {
        //http://www.dspguide.com/ch16/2.htm
        float sine_scale = 2 * M_PI * cutoff_normalized;
        // offset is M/2...M is filter order -1. so truncation is desired
        int offset = filter_size / 2;
        for (int i = 0; i < filter_size / 2; i++)
        {
            f_taps[i] = sin(sine_scale * (i - offset)) / (i - offset);
        }
        f_taps[filter_size / 2] = sine_scale;
        for (int i = filter_size / 2 + 1; i < filter_size; i++)
        {
            f_taps[i] = sin(sine_scale * (i - offset)) / (i - offset);
        }
    }

    void apply_hamming(std::vector<float> &f_taps)
    {
        for (int i = 0; i < filter_size; i++)
        {
            f_taps[i] *= 0.54 - 0.46 * cos(2 * M_PI * i / (filter_size - 1));
        }
    }

    void scale_to_unity_gain(std::vector<float> &f_taps)
    {
        //find the sum of taps
        float sum = 0;
        for (auto tap : f_taps)
        {
            sum += tap;
        }
        //scale down
        for (auto &tap : f_taps)
        {
            tap /= sum;
        }
    }

    void convert_lowpass_to_highpass(std::vector<float> &f_taps)
    {
        for (size_t i = 0; i < f_taps.size(); i += 2)
        {
            f_taps[i] *= -1;
        }
    }

public:
    /**
     * @brief Perform in place filtering on the input matrix
     * @param sampling_frequency Sampling freqency of data
     * @param filter_size Number of taps desired (note, filter order +1)
     * @param lowpass_cutoff Lowpass cutoff freqency.  If 0, will be a high pass filter
     * @param highpass_cutoff Highpass cutoff.  If 0, will just be a lowpass.  If both lowpass and higpass, bandpass
     * @param decimation_ratio To downsample, ratio of samples to get rid of.  
     * For example, 4 to go from sample rate of 40k to 10k.  LOWPASS CUTOFF MUST MATCH THIS
     * If you don't filter the high frequencies, they WILL alias into the passband
     * So in the above example, you would want to cutoff at 5K (so you have some buffer)
     */
    fir_filter(
        float sampling_frequency,
        uint8_t filter_size,
        float lowpass_cutoff,
        float highpass_cutoff = 0,
        int decimation_ratio = 1) :  taps(filter_size) , history(filter_size, 0)
    {
        this->filter_size = filter_size;
        std::vector<float> f_taps(filter_size, 0);
        if( highpass_cutoff == 0 && lowpass_cutoff == 0 ) 
        {
            ei_printf("You must choose either a lowpass or highpass cutoff");
            return; // return a filter that will return zeros always
        }
        if (highpass_cutoff == 0)
        {
            // use normalized frequency
            set_taps_lowpass(lowpass_cutoff / sampling_frequency, f_taps);
        }
        if (lowpass_cutoff == 0)
        {
            //for highpass, we'll just design a lowpass filter, then invert its spectrum
            set_taps_lowpass(highpass_cutoff / sampling_frequency, f_taps);
        }
        //todo bandpass
        apply_hamming(f_taps);
        //scale to unity gain in passband (this prevents overflow)
        scale_to_unity_gain(f_taps);
        // aka if highpass filter
        if (lowpass_cutoff == 0)
        {
            //now invert the spectrum
            convert_lowpass_to_highpass(f_taps);
        }
        // scale and write into fixed point taps
        for (int i = 0; i < filter_size; i++)
        {
            taps[i] = f_taps[i] * 32767;
        }
    }

/**
 * @brief Apply the filter to the input data.  You can do this blockwise, as the object preserves memory of old samples
 * Call reset if there's a gap in the data
 * 
 * @param src Source array
 * @param dest Output array (can be the same as source for in place)
 * @param size Number of samples to process
 */
    void apply_filter(
        const input_t *src,
        input_t *dest,
        size_t size)
    {
        for (size_t i = 0; i < size; i++)
        {
            history[write_index] = src[i];
            int read_index = write_index;
            //minus one b/c of the sign bit
            int shift = (sizeof(input_t) * 8) - 1;
            //stuff a 1 into one less than we're going to shift to effectively round
            //this is essentially resetting the accumulator back to zero otherwise
            acc_t accumulator = 1 << (shift - 1);
            for (auto tap : taps)
            {
                accumulator += static_cast<acc_t>(tap) * history[read_index];
                //wrap the read index
                read_index = read_index == 0 ? filter_size - 1 : read_index - 1;
            }
            //wrap the write index
            write_index++;
            if (write_index == filter_size)
            {
                write_index = 0;
            }

            accumulator >>= shift;
            //saturate if overflow
            if (accumulator > std::numeric_limits<input_t>::max())
            {
                dest[i] = std::numeric_limits<input_t>::max();
            }
            else if (accumulator < std::numeric_limits<input_t>::min())
            {
                dest[i] = std::numeric_limits<input_t>::min();
            }
            else
            {
                dest[i] = accumulator;
            }
        }
    }

    /**
     * @brief Reset the filter (when changing rows for instance, for a new signal)
     * This simply clears the filter history
     * 
     */
    void reset()
    {
        std::fill(history.begin(), history.end(), 0);
    }

private:
    std::vector<input_t> taps;
    std::vector<input_t> history;
    int write_index = 0;
    int filter_size;

    friend class AccelerometerQuantizedTestCase;

};
#endif  //!__FIR_FILTER__H__
