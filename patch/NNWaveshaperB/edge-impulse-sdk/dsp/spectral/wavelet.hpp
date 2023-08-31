/* Edge Impulse inferencing library
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "edge-impulse-sdk/dsp/ei_vector.h"

#include "processing.hpp"
#include "wavelet_coeff.hpp"

namespace ei {
namespace spectral {

using fvec = ei_vector<float>;

inline float dot(const float *x, const float *y, size_t sz)
{
    float sum = 0.0f;
    for (size_t i = 0; i < sz; i++) {
        sum += x[i] * y[i];
    }
    return sum;
}

inline void histo(const fvec &x, size_t nbins, fvec &h, bool normalize = false)
{
    float min = *std::min_element(x.begin(), x.end());
    float max = *std::max_element(x.begin(), x.end());
    float step = (max - min) / nbins;
    h.resize(nbins);
    for (size_t i = 0; i < x.size(); i++) {
        size_t bin = (x[i] - min) / step;
        if (bin >= nbins)
            bin = nbins - 1;
        h[bin]++;
    }
    if (normalize) {
        float s = numpy::sum(h.data(), h.size());
        for (size_t i = 0; i < nbins; i++) {
            h[i] /= s;
        }
    }
}

class wavelet {

    static constexpr size_t NUM_FEATHERS_PER_COMP = 14;

    template <size_t wave_size>
    static void get_filter(const std::array<std::array<float, wave_size>, 2> wav, fvec &h, fvec &g)
    {
        size_t n = wav[0].size();
        h.resize(n);
        g.resize(n);
        for (size_t i = 0; i < n; i++) {
            h[i] = wav[0][n - i - 1];
            g[i] = wav[1][n - i - 1];
        }
    }

    static void find_filter(const char *wav, fvec &h, fvec &g)
    {
        if (strcmp(wav, "bior1.3") == 0) get_filter<6>(bior1p3, h, g);
        else if (strcmp(wav, "bior1.5") == 0) get_filter<10>(bior1p5, h, g);
        else if (strcmp(wav, "bior2.2") == 0) get_filter<6>(bior2p2, h, g);
        else if (strcmp(wav, "bior2.4") == 0) get_filter<10>(bior2p4, h, g);
        else if (strcmp(wav, "bior2.6") == 0) get_filter<14>(bior2p6, h, g);
        else if (strcmp(wav, "bior2.8") == 0) get_filter<18>(bior2p8, h, g);
        else if (strcmp(wav, "bior3.1") == 0) get_filter<4>(bior3p1, h, g);
        else if (strcmp(wav, "bior3.3") == 0) get_filter<8>(bior3p3, h, g);
        else if (strcmp(wav, "bior3.5") == 0) get_filter<12>(bior3p5, h, g);
        else if (strcmp(wav, "bior3.7") == 0) get_filter<16>(bior3p7, h, g);
        else if (strcmp(wav, "bior3.9") == 0) get_filter<20>(bior3p9, h, g);
        else if (strcmp(wav, "bior4.4") == 0) get_filter<10>(bior4p4, h, g);
        else if (strcmp(wav, "bior5.5") == 0) get_filter<12>(bior5p5, h, g);
        else if (strcmp(wav, "bior6.8") == 0) get_filter<18>(bior6p8, h, g);
        else if (strcmp(wav, "coif1") == 0) get_filter<6>(coif1, h, g);
        else if (strcmp(wav, "coif2") == 0) get_filter<12>(coif2, h, g);
        else if (strcmp(wav, "coif3") == 0) get_filter<18>(coif3, h, g);
        else if (strcmp(wav, "db2") == 0) get_filter<4>(db2, h, g);
        else if (strcmp(wav, "db3") == 0) get_filter<6>(db3, h, g);
        else if (strcmp(wav, "db4") == 0) get_filter<8>(db4, h, g);
        else if (strcmp(wav, "db5") == 0) get_filter<10>(db5, h, g);
        else if (strcmp(wav, "db6") == 0) get_filter<12>(db6, h, g);
        else if (strcmp(wav, "db7") == 0) get_filter<14>(db7, h, g);
        else if (strcmp(wav, "db8") == 0) get_filter<16>(db8, h, g);
        else if (strcmp(wav, "db9") == 0) get_filter<18>(db9, h, g);
        else if (strcmp(wav, "db10") == 0) get_filter<20>(db10, h, g);
        else if (strcmp(wav, "haar") == 0) get_filter<2>(haar, h, g);
        else if (strcmp(wav, "rbio1.3") == 0) get_filter<6>(rbio1p3, h, g);
        else if (strcmp(wav, "rbio1.5") == 0) get_filter<10>(rbio1p5, h, g);
        else if (strcmp(wav, "rbio2.2") == 0) get_filter<6>(rbio2p2, h, g);
        else if (strcmp(wav, "rbio2.4") == 0) get_filter<10>(rbio2p4, h, g);
        else if (strcmp(wav, "rbio2.6") == 0) get_filter<14>(rbio2p6, h, g);
        else if (strcmp(wav, "rbio2.8") == 0) get_filter<18>(rbio2p8, h, g);
        else if (strcmp(wav, "rbio3.1") == 0) get_filter<4>(rbio3p1, h, g);
        else if (strcmp(wav, "rbio3.3") == 0) get_filter<8>(rbio3p3, h, g);
        else if (strcmp(wav, "rbio3.5") == 0) get_filter<12>(rbio3p5, h, g);
        else if (strcmp(wav, "rbio3.7") == 0) get_filter<16>(rbio3p7, h, g);
        else if (strcmp(wav, "rbio3.9") == 0) get_filter<20>(rbio3p9, h, g);
        else if (strcmp(wav, "rbio4.4") == 0) get_filter<10>(rbio4p4, h, g);
        else if (strcmp(wav, "rbio5.5") == 0) get_filter<12>(rbio5p5, h, g);
        else if (strcmp(wav, "rbio6.8") == 0) get_filter<18>(rbio6p8, h, g);
        else if (strcmp(wav, "sym2") == 0) get_filter<4>(sym2, h, g);
        else if (strcmp(wav, "sym3") == 0) get_filter<6>(sym3, h, g);
        else if (strcmp(wav, "sym4") == 0) get_filter<8>(sym4, h, g);
        else if (strcmp(wav, "sym5") == 0) get_filter<10>(sym5, h, g);
        else if (strcmp(wav, "sym6") == 0) get_filter<12>(sym6, h, g);
        else if (strcmp(wav, "sym7") == 0) get_filter<14>(sym7, h, g);
        else if (strcmp(wav, "sym8") == 0) get_filter<16>(sym8, h, g);
        else if (strcmp(wav, "sym9") == 0) get_filter<18>(sym9, h, g);
        else if (strcmp(wav, "sym10") == 0) get_filter<20>(sym10, h, g);
        else assert(0); // wavelet not in the list
    }

    static void calculate_entropy(const fvec &y, fvec &features)
    {
        fvec h;
        histo(y, 100, h, true);
        // entropy = -sum(prob * log(prob)
        float entropy = 0.0f;
        for (size_t i = 0; i < h.size(); i++) {
            if (h[i] > 0.0f) {
                entropy -= h[i] * log(h[i]);
            }
        }
        features.push_back(entropy);
    }

    static float get_percentile_from_sorted(const fvec &sorted, float percentile)
    {
        // adding 0.5 is a trick to get rounding out of C flooring behavior during cast
        size_t index = (size_t) ((percentile * (sorted.size()-1)) + 0.5);
        return sorted[index];
    }

    static void calculate_statistics(const fvec &y, fvec &features, float mean)
    {
        fvec sorted = y;
        std::sort(sorted.begin(), sorted.end());
        features.push_back(get_percentile_from_sorted(sorted,0.05));
        features.push_back(get_percentile_from_sorted(sorted,0.25));
        features.push_back(get_percentile_from_sorted(sorted,0.75));
        features.push_back(get_percentile_from_sorted(sorted,0.95));
        features.push_back(get_percentile_from_sorted(sorted,0.5));

        matrix_t x(1, y.size(), const_cast<float *>(y.data()));
        matrix_t out(1, 1);

        features.push_back(mean);
        if (numpy::stdev(&x, &out) == EIDSP_OK)
            features.push_back(out.get_row_ptr(0)[0]);
        features.push_back(numpy::variance(const_cast<float *>(y.data()), y.size()));
        if (numpy::rms(&x, &out) == EIDSP_OK)
            features.push_back(out.get_row_ptr(0)[0]);
        if (numpy::skew(&x, &out) == EIDSP_OK)
            features.push_back(out.get_row_ptr(0)[0]);
        if (numpy::kurtosis(&x, &out) == EIDSP_OK)
            features.push_back(out.get_row_ptr(0)[0]);
    }

    static void calculate_crossings(const fvec &y, fvec &features, float mean)
    {
        size_t zc = 0;
        for (size_t i = 1; i < y.size(); i++) {
            if (y[i] * y[i - 1] < 0) {
                zc++;
            }
        }
        features.push_back(zc / (float)y.size());

        size_t mc = 0;
        for (size_t i = 1; i < y.size(); i++) {
            if ((y[i] - mean) * (y[i - 1] - mean) < 0) {
                mc++;
            }
        }
        features.push_back(mc / (float)y.size());
    }

    static void
    dwt(const float *x, size_t nx, const float *h, const float *g, size_t nh, fvec &a, fvec &d)
    {
        assert(nh <= 20 && nh > 0 && nx > 0);
        size_t nx_padded = nx + nh * 2 - 2;
        fvec x_padded(nx_padded);

        // symmetric padding (default in PyWavelet)
        for (size_t i = 0; i < nh - 2; i++)
            x_padded[i] = x[nh - 3 - i];
        for (size_t i = 0; i < nx; i++)
            x_padded[i + nh - 2] = x[i];
        for (size_t i = 0; i < nh; i++)
            x_padded[i + nx + nh - 2] = x[nx - 1 - i];

        size_t ny = (nx + nh - 1) / 2;
        a.resize(ny);
        d.resize(ny);

        // decimate and filter
        const float *xx = x_padded.data();
        for (size_t i = 0; i < ny; i++) {
            a[i] = dot(xx + 2 * i, h, nh);
            d[i] = dot(xx + 2 * i, g, nh);
        }
    }

    static void extract_features(fvec& y, fvec &features)
    {
        matrix_t x(1, y.size(), const_cast<float *>(y.data()));
        matrix_t out(1, 1);
        if (numpy::mean(&x, &out) != EIDSP_OK)
            assert(0);
        float mean = out.get_row_ptr(0)[0];

        calculate_entropy(y, features);
        calculate_crossings(y, features, mean);
        calculate_statistics(y, features, mean);
    }

    static void
    wavedec_features(const float *x, int len, const char *wav, int level, fvec &features)
    {
        assert(level > 0 && level < 8);

        fvec h;
        fvec g;
        find_filter(wav, h, g);

        features.clear();
        fvec a;
        fvec d;
        dwt(x, len, h.data(), g.data(), h.size(), a, d);
        extract_features(d, features);

        for (int l = 1; l < level; l++) {
            dwt(a.data(), a.size(), h.data(), g.data(), h.size(), a, d);
            extract_features(d, features);
        }

        extract_features(a, features);

        for (int l = 0; l <= level / 2; l++) { // reverse order to match python results.
            for (int i = 0; i < (int)NUM_FEATHERS_PER_COMP; i++) {
                std::swap(
                    features[l * NUM_FEATHERS_PER_COMP + i],
                    features[(level - l) * NUM_FEATHERS_PER_COMP + i]);
            }
        }
    }

    static int dwt_features(const float *x, int len, const char *wav, int level, fvec &features)
    {
        assert(level <= 7);

        assert(features.size() == 0); // make sure features is empty
        features.reserve((level + 1) * NUM_FEATHERS_PER_COMP);

        wavedec_features(x, len, wav, level, features);

        return features.size();
    }

    static bool check_min_size(int len, int level)
    {
        int min_size = 32 * (1 << level);
        return (len >= min_size);
    }

public:
    static int extract_wavelet_features(
        matrix_t *input_matrix,
        matrix_t *output_matrix,
        ei_dsp_config_spectral_analysis_t *config,
        const float sampling_freq)
    {
        // transpose the matrix so we have one row per axis
        numpy::transpose_in_place(input_matrix);

        // func tests for scale of 1 and does a no op in that case
        EI_TRY(numpy::scale(input_matrix, config->scale_axes));

        // apply filter, if enabled
        // "zero" order filter allowed.  will still remove unwanted fft bins later
        if (strcmp(config->filter_type, "low") == 0) {
            if (config->filter_order) {
                EI_TRY(spectral::processing::butterworth_lowpass_filter(
                    input_matrix,
                    sampling_freq,
                    config->filter_cutoff,
                    config->filter_order));
            }
        }
        else if (strcmp(config->filter_type, "high") == 0) {
            if (config->filter_order) {
                EI_TRY(spectral::processing::butterworth_highpass_filter(
                    input_matrix,
                    sampling_freq,
                    config->filter_cutoff,
                    config->filter_order));
            }
        }

        EI_TRY(processing::subtract_mean(input_matrix));

        int out_idx = 0;
        for (size_t row = 0; row < input_matrix->rows; row++) {
            float *data_window = input_matrix->get_row_ptr(row);
            size_t data_size = input_matrix->cols;

            if (!check_min_size(data_size, config->wavelet_level))
                EIDSP_ERR(EIDSP_BUFFER_SIZE_MISMATCH);

            fvec features;
            size_t num_features = dwt_features(
                data_window,
                data_size,
                config->wavelet,
                config->wavelet_level,
                features);

            assert(num_features == output_matrix->cols / input_matrix->rows);
            for (size_t i = 0; i < num_features; i++) {
                output_matrix->buffer[out_idx++] = features[i];
            }
        }
        return EIDSP_OK;
    }
};

}
}
