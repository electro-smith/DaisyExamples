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

#ifndef _EI_CLASSIFIER_SIGNAL_WITH_RANGE_H_
#define _EI_CLASSIFIER_SIGNAL_WITH_RANGE_H_

#include "edge-impulse-sdk/dsp/numpy_types.h"
#include "edge-impulse-sdk/dsp/returntypes.hpp"

#if !EIDSP_SIGNAL_C_FN_POINTER

using namespace ei;

class SignalWithRange {
public:
    SignalWithRange(signal_t *original_signal, uint32_t range_start, uint32_t range_end):
        _original_signal(original_signal), _range_start(range_start), _range_end(range_end)
    {

    }

    signal_t * get_signal() {
        if (this->_range_start == 0 && this->_range_end == this->_original_signal->total_length) {
            return this->_original_signal;
        }

        wrapped_signal.total_length = _range_end - _range_start;
#ifdef __MBED__
        wrapped_signal.get_data = mbed::callback(this, &SignalWithRange::get_data);
#else
        wrapped_signal.get_data = [this](size_t offset, size_t length, float *out_ptr) {
            return this->get_data(offset, length, out_ptr);
        };
#endif
        return &wrapped_signal;
    }

    int get_data(size_t offset, size_t length, float *out_ptr) {
        return _original_signal->get_data(offset + _range_start, length, out_ptr);
    }

private:
    signal_t *_original_signal;
    uint32_t _range_start;
    uint32_t _range_end;
    signal_t wrapped_signal;
};

#endif // #if !EIDSP_SIGNAL_C_FN_POINTER

#endif // _EI_CLASSIFIER_SIGNAL_WITH_RANGE_H_
