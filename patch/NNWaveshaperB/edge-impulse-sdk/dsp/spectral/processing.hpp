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

#ifndef _EIDSP_SPECTRAL_PROCESSING_H_
#define _EIDSP_SPECTRAL_PROCESSING_H_

#include "edge-impulse-sdk/dsp/ei_vector.h"
#include <algorithm>
#include "../numpy.hpp"
#include "filters.hpp"

namespace ei {
namespace spectral {

namespace processing {
    /**
     * Scaling on the signal.
     * @param signal: The input signal.
     * @param scaling (int): To scale by which factor (e.g. 10 here means multiply by 10)
     */
    class scale {
public:
        scale(ei_signal_t *signal, float scaling = 1.0f)
            : _signal(signal), _scaling(scaling)
        {
        }

        /**
         * Get scaled data from the underlying sensor buffer...
         * This retrieves data from the signal then scales it.
         * @param offset Offset in the audio signal
         * @param length Length of the audio signal
         */
        int get_data(size_t offset, size_t length, float *out_buffer) {
            if (offset + length > _signal->total_length) {
                EIDSP_ERR(EIDSP_OUT_OF_BOUNDS);
            }

            int ret = _signal->get_data(offset, length, out_buffer);
            if (ret != 0) {
                EIDSP_ERR(ret);
            }

            EI_DSP_MATRIX_B(temp, 1, length, out_buffer);
            return numpy::scale(&temp, _scaling);
        }

private:
        ei_signal_t *_signal;
        float _scaling;
    };
}

namespace processing {
    typedef struct {
        float freq;
        float amplitude;
    } freq_peak_t;

    typedef struct {
        EIDSP_i32 freq;
        EIDSP_i32 amplitude;
    } freq_peak_i32_t;

    /**
     * Scale a the signal. This modifies the signal in place!
     * For memory consumption reasons you **probably** want the scaling class,
     * which lazily loads the signal in.
     * @param signal (array): The input signal.
     * @param signal_size: The length of the signal.
     * @param scale (float): The scaling factor (multiplies by this number).
     * @returns 0 when successful
     */
    __attribute__((unused)) static int scale(float *signal, size_t signal_size, float scale = 1)
    {
        EI_DSP_MATRIX_B(temp, 1, signal_size, signal);
        return numpy::scale(&temp, scale);
    }

    /**
     * Filter data along one-dimension with an IIR or FIR filter using
     * Butterworth digital and analog filter design.
     * This modifies the matrix in-place (per row)
     * @param matrix Input matrix
     * @param sampling_freq Sampling frequency
     * @param filter_cutoff
     * @param filter_order
     * @returns 0 when successful
     */
    static int butterworth_lowpass_filter(
        matrix_t *matrix,
        float sampling_frequency,
        float filter_cutoff,
        uint8_t filter_order)
    {
        for (size_t row = 0; row < matrix->rows; row++) {
            filters::butterworth_lowpass(
                filter_order,
                sampling_frequency,
                filter_cutoff,
                matrix->buffer + (row * matrix->cols),
                matrix->buffer + (row * matrix->cols),
                matrix->cols);
        }

        return EIDSP_OK;
    }

    /**
     * Filter data along one-dimension with an IIR or FIR filter using
     * Butterworth digital and analog filter design.
     * This modifies the matrix in-place (per row)
     * @param matrix Input matrix
     * @param sampling_freq Sampling frequency
     * @param filter_cutoff
     * @param filter_order
     * @returns 0 when successful
     */
    static int butterworth_highpass_filter(
        matrix_t *matrix,
        float sampling_frequency,
        float filter_cutoff,
        uint8_t filter_order)
    {
        for (size_t row = 0; row < matrix->rows; row++) {
            filters::butterworth_highpass(
                filter_order,
                sampling_frequency,
                filter_cutoff,
                matrix->buffer + (row * matrix->cols),
                matrix->buffer + (row * matrix->cols),
                matrix->cols);
        }

        return EIDSP_OK;
    }

    /**
     * Find peaks in a FFT spectrum
     * threshold is *normalized* threshold
     * (I'm still not completely sure if this matches my Python code but it looks OK)
     * @param input_matrix Matrix with FFT data of size 1xM
     * @param output_matrix Output matrix with N rows for every peak you want to find.
     * @param threshold Minimum threshold
     * @param peaks_found Out parameter with the number of peaks found
     * @returns 0 if OK
     */
    static int find_peak_indexes(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        float threshold,
        uint16_t *peaks_found)
    {
        if (input_matrix->rows != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        uint16_t out_ix = 0;
        size_t in_size = input_matrix->cols;
        float *in = input_matrix->buffer;
        size_t out_size = output_matrix->rows;
        float *out = output_matrix->buffer;

        // for normalized threshold calculation
        float min = FLT_MAX, max = 0.0f;
        for (size_t ix = 0; ix < in_size - 1; ix++) {
            if (in[ix] < min) {
                min = in[ix];
            }
            if (in[ix] > max) {
                max = in[ix];
            }
        }


        float prev = in[0];

        // so....
        for (size_t ix = 1; ix < in_size - 1; ix++) {
            // first make sure it's actually a peak...
            if (in[ix] > prev && in[ix] > in[ix+1]) {
                // then make sure the threshold is met (on both?)
                float height = (in[ix] - prev) + (in[ix] - in[ix + 1]);
                // printf("%d inx: %f height: %f threshold: %f\r\n", ix, in[ix], height, threshold);
                if (height > threshold) {
                    out[out_ix] = ix;
                    out_ix++;
                    if (out_ix == out_size) break;
                }
            }

            prev = in[ix];
        }

        *peaks_found = out_ix;

        return EIDSP_OK;
    }

    /**
     * Find peaks in FFT
     * @param fft_matrix Matrix of FFT numbers (1xN)
     * @param output_matrix Matrix for the output (Mx2), one row per output you want and two colums per row
     * @param sampling_freq How often we sample (in Hz)
     * @param threshold Minimum threshold (default: 0.1)
     * @returns
     */
    static int find_fft_peaks(
        matrix_t *fft_matrix,
        matrix_t *output_matrix,
        float sampling_freq,
        float threshold,
        uint16_t fft_length)
    {
        if (fft_matrix->rows != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->cols != 2) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->rows == 0) {
            return EIDSP_OK;
        }

        int ret;

        int N = static_cast<int>(fft_length);
        float T = 1.0f / sampling_freq;

        EI_DSP_MATRIX(freq_space, 1, fft_matrix->cols);
        ret = numpy::linspace(0.0f, 1.0f / (2.0f * T), floor(N / 2), freq_space.buffer);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        EI_DSP_MATRIX(peaks_matrix, output_matrix->rows * 10, 1);

        uint16_t peak_count;
        ret = find_peak_indexes(fft_matrix, &peaks_matrix, 0.0f, &peak_count);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        // turn this into C++ vector and sort it based on amplitude
        ei_vector<freq_peak_t> peaks;
        for (uint8_t ix = 0; ix < peak_count; ix++) {
            freq_peak_t d;

            d.freq = freq_space.buffer[static_cast<uint32_t>(peaks_matrix.buffer[ix])];
            d.amplitude = fft_matrix->buffer[static_cast<uint32_t>(peaks_matrix.buffer[ix])];
            // printf("freq %f : %f amp: %f\r\n", peaks_matrix.buffer[ix], d.freq, d.amplitude);
            if (d.amplitude < threshold) {
                d.freq = 0.0f;
                d.amplitude = 0.0f;
            }
            peaks.push_back(d);
        }
        sort(peaks.begin(), peaks.end(),
            [](const freq_peak_t & a, const freq_peak_t & b) -> bool
        {
            return a.amplitude > b.amplitude;
        });

        // fill with zeros at the end (if needed)
        for (size_t ix = peaks.size(); ix < output_matrix->rows; ix++) {
            freq_peak_t d;
            d.freq = 0;
            d.amplitude = 0;
            peaks.push_back(d);
        }

        for (size_t row = 0; row < output_matrix->rows; row++) {
            // col 0 is freq, col 1 is ampl
            output_matrix->buffer[row * output_matrix->cols + 0] = peaks[row].freq;
            output_matrix->buffer[row * output_matrix->cols + 1] = peaks[row].amplitude;
        }

        return EIDSP_OK;
    }


    /**
     * Calculate spectral power edges in a singal
     * @param fft_matrix FFT matrix (1xM)
     * @param input_matrix_cols Number of columns in the input matrix
     * @param edges_matrix The power edges (Nx1) where N=is number of edges
     *      (e.g. [0.1, 0.5, 1.0, 2.0, 5.0])
     * @param output_matrix Output matrix of size (N-1 x 1)
     * @param sampling_freq Sampling frequency
     * @returns 0 if OK
     */
    int spectral_power_edges(
        matrix_t *fft_matrix,
        matrix_t *freq_matrix,
        matrix_t *edges_matrix,
        matrix_t *output_matrix,
        float sampling_freq
    ) {
        if (fft_matrix->rows != 1 || freq_matrix->rows != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (edges_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->rows != edges_matrix->rows - 1 || output_matrix->cols != edges_matrix->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (fft_matrix->cols != freq_matrix->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        EI_DSP_MATRIX(buckets, 1, edges_matrix->rows - 1);
        EI_DSP_MATRIX(bucket_count, 1, edges_matrix->rows - 1);

        for (uint16_t ix = 0; ix < freq_matrix->cols; ix++) {
            float t = freq_matrix->buffer[ix];
            float v = fft_matrix->buffer[ix];

            // does this fit between any edges?
            for (uint16_t ex = 0; ex < edges_matrix->rows - 1; ex++) {
                if (t >= edges_matrix->buffer[ex] && t < edges_matrix->buffer[ex + 1]) {
                    buckets.buffer[ex] += v;
                    bucket_count.buffer[ex]++;
                    break;
                }
            }
        }

        // average out and push to vector
        for (uint16_t ex = 0; ex < edges_matrix->rows - 1; ex++) {
            if (bucket_count.buffer[ex] == 0.0f) {
                output_matrix->buffer[ex] = 0.0f;
            }
            else {
                output_matrix->buffer[ex] = buckets.buffer[ex] / bucket_count.buffer[ex];
            }
        }

        return EIDSP_OK;
    }


    /**
     * Estimate power spectral density using a periodogram using Welch's method.
     * @param input_matrix Of size 1xN
     * @param out_fft_matrix Output matrix of size 1x(n_fft/2+1) with frequency data
     * @param out_freq_matrix Output matrix of size 1x(n_fft/2+1) with frequency data
     * @param sampling_freq The sampling frequency
     * @param n_fft Number of FFT buckets
     * @returns 0 if OK
     */
    int periodogram(matrix_t *input_matrix, matrix_t *out_fft_matrix, matrix_t *out_freq_matrix, float sampling_freq, uint16_t n_fft)
    {
        if (input_matrix->rows != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (out_fft_matrix->rows != 1 || out_fft_matrix->cols != static_cast<uint32_t>(n_fft / 2 + 1)) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (out_freq_matrix->rows != 1 || out_freq_matrix->cols != static_cast<uint32_t>(n_fft / 2 + 1)) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (input_matrix->buffer == NULL) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        if (out_fft_matrix->buffer == NULL) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        if (out_freq_matrix->buffer == NULL) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        // map over the input buffer, so we can manipulate the number of columns
        EI_DSP_MATRIX_B(welch_matrix, input_matrix->rows, input_matrix->cols, input_matrix->buffer);

        uint16_t nperseg = n_fft;

        if (n_fft > input_matrix->cols) {
            nperseg = input_matrix->cols;
        }
        // make the column align to nperseg in this case
        else if (n_fft < input_matrix->cols) {
            welch_matrix.cols = n_fft;
        }

        EI_DSP_MATRIX(triage_segments, 1, nperseg);
        for (uint16_t ix = 0; ix < nperseg; ix++) {
            triage_segments.buffer[ix] = 1.0f;
        }

        float scale = 1.0f / (sampling_freq * nperseg);

        for (uint16_t ix = 0; ix < n_fft / 2 + 1; ix++) {
            out_freq_matrix->buffer[ix] = static_cast<float>(ix) * (1.0f / (n_fft * (1.0f / sampling_freq)));
        }

        int ret;

        // now we need to detrend... which is done constant so just subtract the mean
        EI_DSP_MATRIX(mean_matrix, 1, 1);
        ret = numpy::mean(&welch_matrix, &mean_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        ret = numpy::subtract(&welch_matrix, &mean_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        fft_complex_t *fft_output = (fft_complex_t*)ei_dsp_calloc((n_fft / 2 + 1) * sizeof(fft_complex_t), 1);
        ret = numpy::rfft(welch_matrix.buffer, welch_matrix.cols, fft_output, n_fft / 2 + 1, n_fft);
        if (ret != EIDSP_OK) {
            ei_dsp_free(fft_output, (n_fft / 2 + 1) * sizeof(fft_complex_t));
            EIDSP_ERR(ret);
        }

        // conjugate and then multiply with itself and scale
        for (uint16_t ix = 0; ix < n_fft / 2 + 1; ix++) {
            fft_output[ix].r = (fft_output[ix].r * fft_output[ix].r) +
                (abs(fft_output[ix].i * fft_output[ix].i));
            fft_output[ix].i = 0.0f;

            fft_output[ix].r *= scale;

            if (ix != n_fft / 2) {
                fft_output[ix].r *= 2;
            }

            // then multiply by itself...
            out_fft_matrix->buffer[ix] = fft_output[ix].r;
        }

        ei_dsp_free(fft_output, (n_fft / 2 + 1) * sizeof(fft_complex_t));

        return EIDSP_OK;
    }

    static int subtract_mean(matrix_t* input_matrix) {
        // calculate the mean
        EI_DSP_MATRIX(mean_matrix, input_matrix->rows, 1);
        int ret = numpy::mean(input_matrix, &mean_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        // scale by the mean
        ret = numpy::subtract(input_matrix, &mean_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        return EIDSP_OK;
    }
} // namespace processing
} // namespace spectral
} // namespace ei

#endif // _EIDSP_SPECTRAL_PROCESSING_H_
