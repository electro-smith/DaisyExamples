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

#ifndef _EIDSP_CPP_CONFIG_H_
#define _EIDSP_CPP_CONFIG_H_

// clang-format off
#ifndef EIDSP_USE_CMSIS_DSP // __ARM_ARCH_PROFILE is a predefine of arm-gcc.  __TARGET_* is armcc
#if defined(__MBED__) || __ARM_ARCH_PROFILE == 'M' || defined(__TARGET_CPU_CORTEX_M0) || defined(__TARGET_CPU_CORTEX_M0PLUS) || defined(__TARGET_CPU_CORTEX_M3) || defined(__TARGET_CPU_CORTEX_M4) || defined(__TARGET_CPU_CORTEX_M7) || defined(USE_HAL_DRIVER) || defined(ARDUINO_NRF52_ADAFRUIT)
    // Mbed OS versions before 5.7 are not based on CMSIS5, disable CMSIS-DSP and CMSIS-NN instructions
    #if defined(__MBED__)
        #include "mbed_version.h"
        #if (MBED_VERSION < MBED_ENCODE_VERSION((5), (7), (0)))
            #define EIDSP_USE_CMSIS_DSP      0
        #else
            #define EIDSP_USE_CMSIS_DSP      1
        #endif // Mbed OS 5.7 version check

        // Arduino on Mbed targets prior to Mbed OS 6.0.0 ship their own CMSIS-DSP sources
        #if defined(ARDUINO) && (MBED_VERSION < MBED_ENCODE_VERSION((6), (0), (0)))
            #define EIDSP_LOAD_CMSIS_DSP_SOURCES      0
        #else
            #define EIDSP_LOAD_CMSIS_DSP_SOURCES      1
        #endif // Mbed OS 6.0 version check
    #else
        #define EIDSP_USE_CMSIS_DSP		        1
        #define EIDSP_LOAD_CMSIS_DSP_SOURCES    1
    #endif
#else
    #define EIDSP_USE_CMSIS_DSP     0
#endif // Mbed / ARM Core check
#endif // ifndef EIDSP_USE_CMSIS_DSP

#if EIDSP_USE_CMSIS_DSP == 1
#define EIDSP_i32                int32_t
#define EIDSP_i16                int16_t
#define EIDSP_i8                 q7_t
#define ARM_MATH_ROUNDING        1
#else
#define EIDSP_i32                int32_t
#define EIDSP_i16                int16_t
#define EIDSP_i8                 int8_t
#endif // EIDSP_USE_CMSIS_DSP

#ifndef EIDSP_USE_ASSERTS
#define EIDSP_USE_ASSERTS        0
#endif // EIDSP_USE_ASSERTS

#if EIDSP_USE_ASSERTS == 1
#include <assert.h>
#define EIDSP_ERR(err_code) ei_printf("ERR: %d (%s)\n", err_code, #err_code); assert(false)
#else // EIDSP_USE_ASSERTS == 0
#define EIDSP_ERR(err_code) return(err_code)
#endif

// To save memory you can quantize the filterbanks,
// this has an effect on runtime speed as CMSIS-DSP does not have optimized instructions
// for q7 matrix multiplication and matrix transformation...
#ifndef EIDSP_QUANTIZE_FILTERBANK
#define EIDSP_QUANTIZE_FILTERBANK    1
#endif // EIDSP_QUANTIZE_FILTERBANK

// prints buffer allocations to stdout, useful when debugging
#ifndef EIDSP_TRACK_ALLOCATIONS
#define EIDSP_TRACK_ALLOCATIONS      0
#endif // EIDSP_TRACK_ALLOCATIONS

// set EIDSP_TRACK_ALLOCATIONS=1 and EIDSP_PRINT_ALLOCATIONS=0
// to track but not print allocations
#ifndef EIDSP_PRINT_ALLOCATIONS
#define EIDSP_PRINT_ALLOCATIONS      1
#endif

#ifndef EIDSP_SIGNAL_C_FN_POINTER
#define EIDSP_SIGNAL_C_FN_POINTER    0
#endif // EIDSP_SIGNAL_C_FN_POINTER

// clang-format on
#endif // _EIDSP_CPP_CONFIG_H_
