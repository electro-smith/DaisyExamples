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

#ifndef _EIDSP_SPEECHPY_FEATURE_H_
#define _EIDSP_SPEECHPY_FEATURE_H_

#include <stdint.h>
#include "../../porting/ei_classifier_porting.h"
#include "../ei_utils.h"
#include "functions.hpp"
#include "processing.hpp"
#include "../memory.hpp"
#include "../returntypes.hpp"
#include "../ei_vector.h"

namespace ei {
namespace speechpy {

class feature {
public:
    /**
     * Compute the Mel-filterbanks. Each filter will be stored in one rows.
     * The columns correspond to fft bins.
     *
     * @param filterbanks Matrix of size num_filter * coefficients
     * @param num_filter the number of filters in the filterbank
     * @param coefficients (fftpoints//2 + 1)
     * @param sampling_freq  the samplerate of the signal we are working
     *                       with. It affects mel spacing.
     * @param low_freq lowest band edge of mel filters, default 0 Hz
     * @param high_freq highest band edge of mel filters, default samplerate / 2
     * @param output_transposed If set to true this will transpose the matrix (memory efficient).
     *                          This is more efficient than calling this function and then transposing
     *                          as the latter requires the filterbank to be allocated twice (for a short while).
     * @returns EIDSP_OK if OK
     */
    static int filterbanks(
#if EIDSP_QUANTIZE_FILTERBANK
        quantized_matrix_t *filterbanks,
#else
        matrix_t *filterbanks,
#endif
        uint16_t num_filter, int coefficients, uint32_t sampling_freq,
        uint32_t low_freq, uint32_t high_freq,
        bool output_transposed = false
        )
    {
        const size_t mels_mem_size = (num_filter + 2) * sizeof(float);
        const size_t hertz_mem_size = (num_filter + 2) * sizeof(float);
        const size_t freq_index_mem_size = (num_filter + 2) * sizeof(int);

        float *mels = (float*)ei_dsp_malloc(mels_mem_size);
        if (!mels) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        if (filterbanks->rows != num_filter || filterbanks->cols != static_cast<uint32_t>(coefficients)) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

#if EIDSP_QUANTIZE_FILTERBANK
        memset(filterbanks->buffer, 0, filterbanks->rows * filterbanks->cols * sizeof(uint8_t));
#else
        memset(filterbanks->buffer, 0, filterbanks->rows * filterbanks->cols * sizeof(float));
#endif

        // Computing the Mel filterbank
        // converting the upper and lower frequencies to Mels.
        // num_filter + 2 is because for num_filter filterbanks we need
        // num_filter+2 point.
        numpy::linspace(
            functions::frequency_to_mel(static_cast<float>(low_freq)),
            functions::frequency_to_mel(static_cast<float>(high_freq)),
            num_filter + 2,
            mels);

        // we should convert Mels back to Hertz because the start and end-points
        // should be at the desired frequencies.
        float *hertz = (float*)ei_dsp_malloc(hertz_mem_size);
        if (!hertz) {
            ei_dsp_free(mels, mels_mem_size);
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }
        for (uint16_t ix = 0; ix < num_filter + 2; ix++) {
            hertz[ix] = functions::mel_to_frequency(mels[ix]);
            if (hertz[ix] < low_freq) {
                hertz[ix] = low_freq;
            }
            if (hertz[ix] > high_freq) {
                hertz[ix] = high_freq;
            }

            // here is a really annoying bug in Speechpy which calculates the frequency index wrong for the last bucket
            // the last 'hertz' value is not 8,000 (with sampling rate 16,000) but 7,999.999999
            // thus calculating the bucket to 64, not 65.
            // we're adjusting this here a tiny bit to ensure we have the same result
            if (ix == num_filter + 2 - 1) {
                hertz[ix] -= 0.001;
            }
        }
        ei_dsp_free(mels, mels_mem_size);

        // The frequency resolution required to put filters at the
        // exact points calculated above should be extracted.
        //  So we should round those frequencies to the closest FFT bin.
        int *freq_index = (int*)ei_dsp_malloc(freq_index_mem_size);
        if (!freq_index) {
            ei_dsp_free(hertz, hertz_mem_size);
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }
        for (uint16_t ix = 0; ix < num_filter + 2; ix++) {
            freq_index[ix] = static_cast<int>(floor((coefficients + 1) * hertz[ix] / sampling_freq));
        }
        ei_dsp_free(hertz, hertz_mem_size);

        for (size_t i = 0; i < num_filter; i++) {
            int left = freq_index[i];
            int middle = freq_index[i + 1];
            int right = freq_index[i + 2];

            EI_DSP_MATRIX(z, 1, (right - left + 1));
            if (!z.buffer) {
                ei_dsp_free(freq_index, freq_index_mem_size);
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }
            numpy::linspace(left, right, (right - left + 1), z.buffer);
            functions::triangle(z.buffer, (right - left + 1), left, middle, right);

            // so... z now contains some values that we need to overwrite in the filterbank
            for (int zx = 0; zx < (right - left + 1); zx++) {
                size_t index = (i * filterbanks->cols) + (left + zx);

                if (output_transposed) {
                    index = ((left + zx) * filterbanks->rows) + i;
                }

#if EIDSP_QUANTIZE_FILTERBANK
                filterbanks->buffer[index] = numpy::quantize_zero_one(z.buffer[zx]);
#else
                filterbanks->buffer[index] = z.buffer[zx];
#endif
            }
        }

        if (output_transposed) {
            uint16_t r = filterbanks->rows;
            filterbanks->rows = filterbanks->cols;
            filterbanks->cols = r;
        }

        ei_dsp_free(freq_index, freq_index_mem_size);

        return EIDSP_OK;
    }

    /**
     * @brief Get the fft bin index from hertz
     *
     * @param fft_size Size of fft
     * @param hertz Desired hertz
     * @param sampling_freq In Hz
     * @return int the index of the bin closest to the hertz
     */
    static int get_fft_bin_from_hertz(uint16_t fft_size, float hertz, uint32_t sampling_freq)
    {
        return static_cast<int>(floor((fft_size + 1) * hertz / sampling_freq));
    }

    /**
     * Compute Mel-filterbank energy features from an audio signal.
     * @param out_features Use `calculate_mfe_buffer_size` to allocate the right matrix.
     * @param out_energies A matrix in the form of Mx1 where M is the rows from `calculate_mfe_buffer_size`
     * @param signal: audio signal structure with functions to retrieve data from a signal
     * @param sampling_frequency (int): the sampling frequency of the signal
     *     we are working with.
     * @param frame_length (float): the length of each frame in seconds.
     *     Default is 0.020s
     * @param frame_stride (float): the step between successive frames in seconds.
     *     Default is 0.02s (means no overlap)
     * @param num_filters (int): the number of filters in the filterbank,
     *     default 40.
     * @param fft_length (int): number of FFT points. Default is 512.
     * @param low_frequency (int): lowest band edge of mel filters.
     *     In Hz, default is 0.
     * @param high_frequency (int): highest band edge of mel filters.
     *     In Hz, default is samplerate/2
     * @EIDSP_OK if OK
     */
    static int mfe(matrix_t *out_features, matrix_t *out_energies,
        signal_t *signal,
        uint32_t sampling_frequency,
        float frame_length, float frame_stride, uint16_t num_filters,
        uint16_t fft_length, uint32_t low_frequency, uint32_t high_frequency,
        uint16_t version
        )
    {
        int ret = 0;

        if (high_frequency == 0) {
            high_frequency = sampling_frequency / 2;
        }

        if (version<4) {
            if (low_frequency == 0) {
                low_frequency = 300;
            }
        }

        stack_frames_info_t stack_frame_info = { 0 };
        stack_frame_info.signal = signal;

        ret = processing::stack_frames(
            &stack_frame_info,
            sampling_frequency,
            frame_length,
            frame_stride,
            false,
            version
        );
        if (ret != 0) {
            EIDSP_ERR(ret);
        }

        if (stack_frame_info.frame_ixs.size() != out_features->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (num_filters != out_features->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (out_energies) {
            if (stack_frame_info.frame_ixs.size() != out_energies->rows || out_energies->cols != 1) {
                EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
            }
        }

        for (uint32_t i = 0; i < out_features->rows * out_features->cols; i++) {
            *(out_features->buffer + i) = 0;
        }

        const size_t power_spectrum_frame_size = (fft_length / 2 + 1);
        // Computing the Mel filterbank
        // converting the upper and lower frequencies to Mels.
        // num_filter + 2 is because for num_filter filterbanks we need
        // num_filter+2 point.
        float *mels;
        const int MELS_SIZE = num_filters + 2;
        mels = (float*)ei_calloc(MELS_SIZE, sizeof(float));
        EI_ERR_AND_RETURN_ON_NULL(mels, EIDSP_OUT_OF_MEM);
        ei_unique_ptr_t __ptr__(mels,ei_free);
        uint16_t* bins = reinterpret_cast<uint16_t*>(mels); // alias the mels array so we can reuse the space

        numpy::linspace(
            functions::frequency_to_mel(static_cast<float>(low_frequency)),
            functions::frequency_to_mel(static_cast<float>(high_frequency)),
            num_filters + 2,
            mels);

        uint16_t max_bin = version >= 4 ? fft_length : power_spectrum_frame_size; // preserve a bug in v<4
        // go to -1 size b/c special handling, see after
        for (uint16_t ix = 0; ix < MELS_SIZE-1; ix++) {
            mels[ix] = functions::mel_to_frequency(mels[ix]);
            if (mels[ix] < low_frequency) {
                mels[ix] = low_frequency;
            }
            if (mels[ix] > high_frequency) {
                mels[ix] = high_frequency;
            }
            bins[ix] = get_fft_bin_from_hertz(max_bin, mels[ix], sampling_frequency);
        }

        // here is a really annoying bug in Speechpy which calculates the frequency index wrong for the last bucket
        // the last 'hertz' value is not 8,000 (with sampling rate 16,000) but 7,999.999999
        // thus calculating the bucket to 64, not 65.
        // we're adjusting this here a tiny bit to ensure we have the same result
        mels[MELS_SIZE-1] = functions::mel_to_frequency(mels[MELS_SIZE-1]);
        if (mels[MELS_SIZE-1] > high_frequency) {
            mels[MELS_SIZE-1] = high_frequency;
        }
        mels[MELS_SIZE-1] -= 0.001;
        bins[MELS_SIZE-1] = get_fft_bin_from_hertz(max_bin, mels[MELS_SIZE-1], sampling_frequency);

        EI_DSP_MATRIX(power_spectrum_frame, 1, power_spectrum_frame_size);
        if (!power_spectrum_frame.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        // get signal data from the audio file
        EI_DSP_MATRIX(signal_frame, 1, stack_frame_info.frame_length);

        for (size_t ix = 0; ix < stack_frame_info.frame_ixs.size(); ix++) {
            // don't read outside of the audio buffer... we'll automatically zero pad then
            size_t signal_offset = stack_frame_info.frame_ixs.at(ix);
            size_t signal_length = stack_frame_info.frame_length;
            if (signal_offset + signal_length > stack_frame_info.signal->total_length) {
                signal_length = signal_length -
                    (stack_frame_info.signal->total_length - (signal_offset + signal_length));
            }

            ret = stack_frame_info.signal->get_data(
                signal_offset,
                signal_length,
                signal_frame.buffer
            );
            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            ret = numpy::power_spectrum(
                signal_frame.buffer,
                stack_frame_info.frame_length,
                power_spectrum_frame.buffer,
                power_spectrum_frame_size,
                fft_length
            );

            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            float energy = numpy::sum(power_spectrum_frame.buffer, power_spectrum_frame_size);
            if (energy == 0) {
                energy = 1e-10;
            }

            if (out_energies) {
                out_energies->buffer[ix] = energy;
            }

            auto row_ptr = out_features->get_row_ptr(ix);
            for (size_t i = 0; i < num_filters; i++) {
                size_t left = bins[i];
                size_t middle = bins[i+1];
                size_t right = bins[i+2];

                assert(right < power_spectrum_frame_size);
                // now we have weights and locations to move from fft to mel sgram
                // both left and right become zero weights, so skip them

                // middle always has weight of 1.0
                // since we skip left and right, if left = middle we need to handle that
                row_ptr[i] = power_spectrum_frame.buffer[middle];

                for (size_t bin = left+1; bin < right; bin++) {
                    if (bin < middle) {
                        row_ptr[i] +=
                            ((static_cast<float>(bin) - left) / (middle - left)) * // weight *
                            power_spectrum_frame.buffer[bin];
                    }
                    // intentionally skip middle, handled above
                    if (bin > middle) {
                        row_ptr[i] +=
                            ((right - static_cast<float>(bin)) / (right - middle)) * // weight *
                            power_spectrum_frame.buffer[bin];
                    }
                }
            }

            if (ret != 0) {
                EIDSP_ERR(ret);
            }
        }

        numpy::zero_handling(out_features);

        return EIDSP_OK;
    }

    /**
     * Compute Mel-filterbank energy features from an audio signal.
     * @param out_features Use `calculate_mfe_buffer_size` to allocate the right matrix.
     * @param out_energies A matrix in the form of Mx1 where M is the rows from `calculate_mfe_buffer_size`
     * @param signal: audio signal structure with functions to retrieve data from a signal
     * @param sampling_frequency (int): the sampling frequency of the signal
     *     we are working with.
     * @param frame_length (float): the length of each frame in seconds.
     *     Default is 0.020s
     * @param frame_stride (float): the step between successive frames in seconds.
     *     Default is 0.02s (means no overlap)
     * @param num_filters (int): the number of filters in the filterbank,
     *     default 40.
     * @param fft_length (int): number of FFT points. Default is 512.
     * @param low_frequency (int): lowest band edge of mel filters.
     *     In Hz, default is 0.
     * @param high_frequency (int): highest band edge of mel filters.
     *     In Hz, default is samplerate/2
     * @EIDSP_OK if OK
     */
    static int mfe_v3(matrix_t *out_features, matrix_t *out_energies,
        signal_t *signal,
        uint32_t sampling_frequency,
        float frame_length, float frame_stride, uint16_t num_filters,
        uint16_t fft_length, uint32_t low_frequency, uint32_t high_frequency,
        uint16_t version
        )
    {
        int ret = 0;

        if (high_frequency == 0) {
            high_frequency = sampling_frequency / 2;
        }

        if (low_frequency == 0) {
            low_frequency = 300;
        }

        stack_frames_info_t stack_frame_info = { 0 };
        stack_frame_info.signal = signal;

        ret = processing::stack_frames(
            &stack_frame_info,
            sampling_frequency,
            frame_length,
            frame_stride,
            false,
            version
        );
        if (ret != 0) {
            EIDSP_ERR(ret);
        }

        if (stack_frame_info.frame_ixs.size() != out_features->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (num_filters != out_features->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (out_energies) {
            if (stack_frame_info.frame_ixs.size() != out_energies->rows || out_energies->cols != 1) {
                EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
            }
        }

        for (uint32_t i = 0; i < out_features->rows * out_features->cols; i++) {
            *(out_features->buffer + i) = 0;
        }

        uint16_t coefficients = fft_length / 2 + 1;

        // calculate the filterbanks first... preferably I would want to do the matrix multiplications
        // whenever they happen, but OK...
#if EIDSP_QUANTIZE_FILTERBANK
        EI_DSP_QUANTIZED_MATRIX(filterbanks, num_filters, coefficients, &numpy::dequantize_zero_one);
#else
        EI_DSP_MATRIX(filterbanks, num_filters, coefficients);
#endif
        if (!filterbanks.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        ret = feature::filterbanks(
            &filterbanks, num_filters, coefficients, sampling_frequency, low_frequency, high_frequency, true);
        if (ret != 0) {
            EIDSP_ERR(ret);
        }
        for (size_t ix = 0; ix < stack_frame_info.frame_ixs.size(); ix++) {
            size_t power_spectrum_frame_size = (fft_length / 2 + 1);

            EI_DSP_MATRIX(power_spectrum_frame, 1, power_spectrum_frame_size);
            if (!power_spectrum_frame.buffer) {
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }

            // get signal data from the audio file
            EI_DSP_MATRIX(signal_frame, 1, stack_frame_info.frame_length);

            // don't read outside of the audio buffer... we'll automatically zero pad then
            size_t signal_offset = stack_frame_info.frame_ixs.at(ix);
            size_t signal_length = stack_frame_info.frame_length;
            if (signal_offset + signal_length > stack_frame_info.signal->total_length) {
                signal_length = signal_length -
                    (stack_frame_info.signal->total_length - (signal_offset + signal_length));
            }

            ret = stack_frame_info.signal->get_data(
                signal_offset,
                signal_length,
                signal_frame.buffer
            );
            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            ret = numpy::power_spectrum(
                signal_frame.buffer,
                stack_frame_info.frame_length,
                power_spectrum_frame.buffer,
                power_spectrum_frame_size,
                fft_length
            );

            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            float energy = numpy::sum(power_spectrum_frame.buffer, power_spectrum_frame_size);
            if (energy == 0) {
                energy = 1e-10;
            }

            if (out_energies) {
                out_energies->buffer[ix] = energy;
            }

            // calculate the out_features directly here
            ret = numpy::dot_by_row(
                ix,
                power_spectrum_frame.buffer,
                power_spectrum_frame_size,
                &filterbanks,
                out_features
            );

            if (ret != 0) {
                EIDSP_ERR(ret);
            }
        }

        numpy::zero_handling(out_features);

        return EIDSP_OK;
    }

    /**
     * Compute spectrogram from a sensor signal.
     * @param out_features Use `calculate_mfe_buffer_size` to allocate the right matrix.
     * @param signal: audio signal structure with functions to retrieve data from a signal
     * @param sampling_frequency (int): the sampling frequency of the signal
     *     we are working with.
     * @param frame_length (float): the length of each frame in seconds.
     *     Default is 0.020s
     * @param frame_stride (float): the step between successive frames in seconds.
     *     Default is 0.02s (means no overlap)
     * @param fft_length (int): number of FFT points. Default is 512.
     * @EIDSP_OK if OK
     */
    static int spectrogram(matrix_t *out_features,
        signal_t *signal, float sampling_frequency,
        float frame_length, float frame_stride, uint16_t fft_length,
        uint16_t version
        )
    {
        int ret = 0;

        stack_frames_info_t stack_frame_info = { 0 };
        stack_frame_info.signal = signal;

        ret = processing::stack_frames(
            &stack_frame_info,
            sampling_frequency,
            frame_length,
            frame_stride,
            false,
            version
        );
        if (ret != 0) {
            EIDSP_ERR(ret);
        }

        if (stack_frame_info.frame_ixs.size() != out_features->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        uint16_t coefficients = fft_length / 2 + 1;

        if (coefficients != out_features->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (uint32_t i = 0; i < out_features->rows * out_features->cols; i++) {
            *(out_features->buffer + i) = 0;
        }

        for (size_t ix = 0; ix < stack_frame_info.frame_ixs.size(); ix++) {
            // get signal data from the audio file
            EI_DSP_MATRIX(signal_frame, 1, stack_frame_info.frame_length);

            // don't read outside of the audio buffer... we'll automatically zero pad then
            size_t signal_offset = stack_frame_info.frame_ixs.at(ix);
            size_t signal_length = stack_frame_info.frame_length;
            if (signal_offset + signal_length > stack_frame_info.signal->total_length) {
                signal_length = signal_length -
                    (stack_frame_info.signal->total_length - (signal_offset + signal_length));
            }

            ret = stack_frame_info.signal->get_data(
                signal_offset,
                signal_length,
                signal_frame.buffer
            );
            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            // normalize data (only when version is above 3)
            if (version >= 3) {
                // it might be that everything is already normalized here...
                bool all_between_min_1_and_1 = true;
                for (size_t ix = 0; ix < signal_frame.rows * signal_frame.cols; ix++) {
                    if (signal_frame.buffer[ix] < -1.0f || signal_frame.buffer[ix] > 1.0f) {
                        all_between_min_1_and_1 = false;
                        break;
                    }
                }

                if (!all_between_min_1_and_1) {
                    ret = numpy::scale(&signal_frame, 1.0f / 32768.0f);
                    if (ret != 0) {
                        EIDSP_ERR(ret);
                    }
                }
            }

            ret = numpy::power_spectrum(
                signal_frame.buffer,
                stack_frame_info.frame_length,
                out_features->buffer + (ix * coefficients),
                coefficients,
                fft_length
            );

            if (ret != 0) {
                EIDSP_ERR(ret);
            }
        }

        numpy::zero_handling(out_features);

        return EIDSP_OK;
    }

    /**
     * Calculate the buffer size for MFE
     * @param signal_length: Length of the signal.
     * @param sampling_frequency (int): The sampling frequency of the signal.
     * @param frame_length (float): The length of the frame in second.
     * @param frame_stride (float): The stride between frames.
     * @param num_filters
     */
    static matrix_size_t calculate_mfe_buffer_size(
        size_t signal_length,
        uint32_t sampling_frequency,
        float frame_length, float frame_stride, uint16_t num_filters,
        uint16_t version)
    {
        int32_t rows = processing::calculate_no_of_stack_frames(
            signal_length,
            sampling_frequency,
            frame_length,
            frame_stride,
            false,
            version);
        int32_t cols = num_filters;

        matrix_size_t size_matrix;
        size_matrix.rows = (uint32_t)rows;
        size_matrix.cols = (uint32_t)cols;
        return size_matrix;
    }

    /**
     * Compute MFCC features from an audio signal.
     * @param out_features Use `calculate_mfcc_buffer_size` to allocate the right matrix.
     * @param signal: audio signal structure from which to compute features.
     *     has functions to retrieve data from a signal lazily.
     * @param sampling_frequency (int): the sampling frequency of the signal
     *     we are working with.
     * @param frame_length (float): the length of each frame in seconds.
     *     Default is 0.020s
     * @param frame_stride (float): the step between successive frames in seconds.
     *     Default is 0.01s (means no overlap)
     * @param num_cepstral (int): Number of cepstral coefficients.
     * @param num_filters (int): the number of filters in the filterbank,
     *     default 40.
     * @param fft_length (int): number of FFT points. Default is 512.
     * @param low_frequency (int): lowest band edge of mel filters.
     *     In Hz, default is 0.
     * @param high_frequency (int): highest band edge of mel filters.
     *     In Hz, default is samplerate/2
     * @param dc_elimination Whether the first dc component should
     *     be eliminated or not.
     * @returns 0 if OK
     */
    static int mfcc(matrix_t *out_features, signal_t *signal,
        uint32_t sampling_frequency, float frame_length, float frame_stride,
        uint8_t num_cepstral, uint16_t num_filters, uint16_t fft_length,
        uint32_t low_frequency, uint32_t high_frequency, bool dc_elimination,
        uint16_t version)
    {
        if (out_features->cols != num_cepstral) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        matrix_size_t mfe_matrix_size =
            calculate_mfe_buffer_size(
                signal->total_length,
                sampling_frequency,
                frame_length,
                frame_stride,
                num_filters,
                version);

        if (out_features->rows != mfe_matrix_size.rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        int ret = EIDSP_OK;

        // allocate some memory for the MFE result
        EI_DSP_MATRIX(features_matrix, mfe_matrix_size.rows, mfe_matrix_size.cols);
        if (!features_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        EI_DSP_MATRIX(energy_matrix, mfe_matrix_size.rows, 1);
        if (!energy_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        ret = mfe(&features_matrix, &energy_matrix, signal,
            sampling_frequency, frame_length, frame_stride, num_filters, fft_length,
            low_frequency, high_frequency, version);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        // ok... now we need to calculate the MFCC from this...
        // first do log() over all features...
        ret = numpy::log(&features_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        // now do DST type 2
        ret = numpy::dct2(&features_matrix, DCT_NORMALIZATION_ORTHO);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        // replace first cepstral coefficient with log of frame energy for DC elimination
        if (dc_elimination) {
            for (size_t row = 0; row < features_matrix.rows; row++) {
                features_matrix.buffer[row * features_matrix.cols] = numpy::log(energy_matrix.buffer[row]);
            }
        }

        // copy to the output...
        for (size_t row = 0; row < features_matrix.rows; row++) {
            for(int i = 0; i < num_cepstral; i++) {
                *(out_features->buffer + (num_cepstral * row) + i) = *(features_matrix.buffer + (features_matrix.cols * row) + i);
            }
        }

        return EIDSP_OK;
    }

    /**
     * Calculate the buffer size for MFCC
     * @param signal_length: Length of the signal.
     * @param sampling_frequency (int): The sampling frequency of the signal.
     * @param frame_length (float): The length of the frame in second.
     * @param frame_stride (float): The stride between frames.
     * @param num_cepstral
     */
    static matrix_size_t calculate_mfcc_buffer_size(
        size_t signal_length,
        uint32_t sampling_frequency,
        float frame_length, float frame_stride, uint16_t num_cepstral,
        uint16_t version)
    {
        int32_t rows = processing::calculate_no_of_stack_frames(
            signal_length,
            sampling_frequency,
            frame_length,
            frame_stride,
            false,
            version);
        int32_t cols = num_cepstral;

        matrix_size_t size_matrix;
        size_matrix.rows = (uint32_t)rows;
        size_matrix.cols = (uint32_t)cols;
        return size_matrix;
    }
};

} // namespace speechpy
} // namespace ei

#endif // _EIDSP_SPEECHPY_FEATURE_H_
