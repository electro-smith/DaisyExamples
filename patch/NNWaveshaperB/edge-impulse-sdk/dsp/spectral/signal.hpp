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
#include <assert.h>
#include <string.h>

namespace ei {

/**
 * @brief Class for signal processing.
 * tries to mimic scipy.signal
 *
 * @todo: call CMSIS DSP functions if available
 */
class signal {
public:
    using fvec = ei_vector<float>;

    static void scale(fvec &x, float a)
    {
        for (size_t ix = 0; ix < x.size(); ix++) {
            x[ix] *= a;
        }
    }

    /**
     * @brief Decimate a signal using a IIR filter
     * This is the counterpart of scipy.signal.decimate with zero-phase=false. This function
     * is not recommended for larger decimation factors, as it will have stability issues.
     * Use the SOS version instead.
     * @param input Input signal
     * @param output Output signal
     * @param factor Decimation factor
     * @param b Numerator coefficients
     * @param a Denominator coefficients
     * @param zi Initial conditions
     */
    static void decimate_simple(
        const fvec &input,
        fvec &output,
        size_t factor,
        const fvec &b,
        const fvec &a,
        const fvec &zi)
    {
        fvec d = zi;
        scale(d, input[0]);

        fvec filtered(input.size());
        lfilter(b, a, input, filtered, d);

        size_t output_size = input.size() / factor;
        output.resize(output_size);

        for (size_t ix = 0; ix < output_size; ix++) {
            output[ix] = filtered[ix * factor];
        }
    }

    static size_t get_decimated_size(size_t input_size, size_t factor)
    {
        return (input_size + factor - 1) / factor;
    }

    struct sosfilt {
        const float *coeff; // 6 * num_sections coefficients
        float* zi;
        fvec zi_vec; // 2 * num_sections initial conditions
        size_t num_sections;

        sosfilt(const float *coeff_, const float *zi_, size_t num_sections_)
            : coeff(coeff_),
              zi_vec(zi_, zi_ + (num_sections_ * 2)),
              num_sections(num_sections_)
        {
            zi = zi_vec.data();
        }

        /**
         * @brief IIR filters in second-order sections.
         * This is the counterpart of scipy.signal.sosfilt .
         * @param input Input signal
         * @param output Output signal. Can be the same as input for in place
         * @param x_size Minimum size of input and output signal
         */
        void run(const float *input, const size_t size, float* output)
        {
            assert(num_sections > 0);

            iir2(input, output, size, coeff, coeff + 3, zi);

            for (size_t sect = 1; sect < num_sections; sect++) {
                iir2(
                    output,
                    output,
                    size,
                    coeff + sect * 6,
                    coeff + sect * 6 + 3,
                    zi + sect * 2);
            }
        }

        void init(float x0)
        {
            for (size_t sect = 0; sect < num_sections; sect++) {
                zi[sect * 2] *= x0;
                zi[sect * 2 + 1] *= x0;
            }
        }
    };

    /**
     * @brief Decimate a signal using a IIR filter with second-order sections
     * This is the counterpart of scipy.signal.decimate with zero-phase=false.
     * @param input Input signal
     * @param output Output signal
     * @param factor Decimation factor
     * @param sos Second-order section
     */
    static void decimate_simple(
        const float *input,
        const size_t input_size,
        float *output,
        const size_t output_size,
        size_t factor,
        sosfilt &sos)
    {
        sos.init(input[0]);

        fvec filtered(input_size);
        sos.run(input, input_size, filtered.data());

        size_t expected_size = get_decimated_size(input_size, factor);
        assert(output_size >= expected_size);

        for (size_t ix = 0; ix < expected_size; ix++) {
            output[ix] = filtered[ix * factor];
        }
    }

    /**
     * @brief Linear filter.
     * This is the counterpart of scipy.signal.lfilter with zero-phase=false. This function
     * is not recommended for high order filters or cutoff close to boundaries, as it will
     * have stability issues. Use the sosfilt instead.
     * @param input Input signal
     * @param output Output signal
     * @param b Numerator coefficients
     * @param a Denominator coefficients
     * @param zi Initial conditions
     */
    static void lfilter(const fvec &b, const fvec &a, const fvec &x, fvec &y, fvec &d)
    {
        /*
         a[0]*y[n] = b[0] * x[n]               + d[0][n-1]
           d[0][n] = b[1] * x[n] - a[1] * y[n] + d[1][n-1]
           d[1][n] = b[2] * x[n] - a[2] * y[n] + d[2][n-1]
         ...
         d[N-2][n] = b[N-1]*x[n] - a[N-1]*y[n] + d[N-1][n-1]
         d[N-1][n] = b[N] * x[n] - a[N] * y[n]
         */

        assert(b.size() == a.size() && b.size() == d.size() + 1);
        assert(d.size() > 0);
        assert(y.size() >= x.size());
        assert(a[0] != 0.0f);

        const float one_over_a0 = 1.0f / a[0];
        for (size_t ix = 0; ix < x.size(); ix++) {
            const float xx = x[ix];
            y[ix] = b[0] * xx + d[0];
            y[ix] *= one_over_a0;
            size_t jx;
            for (jx = 1; jx < b.size() - 1; jx++) {
                d[jx - 1] = b[jx] * xx - a[jx] * y[ix] + d[jx];
            }
            d[jx - 1] = b[jx] * xx - a[jx] * y[ix];
        }
    }

    static void iir2(const float *x, float *y, size_t n, const float *b, const float *a, float *d)
    {
        /*
         a[0]*y[n] = b[0] * x[n]               + d[0][n-1]
           d[0][n] = b[1] * x[n] - a[1] * y[n] + d[1][n-1]
           d[1][n] = b[2] * x[n] - a[2] * y[n]
         */
        const float one_over_a0 = 1.0f / a[0];
        for (size_t ix = 0; ix < n; ix++) {
            const float xx = x[ix];
            y[ix] = b[0] * xx + d[0];
            y[ix] *= one_over_a0;
            d[0] = b[1] * xx - a[1] * y[ix] + d[1];
            d[1] = b[2] * xx - a[2] * y[ix];
        }
    }

    static int gcd(int a, int b)
    {
        if (b == 0)
            return a;
        return gcd(b, a % b);
    }

    /**
     * @brief Upsample, FIR and downsample.
     * This is the counterpart of scipy.signal.upfirdn without the padding.
     * @param y Input signal
     * @param y Output signal
     * @param h FIR coefficients
     */
    static void upfirdn(const float * x, size_t x_size, fvec &y, int up, int down, const fvec &h)
    {
        assert(up > 0);
        assert(down > 0);
        assert(h.size() > 0);

#if 0 // bug in optimized version
        const int N = (h.size() - 1) / 2;

        for (size_t n = 0; n < y.size(); n++) {
            float acc = 0.0f;
            for (size_t k = 0; k < h.size(); k += up) {
                const size_t x_ind = n * down + k - N;
                if (x_ind >= 0 && x_ind < x.size()) {
                    acc += h[k] * x[x_ind];
                }
            }
            y[n] = acc;
        }
#else
        int nx = x_size;
        int nh = h.size();

        // Upsample the input signal by inserting zeros
        fvec r(up * nx);
        for (int i = 0; i < nx; i++)
        {
            r[i * up] = x[i];
        }

        // Filter the upsampled signal using the given filter coefficients
        fvec z(nh + up * nx - 1);
        for (int i = 0; i < up * nx; i++)
        {
            for (int j = 0; j < nh; j++)
            {
                if (i - j >= 0 && i - j < up * nx)
                {
                    z[i] += r[i - j] * h[j];
                }
            }
        }

        // Downsample the filtered signal by skipping samples
        int skip = (nh - 1) / 2;
        for (size_t i = 0; i < y.size(); i++)
        {
            y[i] = z[i * down + skip];
        }
#endif

    }

    /**
     * @brief Resample using a polyphase FIR.
     * This is the counterpart of scipy.signal.resample_poly.
     * @param input Input signal
     * @param output Output signal, will be moved from an internal vector sized correctly.
     * @param window FIR coefficients. e.g. signal.firwin(2 * half_len + 1, f_c, window=('kaiser', 5.0))
     */
    static void resample_poly(const float* input, size_t input_size, fvec &output, int up, int down, const fvec &window)
    {
        assert(up > 0);
        assert(down > 0);
        assert(window.size() > 0 && (window.size() % 2) == 1);

        int gcd_up_down = gcd(up, down);
        up /= gcd_up_down;
        down /= gcd_up_down;

        if (up == 1 && down == 1) {
            // output = std::move(fvec(input, input + input_size));
            output = fvec(input, input + input_size);
            return;
        }

        int n_out = (input_size * up);
        n_out = n_out / down + (n_out % down == 0 ? 0 : 1);

        fvec h = window;
        scale(h, float(up));

        output.resize(n_out);
        upfirdn(input, input_size, output, up, down, h);
    }

    static void calc_decimation_ratios(
        const char *filter_type,
        float filter_cutoff,
        float sample_rate,
        std::vector<int> &ratios)
    {
        if (strcmp(filter_type, "low") == 0) {
            ratios = {1};
            return;
        }

        static const std::vector<int> supported = {1000, 100, 30, 10, 3};
        for (size_t i = 0; i < supported.size(); i++) {
            const int r = supported[i];
            if (sample_rate * 0.5f / r > filter_cutoff) {
                if (r == 3 || r == 10) {
                    ratios = {r};
                } else if (r == 30) {
                    ratios = {3, 10};
                } else if (r == 100) {
                    ratios = {10, 10};
                } else if (r == 1000) {
                    ratios = {10, 10, 10};
                }
                return;
            }
        }

    }
};

} // namespace ei
