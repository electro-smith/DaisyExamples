/*
 * Copyright (c) 2022 Project Nayuki. (MIT License)
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

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "fast-dct-fft.h"
#include "../returntypes.hpp"
#include "../numpy.hpp"
#include "../memory.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif // M_PI

// DCT type II, unscaled
int ei::dct::transform(float vector[], size_t len) {
    const size_t fft_data_out_size = (len / 2 + 1) * sizeof(ei::fft_complex_t);
    const size_t fft_data_in_size = len * sizeof(float);

    // Allocate KissFFT input / output buffer
    fft_complex_t *fft_data_out =
        (ei::fft_complex_t*)ei_dsp_calloc(fft_data_out_size, 1);
    if (!fft_data_out) {
        return ei::EIDSP_OUT_OF_MEM;
    }

    float *fft_data_in = (float*)ei_dsp_calloc(fft_data_in_size, 1);
    if (!fft_data_in) {
        ei_dsp_free(fft_data_out, fft_data_out_size);
        return ei::EIDSP_OUT_OF_MEM;
    }

    // Preprocess the input buffer with the data from the vector
    size_t halfLen = len / 2;
    for (size_t i = 0; i < halfLen; i++) {
        fft_data_in[i] = vector[i * 2];
        fft_data_in[len - 1 - i] = vector[i * 2 + 1];
    }
    if (len % 2 == 1) {
        fft_data_in[halfLen] = vector[len - 1];
    }

    int r = ei::numpy::rfft(fft_data_in, len, fft_data_out, (len / 2 + 1), len);
    if (r != 0) {
        ei_dsp_free(fft_data_in, fft_data_in_size);
        ei_dsp_free(fft_data_out, fft_data_out_size);
        return r;
    }

    size_t i = 0;
    for (; i < len / 2 + 1; i++) {
        float temp = i * M_PI / (len * 2);
        vector[i] = fft_data_out[i].r * cos(temp) + fft_data_out[i].i * sin(temp);
    }
    //take advantage of hermetian symmetry to calculate remainder of signal
    for (; i < len; i++) {
        float temp = i * M_PI / (len * 2);
        int conj_idx = len-i;
        // second half bins not calculated would have just been the conjugate of the first half (note minus of imag)
        vector[i] = fft_data_out[conj_idx].r * cos(temp) - fft_data_out[conj_idx].i * sin(temp);
    }
    ei_dsp_free(fft_data_in, fft_data_in_size);
    ei_dsp_free(fft_data_out, fft_data_out_size);

    return 0;
}