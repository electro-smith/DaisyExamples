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

#ifndef _EIDSP_RETURN_TYPES_H_
#define _EIDSP_RETURN_TYPES_H_

#include <stdint.h>

namespace ei {

typedef enum {
    EIDSP_OK = 0,
    EIDSP_OUT_OF_MEM = -1002,
    EIDSP_SIGNAL_SIZE_MISMATCH = -1003,
    EIDSP_MATRIX_SIZE_MISMATCH = -1004,
    EIDSP_DCT_ERROR = -1005,
    EIDSP_INPUT_MATRIX_EMPTY = -1006,
    EIDSP_BUFFER_SIZE_MISMATCH = -1007,
    EIDSP_PARAMETER_INVALID = -1008,
    EIDSP_UNEXPECTED_NEXT_OFFSET = -1009,
    EIDSP_OUT_OF_BOUNDS = -1010,
    EIDSP_UNSUPPORTED_FILTER_CONFIG = -1011,
    EIDSP_NARROWING = -1012,
    EIDSP_BLOCK_VERSION_INCORRECT = -1013,
    EIDSP_NOT_SUPPORTED = -1014,
    EIDSP_REQUIRES_CMSIS_DSP = -1015,
    EIDSP_FFT_TABLE_NOT_LOADED = -1016,
    EIDSP_INFERENCE_ERROR = -1017
} EIDSP_RETURN_T;

} // namespace ei

#endif // _EIDSP_RETURN_TYPES_H_
