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

#ifndef _EIDSP_SPECTRAL_FEATURE_H_
#define _EIDSP_SPECTRAL_FEATURE_H_

#include <stdint.h>
#include "processing.hpp"
#include "wavelet.hpp"
#include "signal.hpp"
#include "edge-impulse-sdk/dsp/ei_utils.h"
#include "model-parameters/model_metadata.h"

namespace ei {
namespace spectral {

typedef enum {
    filter_none = 0,
    filter_lowpass = 1,
    filter_highpass = 2
} filter_t;

class feature {
public:

    /**
     * Calculate the spectral features over a signal.
     * @param out_features Output matrix. Use `calculate_spectral_buffer_size` to calculate
     *  the size required. Needs as many rows as `raw_data`.
     * @param input_matrix Signal, with one row per axis
     * @param sampling_freq Sampling frequency of the signal
     * @param filter_type Filter type
     * @param filter_cutoff Filter cutoff frequency
     * @param filter_order Filter order
     * @param fft_length Length of the FFT signal
     * @param fft_peaks Number of FFT peaks to find
     * @param fft_peaks_threshold Minimum threshold
     * @param edges_matrix Spectral power edges
     * @returns 0 if OK
     */
    static int spectral_analysis(
        matrix_t *out_features,
        matrix_t *input_matrix,
        float sampling_freq,
        filter_t filter_type,
        float filter_cutoff,
        uint8_t filter_order,
        uint16_t fft_length,
        uint8_t fft_peaks,
        float fft_peaks_threshold,
        matrix_t *edges_matrix_in
    ) {
        if (out_features->rows != input_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (out_features->cols != calculate_spectral_buffer_size(true, fft_peaks, edges_matrix_in->rows)) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (edges_matrix_in->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        int ret;

        size_t axes = input_matrix->rows;

        EI_TRY(processing::subtract_mean(input_matrix) );

        // apply filter
        if (filter_type == filter_lowpass) {
            ret = spectral::processing::butterworth_lowpass_filter(
                input_matrix, sampling_freq, filter_cutoff, filter_order);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
            }
        }
        else if (filter_type == filter_highpass) {
            ret = spectral::processing::butterworth_highpass_filter(
                input_matrix, sampling_freq, filter_cutoff, filter_order);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
            }
        }

        // calculate RMS
        EI_DSP_MATRIX(rms_matrix, axes, 1);
        ret = numpy::rms(input_matrix, &rms_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        // find peaks in FFT
        EI_DSP_MATRIX(peaks_matrix, axes, fft_peaks * 2);

        for (size_t row = 0; row < input_matrix->rows; row++) {
            // per axis code

            // get a slice of the current axis
            EI_DSP_MATRIX_B(axis_matrix, 1, input_matrix->cols, input_matrix->buffer + (row * input_matrix->cols));

            // calculate FFT
            EI_DSP_MATRIX(fft_matrix, 1, fft_length / 2 + 1);
            ret = numpy::rfft(axis_matrix.buffer, axis_matrix.cols, fft_matrix.buffer, fft_matrix.cols, fft_length);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
            }

            // multiply by 2/N
            numpy::scale(&fft_matrix, (2.0f / static_cast<float>(fft_length)));

            // we're now using the FFT matrix to calculate peaks etc.
            EI_DSP_MATRIX(peaks_matrix, fft_peaks, 2);
            ret = spectral::processing::find_fft_peaks(&fft_matrix, &peaks_matrix,
                sampling_freq, fft_peaks_threshold, fft_length);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
            }

            // calculate periodogram for spectral power buckets
            EI_DSP_MATRIX(period_fft_matrix, 1, fft_length / 2 + 1);
            EI_DSP_MATRIX(period_freq_matrix, 1, fft_length / 2 + 1);
            ret = spectral::processing::periodogram(&axis_matrix,
                &period_fft_matrix, &period_freq_matrix, sampling_freq, fft_length);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }

            EI_DSP_MATRIX(edges_matrix_out, edges_matrix_in->rows - 1, 1);
            ret = spectral::processing::spectral_power_edges(
                &period_fft_matrix,
                &period_freq_matrix,
                edges_matrix_in,
                &edges_matrix_out,
                sampling_freq);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }

            float *features_row = out_features->buffer + (row * out_features->cols);

            size_t fx = 0;

            features_row[fx++] = rms_matrix.buffer[row];
            for (size_t peak_row = 0; peak_row < peaks_matrix.rows; peak_row++) {
                features_row[fx++] = peaks_matrix.buffer[peak_row * peaks_matrix.cols + 0];
                features_row[fx++] = peaks_matrix.buffer[peak_row * peaks_matrix.cols + 1];
            }
            for (size_t edge_row = 0; edge_row < edges_matrix_out.rows; edge_row++) {
                features_row[fx++] = edges_matrix_out.buffer[edge_row * edges_matrix_out.cols] / 10.0f;
            }
        }

        return EIDSP_OK;
    }


    /**
     * Calculate the buffer size for Spectral Analysis
     * @param rms: Whether to calculate the RMS as part of the features
     * @param peaks_count: Number of FFT peaks
     * @param spectral_edges_count: Number of spectral edges
     */
    static size_t calculate_spectral_buffer_size(
        bool rms, size_t peaks_count, size_t spectral_edges_count)
    {
        size_t count = 0;
        if (rms) count++;
        count += (peaks_count * 2);
        if (spectral_edges_count > 0) {
            count += (spectral_edges_count - 1);
        }
        return count;
    }

    static int extract_spectral_analysis_features_v1(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        ei_dsp_config_spectral_analysis_t *config_ptr,
        const float sampling_freq)
    {
        // scale the signal
        int ret = numpy::scale(input_matrix, config_ptr->scale_axes);
        if (ret != EIDSP_OK) {
            ei_printf("ERR: Failed to scale signal (%d)\n", ret);
            EIDSP_ERR(ret);
        }

        // transpose the matrix so we have one row per axis (nifty!)
        ret = numpy::transpose(input_matrix);
        if (ret != EIDSP_OK) {
            ei_printf("ERR: Failed to transpose matrix (%d)\n", ret);
            EIDSP_ERR(ret);
        }

        // the spectral edges that we want to calculate
        matrix_t edges_matrix_in(64, 1);
        size_t edge_matrix_ix = 0;

        char spectral_str[128] = { 0 };
        if (strlen(config_ptr->spectral_power_edges) > sizeof(spectral_str) - 1) {
            EIDSP_ERR(EIDSP_PARAMETER_INVALID);
        }
        memcpy(
            spectral_str,
            config_ptr->spectral_power_edges,
            strlen(config_ptr->spectral_power_edges));

        // convert spectral_power_edges (string) into float array
        char *spectral_ptr = spectral_str;
        while (spectral_ptr != NULL) {
            while ((*spectral_ptr) == ' ') {
                spectral_ptr++;
            }

            edges_matrix_in.buffer[edge_matrix_ix++] = atof(spectral_ptr);

            // find next (spectral) delimiter (or '\0' character)
            while ((*spectral_ptr != ',')) {
                spectral_ptr++;
                if (*spectral_ptr == '\0')
                    break;
            }

            if (*spectral_ptr == '\0') {
                spectral_ptr = NULL;
            }
            else {
                spectral_ptr++;
            }
        }
        edges_matrix_in.rows = edge_matrix_ix;

        // calculate how much room we need for the output matrix
        size_t output_matrix_cols = spectral::feature::calculate_spectral_buffer_size(
            true,
            config_ptr->spectral_peaks_count,
            edges_matrix_in.rows);
        // ei_printf("output_matrix_size %hux%zu\n", input_matrix.rows, output_matrix_cols);
        if (output_matrix->cols * output_matrix->rows !=
            static_cast<uint32_t>(output_matrix_cols * config_ptr->axes)) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        output_matrix->cols = output_matrix_cols;
        output_matrix->rows = config_ptr->axes;

        spectral::filter_t filter_type;
        if (strcmp(config_ptr->filter_type, "low") == 0) {
            filter_type = spectral::filter_lowpass;
        }
        else if (strcmp(config_ptr->filter_type, "high") == 0) {
            filter_type = spectral::filter_highpass;
        }
        else {
            filter_type = spectral::filter_none;
        }

        ret = spectral::feature::spectral_analysis(
            output_matrix,
            input_matrix,
            sampling_freq,
            filter_type,
            config_ptr->filter_cutoff,
            config_ptr->filter_order,
            config_ptr->fft_length,
            config_ptr->spectral_peaks_count,
            config_ptr->spectral_peaks_threshold,
            &edges_matrix_in);
        if (ret != EIDSP_OK) {
            ei_printf("ERR: Failed to calculate spectral features (%d)\n", ret);
            EIDSP_ERR(ret);
        }

        // flatten again
        output_matrix->cols = config_ptr->axes * output_matrix_cols;
        output_matrix->rows = 1;

        return EIDSP_OK;
    }

    static void get_start_stop_bin(
        float sampling_freq,
        size_t fft_length,
        float filter_cutoff,
        size_t *start_bin,
        size_t *stop_bin,
        bool is_high_pass)
    {
        // we want to find n such that fcutoff < sample_f / fft * n ( or > for high pass )
        // also, + - half bin width (sample_f/(fft*2)) for high / low pass
        float bin = filter_cutoff * fft_length / sampling_freq;
        if (is_high_pass) {
            *start_bin = static_cast<size_t>(bin - 0.5) + 1; // add one b/c we want to always round up
            // don't use the DC bin b/c it's zero
            *start_bin = *start_bin == 0 ? 1 : *start_bin;
            *stop_bin = fft_length / 2 + 1; // go one past
        }
        else {
            *start_bin = 1;
            *stop_bin = static_cast<size_t>(bin + 0.5) + 1; // go one past
        }
    }

    /**
     * @brief Calculates the spectral analysis features.
     *
     * @return the number of features calculated
     */
    static size_t extract_spec_features(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        ei_dsp_config_spectral_analysis_t *config,
        const float sampling_freq,
        const bool remove_mean = true,
        const bool transpose_and_scale_input = true)
    {
        if (transpose_and_scale_input) {
            // transpose the matrix so we have one row per axis
            numpy::transpose_in_place(input_matrix);

            // func tests for scale of 1 and does a no op in that case
            EI_TRY(numpy::scale(input_matrix, config->scale_axes));
        }

        bool do_filter = false;
        bool is_high_pass;

        // apply filter, if enabled
        // "zero" order filter allowed.  will still remove unwanted fft bins later
        if (strcmp(config->filter_type, "low") == 0) {
            if( config->filter_order ) {
                EI_TRY(spectral::processing::butterworth_lowpass_filter(
                    input_matrix,
                    sampling_freq,
                    config->filter_cutoff,
                    config->filter_order));
            }
            do_filter = true;
            is_high_pass = false;
        }
        else if (strcmp(config->filter_type, "high") == 0) {
            if( config->filter_order ) {
                EI_TRY(spectral::processing::butterworth_highpass_filter(
                    input_matrix,
                    sampling_freq,
                    config->filter_cutoff,
                    config->filter_order));
            }
            do_filter = true;
            is_high_pass = true;
        }

        if (remove_mean){
            EI_TRY(processing::subtract_mean(input_matrix));
        }

        // Figure bins we remove based on filter cutoff
        size_t start_bin, stop_bin;
        if (do_filter) {
            get_start_stop_bin(
                sampling_freq,
                config->fft_length,
                config->filter_cutoff,
                &start_bin,
                &stop_bin,
                is_high_pass);
        }
        else {
            start_bin = 1;
            stop_bin = config->fft_length / 2 + 1;
        }
        size_t num_bins = stop_bin - start_bin;

        float *feature_out = output_matrix->buffer;
        const float *feature_out_ori = feature_out;
        for (size_t row = 0; row < input_matrix->rows; row++) {
            float *data_window = input_matrix->get_row_ptr(row);
            size_t data_size = input_matrix->cols;

            matrix_t rms_in_matrix(1, data_size, data_window);
            matrix_t rms_out_matrix(1, 1, feature_out);
            EI_TRY(numpy::rms(&rms_in_matrix, &rms_out_matrix));

            feature_out++;

            // Standard Deviation
            float stddev = *(feature_out-1); //= sqrt(numpy::variance(data_window, data_size));
            if (stddev == 0.0f) {
                stddev = 1e-10f;
            }
            // Don't add std dev as a feature b/c it's the same as RMS
            // Skew and Kurtosis w/ shortcut:
            // See definition at https://en.wikipedia.org/wiki/Skewness
            // See definition at https://en.wikipedia.org/wiki/Kurtosis
            // Substitute 0 for mean (b/c it is subtracted out above)
            // Skew becomes: mean(X^3) / stddev^3
            // Kurtosis becomes: mean(X^4) / stddev^4
            // Note, this is the Fisher definition of Kurtosis, so subtract 3
            // (see https://docs.scipy.org/doc/scipy/reference/generated/scipy.stats.kurtosis.html)
            float s_sum = 0;
            float k_sum = 0;
            float temp;
            for (size_t i = 0; i < data_size; i++) {
                temp = data_window[i] * data_window[i] * data_window[i];
                s_sum += temp;
                k_sum += temp * data_window[i];
            }
            // Skewness out
            temp = stddev * stddev * stddev;
            *feature_out++ = (s_sum / data_size) / temp;
            // Kurtosis out
            *feature_out++ = ((k_sum / data_size) / (temp * stddev)) - 3;

            if (config->implementation_version == 4) {

                size_t fft_out_size = config->fft_length / 2 + 1;
                ei_vector<float> fft_out(fft_out_size);
                EI_TRY(numpy::welch_max_hold(
                    data_window,
                    data_size,
                    fft_out.data(),
                    0,
                    fft_out_size,
                    config->fft_length,
                    config->do_fft_overlap));

                matrix_t x(1, fft_out.size(), const_cast<float *>(fft_out.data()));
                matrix_t out(1, 1);

                *feature_out++ = (numpy::skew(&x, &out) == EIDSP_OK) ? (out.get_row_ptr(0)[0]) : 0.0f;
                *feature_out++ = (numpy::kurtosis(&x, &out) == EIDSP_OK) ? (out.get_row_ptr(0)[0]) : 0.0f;

                for (size_t i = start_bin; i < stop_bin; i++) {
                    feature_out[i - start_bin] = fft_out[i];
                }
            } else {
                EI_TRY(numpy::welch_max_hold(
                    data_window,
                    data_size,
                    feature_out,
                    start_bin,
                    stop_bin,
                    config->fft_length,
                    config->do_fft_overlap));
            }
            if (config->do_log) {
                numpy::zero_handling(feature_out, num_bins);
                ei_matrix temp(num_bins, 1, feature_out);
                numpy::log10(&temp);
            }
            feature_out += num_bins;
        }
        size_t num_features = feature_out - feature_out_ori;
        return num_features;
    }

    static int extract_spectral_analysis_features_v2(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        ei_dsp_config_spectral_analysis_t *config,
        const float sampling_freq)
    {
        size_t n_features =
            extract_spec_features(input_matrix, output_matrix, config, sampling_freq);
        return n_features == output_matrix->cols ? EIDSP_OK : EIDSP_MATRIX_SIZE_MISMATCH;
    }

    static int extract_spectral_analysis_features_v3(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        ei_dsp_config_spectral_analysis_t *config,
        const float sampling_freq)
    {
        if (strcmp(config->analysis_type, "Wavelet") == 0) {
            return wavelet::extract_wavelet_features(input_matrix, output_matrix, config, sampling_freq);
        } else {
            return extract_spectral_analysis_features_v2(input_matrix, output_matrix, config, sampling_freq);
        }
    }

    static ei_vector<int> get_ratio_combo(int r)
    {
        if (r == 1 || r == 3 || r == 10) {
            return {r};
        } else if (r == 30) {
            return {3, 10};
        } else if (r == 100) {
            return {10, 10};
        } else if (r == 1000) {
            return {10, 10, 10};
        } else {
            assert(0);
        }
        return {0}; // to make linter happy
    }

    // can do in-place or out-of-place
    static size_t _decimate(matrix_t *input_matrix, matrix_t *output_matrix, size_t ratio)
    {
        // generated by build_sav4_header in prepare.py
        static float sos_deci_3[] = {
            3.4799547399084973e-05f, 6.959909479816995e-05f, 3.4799547399084973e-05f, 1.0f, -1.416907422639627f, 0.5204552955670066f, 1.0f, 2.0f, 1.0f, 1.0f, -1.3342748248687593f, 0.594631953081447f, 1.0f, 2.0f, 1.0f, 1.0f, -1.237675162600336f, 0.7259326611233617f, 1.0f, 2.0f, 1.0f, 1.0f, -1.2180861262950025f, 0.8987833581253264};
        static float sos_zi_deci_3[] = { 0.0013094887094341828f, -0.0006648423946383296f,
                                         0.0193087012128479f,    -0.010936639208493802f,
                                         0.1485445305451165f,    -0.10217301649013415f,
                                         0.8250625539381586f,    -0.7244268881025758 };
        static float sos_deci_10[] = { 3.5863243209995215e-09f,
                                       7.172648641999043e-09f,
                                       3.5863243209995215e-09f,
                                       1.0f,
                                       -1.8204968644767618f,
                                       0.8308597403796137f,
                                       1.0f,
                                       2.0f,
                                       1.0f,
                                       1.0f,
                                       -1.8289505620176847f,
                                       0.8553173710387741f,
                                       1.0f,
                                       2.0f,
                                       1.0f,
                                       1.0f,
                                       -1.8517334482627625f,
                                       0.9015161055713813f,
                                       1.0f,
                                       2.0f,
                                       1.0f,
                                       1.0f,
                                       -1.8965395961864169f,
                                       0.9644245584642932 };
        static float sos_zi_deci_10[] = { 1.38071060429997e-06f,   -1.146570262401316e-06f,
                                          0.00020862168862901534f, -0.0001782374705409433f,
                                          0.016663820918116152f,   -0.015002020730727955f,
                                          0.9773862470492868f,     -0.9420150059170858 };

        assert(ratio == 3 || ratio == 10);

        float* sos = ratio == 3 ? sos_deci_3 : sos_deci_10;
        float* sos_zi = ratio == 3 ? sos_zi_deci_3 : sos_zi_deci_10;

        const size_t out_size = signal::get_decimated_size(input_matrix->cols, ratio);

        for (size_t row = 0; row < input_matrix->rows; row++) {
            const float *x = input_matrix->get_row_ptr(row);
            float *y = output_matrix->get_row_ptr(row);
            signal::sosfilt sosfilt(sos, sos_zi, 4);
            signal::decimate_simple(
                x,
                input_matrix->cols,
                y,
                output_matrix->cols,
                ratio,
                sosfilt);
        }

        return out_size;
    }

    static int extract_spectral_analysis_features_v4(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        ei_dsp_config_spectral_analysis_t *config,
        const float sampling_freq)
    {
        if (strcmp(config->analysis_type, "Wavelet") == 0) {
            return wavelet::extract_wavelet_features(input_matrix, output_matrix, config, sampling_freq);
        }
        else if (config->extra_low_freq == false && config->input_decimation_ratio == 1) {
            size_t n_features =
                extract_spec_features(input_matrix, output_matrix, config, sampling_freq);
            return n_features == output_matrix->cols ? EIDSP_OK : EIDSP_MATRIX_SIZE_MISMATCH;
        }
        else {
            numpy::transpose_in_place(input_matrix);
            EI_TRY(numpy::scale(input_matrix, config->scale_axes));

            if (config->input_decimation_ratio > 1) {
                ei_vector<int> ratio_combo = get_ratio_combo(config->input_decimation_ratio);
                size_t out_size = input_matrix->cols;
                for (int r : ratio_combo) {
                    out_size = _decimate(input_matrix, input_matrix, r);
                }

                // rearrange input matrix to be in the right shape after decimation
                float* out = input_matrix->get_row_ptr(0) + out_size;
                for(uint32_t r = 1; r < input_matrix->rows; r++) {
                    float *row = input_matrix->get_row_ptr(r);
                    for(size_t c = 0; c < out_size; c++) {
                        *out++ = row[c];
                    }
                }
                input_matrix->cols = out_size;
            }

            float new_sampling_freq = sampling_freq / config->input_decimation_ratio;

            // do this before extract_spec_features because extract_spec_features modifies the matrix
            constexpr size_t decimation = 10;
            const size_t decimated_size =
                signal::get_decimated_size(input_matrix->cols, decimation);
            matrix_t lf_signal(input_matrix->rows, decimated_size);
            _decimate(input_matrix, &lf_signal, decimation);

            size_t n_features = extract_spec_features(
                input_matrix,
                output_matrix,
                config,
                new_sampling_freq,
                true,
                false);

            if (n_features > 0 && config->extra_low_freq) {
                matrix_t lf_features(1, output_matrix->rows * output_matrix->cols - n_features,
                    output_matrix->buffer + n_features);

                n_features += extract_spec_features(
                    &lf_signal,
                    &lf_features,
                    config,
                    new_sampling_freq / decimation,
                    true,
                    false);
            }
            return n_features == output_matrix->cols ? EIDSP_OK : EIDSP_MATRIX_SIZE_MISMATCH;
        }
    }
};

} // namespace spectral
} // namespace ei



#endif // _EIDSP_SPECTRAL_FEATURE_H_
