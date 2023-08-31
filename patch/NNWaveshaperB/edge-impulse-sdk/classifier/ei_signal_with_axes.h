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

#ifndef _EI_CLASSIFIER_SIGNAL_WITH_AXES_H_
#define _EI_CLASSIFIER_SIGNAL_WITH_AXES_H_

#include "edge-impulse-sdk/dsp/numpy_types.h"
#include "edge-impulse-sdk/dsp/returntypes.hpp"
#include "edge-impulse-sdk/classifier/ei_model_types.h"

#if !EIDSP_SIGNAL_C_FN_POINTER

using namespace ei;

class SignalWithAxes {
public:
    SignalWithAxes(signal_t *original_signal, uint8_t *axes, size_t axes_count, const ei_impulse_t *impulse):
        _original_signal(original_signal), _axes(axes), _axes_count(axes_count), _impulse(impulse)
    {

    }

    signal_t * get_signal() {
        if (this->_axes_count == _impulse->raw_samples_per_frame) {
            return this->_original_signal;
        }

        wrapped_signal.total_length = _original_signal->total_length / _impulse->raw_samples_per_frame * _axes_count;
#ifdef __MBED__
        wrapped_signal.get_data = mbed::callback(this, &SignalWithAxes::get_data);
#else
        wrapped_signal.get_data = [this](size_t offset, size_t length, float *out_ptr) {
            return this->get_data(offset, length, out_ptr);
        };
#endif
        return &wrapped_signal;
    }

    int get_data(size_t offset, size_t length, float *out_ptr) {
        size_t offset_on_original_signal = offset / _axes_count * _impulse->raw_samples_per_frame;
        size_t length_on_original_signal = length / _axes_count * _impulse->raw_samples_per_frame;

        size_t out_ptr_ix = 0;

        for (size_t ix = offset_on_original_signal; ix < offset_on_original_signal + length_on_original_signal; ix += _impulse->raw_samples_per_frame) {
            for (size_t axis_ix = 0; axis_ix < this->_axes_count; axis_ix++) {
                int r = _original_signal->get_data(ix + _axes[axis_ix], 1, &out_ptr[out_ptr_ix++]);
                if (r != 0) {
                    return r;
                }
            }
        }

        return 0;
    }

private:
    signal_t *_original_signal;
    uint8_t *_axes;
    size_t _axes_count;
    const ei_impulse_t *_impulse;
    signal_t wrapped_signal;
};

#endif // #if !EIDSP_SIGNAL_C_FN_POINTER

#endif // _EI_CLASSIFIER_SIGNAL_WITH_AXES_H_
