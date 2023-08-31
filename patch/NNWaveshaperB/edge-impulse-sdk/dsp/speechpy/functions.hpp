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

#ifndef _EIDSP_SPEECHPY_FUNCTIONS_H_
#define _EIDSP_SPEECHPY_FUNCTIONS_H_

#include <math.h>
#include "../numpy.hpp"
#include "../returntypes.hpp"

namespace ei {
namespace speechpy {

class functions {
public:
    /**
     * Converting from frequency to Mel scale
     *
     * @param f The frequency values(or a single frequency) in Hz.
     * @returns The mel scale values(or a single mel).
     */
    static float frequency_to_mel(float f) {
#if EI_PORTING_RENESASRA65 == 1
        return 1127.0 * log(1.0 + f / 700.0f);
#else
        return 1127.0 * numpy::log((1.0 + f / 700.0f));
#endif
    }

    /**
     * Converting from Mel scale to frequency.
     *
     * @param mel The mel scale values(or a single mel).
     * @returns The frequency values(or a single frequency) in Hz.
     */
    static float mel_to_frequency(float mel) {
        return 700.0f * (exp(mel / 1127.0f) - 1.0f);
    }




    /**
     * Triangle, linear scale from left up to middle, then down to right
     * @param x Linspace output, will be overwritten!
     * @param x_size Size of the linspace output
     * @param left Starting index (assigned 0)
     * @param middle Index where 1.0 will be placed
     * @param right Ending index (assigned 0)
     */
    static int triangle(float *x, size_t x_size, int left, int middle, int right) {
        EI_DSP_MATRIX(out, 1, x_size);

        for (size_t ix = 0; ix < x_size; ix++) {
            if (x[ix] > left && x[ix] <= middle) {
                out.buffer[ix] = (x[ix] - left) / (middle - left);
            }

            if (x[ix] < right && middle <= x[ix]) {
                out.buffer[ix] = (right - x[ix]) / (right - middle);
            }
        }

        memcpy(x, out.buffer, x_size * sizeof(float));

        return EIDSP_OK;
    }
};

} // namespace speechpy
} // namespace ei

#endif // _EIDSP_SPEECHPY_FUNCTIONS_H_
