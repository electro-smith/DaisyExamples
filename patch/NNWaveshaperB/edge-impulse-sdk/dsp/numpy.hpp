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

#ifndef _EIDSP_NUMPY_H_
#define _EIDSP_NUMPY_H_

// it's valid to include the SDK without a model, but there's information that we need
// in model_metadata.h (like the FFT tables used).
// if the compiler does not support the __has_include directive we'll assume that the
// file exists.
#ifndef __has_include
#define __has_include 1
#endif // __has_include

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <cfloat>
#include "ei_vector.h"
#include <algorithm>
#include "numpy_types.h"
#include "config.hpp"
#include "returntypes.hpp"
#include "memory.hpp"
#include "ei_utils.h"
#include "dct/fast-dct-fft.h"
#include "kissfft/kiss_fftr.h"
#if __has_include("model-parameters/model_metadata.h")
#include "model-parameters/model_metadata.h"
#endif
#if EIDSP_USE_CMSIS_DSP
#include "edge-impulse-sdk/CMSIS/DSP/Include/arm_math.h"
#include "edge-impulse-sdk/CMSIS/DSP/Include/arm_const_structs.h"
#endif

// For the following CMSIS includes, we want to use the C fallback, so include whether or not we set the CMSIS flag
#include "edge-impulse-sdk/CMSIS/DSP/Include/dsp/statistics_functions.h"

#ifdef __MBED__
#include "mbed.h"
#else
#include <functional>
#endif // __MBED__

#define EI_MAX_UINT16 65535

namespace ei {

using fvec = ei_vector<float>;
using ivec = ei_vector<int>;

// clang-format off
// lookup table for quantized values between 0.0f and 1.0f
static constexpr float quantized_values_one_zero[] = { (0.0f / 1.0f), (1.0f / 100.0f), (2.0f / 100.0f), (3.0f / 100.0f), (4.0f / 100.0f), (1.0f / 22.0f), (1.0f / 21.0f), (1.0f / 20.0f), (1.0f / 19.0f), (1.0f / 18.0f), (1.0f / 17.0f), (6.0f / 100.0f), (1.0f / 16.0f), (1.0f / 15.0f), (7.0f / 100.0f), (1.0f / 14.0f), (1.0f / 13.0f), (8.0f / 100.0f), (1.0f / 12.0f), (9.0f / 100.0f), (1.0f / 11.0f), (2.0f / 21.0f), (1.0f / 10.0f), (2.0f / 19.0f), (11.0f / 100.0f), (1.0f / 9.0f), (2.0f / 17.0f), (12.0f / 100.0f), (1.0f / 8.0f), (13.0f / 100.0f), (2.0f / 15.0f), (3.0f / 22.0f), (14.0f / 100.0f), (1.0f / 7.0f), (3.0f / 20.0f), (2.0f / 13.0f), (3.0f / 19.0f), (16.0f / 100.0f), (1.0f / 6.0f), (17.0f / 100.0f), (3.0f / 17.0f), (18.0f / 100.0f), (2.0f / 11.0f), (3.0f / 16.0f), (19.0f / 100.0f), (4.0f / 21.0f), (1.0f / 5.0f), (21.0f / 100.0f), (4.0f / 19.0f), (3.0f / 14.0f), (22.0f / 100.0f), (2.0f / 9.0f), (5.0f / 22.0f), (23.0f / 100.0f), (3.0f / 13.0f), (4.0f / 17.0f), (5.0f / 21.0f), (24.0f / 100.0f), (1.0f / 4.0f), (26.0f / 100.0f), (5.0f / 19.0f), (4.0f / 15.0f), (27.0f / 100.0f), (3.0f / 11.0f), (5.0f / 18.0f), (28.0f / 100.0f), (2.0f / 7.0f), (29.0f / 100.0f), (5.0f / 17.0f), (3.0f / 10.0f), (4.0f / 13.0f), (31.0f / 100.0f), (5.0f / 16.0f), (6.0f / 19.0f), (7.0f / 22.0f), (32.0f / 100.0f), (33.0f / 100.0f), (1.0f / 3.0f), (34.0f / 100.0f), (7.0f / 20.0f), (6.0f / 17.0f), (5.0f / 14.0f), (36.0f / 100.0f), (4.0f / 11.0f), (7.0f / 19.0f), (37.0f / 100.0f), (3.0f / 8.0f), (38.0f / 100.0f), (8.0f / 21.0f), (5.0f / 13.0f), (7.0f / 18.0f), (39.0f / 100.0f), (2.0f / 5.0f), (9.0f / 22.0f), (41.0f / 100.0f), (7.0f / 17.0f), (5.0f / 12.0f), (42.0f / 100.0f), (8.0f / 19.0f), (3.0f / 7.0f), (43.0f / 100.0f), (7.0f / 16.0f), (44.0f / 100.0f), (4.0f / 9.0f), (9.0f / 20.0f), (5.0f / 11.0f), (46.0f / 100.0f), (6.0f / 13.0f), (7.0f / 15.0f), (47.0f / 100.0f), (8.0f / 17.0f), (9.0f / 19.0f), (10.0f / 21.0f), (48.0f / 100.0f), (49.0f / 100.0f), (1.0f / 2.0f), (51.0f / 100.0f), (52.0f / 100.0f), (11.0f / 21.0f), (10.0f / 19.0f), (9.0f / 17.0f), (53.0f / 100.0f), (8.0f / 15.0f), (7.0f / 13.0f), (54.0f / 100.0f), (6.0f / 11.0f), (11.0f / 20.0f), (5.0f / 9.0f), (56.0f / 100.0f), (9.0f / 16.0f), (57.0f / 100.0f), (4.0f / 7.0f), (11.0f / 19.0f), (58.0f / 100.0f), (7.0f / 12.0f), (10.0f / 17.0f), (59.0f / 100.0f), (13.0f / 22.0f), (3.0f / 5.0f), (61.0f / 100.0f), (11.0f / 18.0f), (8.0f / 13.0f), (13.0f / 21.0f), (62.0f / 100.0f), (5.0f / 8.0f), (63.0f / 100.0f), (12.0f / 19.0f), (7.0f / 11.0f), (64.0f / 100.0f), (9.0f / 14.0f), (11.0f / 17.0f), (13.0f / 20.0f), (66.0f / 100.0f), (2.0f / 3.0f), (67.0f / 100.0f), (68.0f / 100.0f), (15.0f / 22.0f), (13.0f / 19.0f), (11.0f / 16.0f), (69.0f / 100.0f), (9.0f / 13.0f), (7.0f / 10.0f), (12.0f / 17.0f), (71.0f / 100.0f), (5.0f / 7.0f), (72.0f / 100.0f), (13.0f / 18.0f), (8.0f / 11.0f), (73.0f / 100.0f), (11.0f / 15.0f), (14.0f / 19.0f), (74.0f / 100.0f), (3.0f / 4.0f), (76.0f / 100.0f), (16.0f / 21.0f), (13.0f / 17.0f), (10.0f / 13.0f), (77.0f / 100.0f), (17.0f / 22.0f), (7.0f / 9.0f), (78.0f / 100.0f), (11.0f / 14.0f), (15.0f / 19.0f), (79.0f / 100.0f), (4.0f / 5.0f), (17.0f / 21.0f), (81.0f / 100.0f), (13.0f / 16.0f), (9.0f / 11.0f), (82.0f / 100.0f), (14.0f / 17.0f), (83.0f / 100.0f), (5.0f / 6.0f), (84.0f / 100.0f), (16.0f / 19.0f), (11.0f / 13.0f), (17.0f / 20.0f), (6.0f / 7.0f), (86.0f / 100.0f), (19.0f / 22.0f), (13.0f / 15.0f), (87.0f / 100.0f), (7.0f / 8.0f), (88.0f / 100.0f), (15.0f / 17.0f), (8.0f / 9.0f), (89.0f / 100.0f), (17.0f / 19.0f), (9.0f / 10.0f), (19.0f / 21.0f), (10.0f / 11.0f), (91.0f / 100.0f), (11.0f / 12.0f), (92.0f / 100.0f), (12.0f / 13.0f), (13.0f / 14.0f), (93.0f / 100.0f), (14.0f / 15.0f), (15.0f / 16.0f), (94.0f / 100.0f), (16.0f / 17.0f), (17.0f / 18.0f), (18.0f / 19.0f), (19.0f / 20.0f), (20.0f / 21.0f), (21.0f / 22.0f), (96.0f / 100.0f), (97.0f / 100.0f), (98.0f / 100.0f), (99.0f / 100.0f), (1.0f / 1.0f) ,
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
// clang-format on

class numpy {
public:

    static float sqrt(float x) {
#if EIDSP_USE_CMSIS_DSP
        float temp;
        arm_sqrt_f32(x, &temp);
        return temp;
#else
        return sqrtf(x);
#endif
    }

    /**
     * Roll array elements along a given axis.
     * Elements that roll beyond the last position are re-introduced at the first.
     * @param input_array
     * @param input_array_size
     * @param shift The number of places by which elements are shifted.
     * @returns EIDSP_OK if OK
     */
    static int roll(float *input_array, size_t input_array_size, int shift) {
        if (shift < 0) {
            shift = input_array_size + shift;
        }

        if (shift == 0) {
            return EIDSP_OK;
        }

        // so we need to allocate a buffer of the size of shift...
        EI_DSP_MATRIX(shift_matrix, 1, shift);

        // we copy from the end of the buffer into the shift buffer
        memcpy(shift_matrix.buffer, input_array + input_array_size - shift, shift * sizeof(float));

        // now we do a memmove to shift the array
        memmove(input_array + shift, input_array, (input_array_size - shift) * sizeof(float));

        // and copy the shift buffer back to the beginning of the array
        memcpy(input_array, shift_matrix.buffer, shift * sizeof(float));

        return EIDSP_OK;
    }

    /**
     * Roll array elements along a given axis.
     * Elements that roll beyond the last position are re-introduced at the first.
     * @param input_array
     * @param input_array_size
     * @param shift The number of places by which elements are shifted.
     * @returns EIDSP_OK if OK
     */
    static int roll(int *input_array, size_t input_array_size, int shift) {
        if (shift < 0) {
            shift = input_array_size + shift;
        }

        if (shift == 0) {
            return EIDSP_OK;
        }

        // so we need to allocate a buffer of the size of shift...
        EI_DSP_MATRIX(shift_matrix, 1, shift);

        // we copy from the end of the buffer into the shift buffer
        memcpy(shift_matrix.buffer, input_array + input_array_size - shift, shift * sizeof(int));

        // now we do a memmove to shift the array
        memmove(input_array + shift, input_array, (input_array_size - shift) * sizeof(int));

        // and copy the shift buffer back to the beginning of the array
        memcpy(input_array, shift_matrix.buffer, shift * sizeof(int));

        return EIDSP_OK;
    }

    /**
     * Roll array elements along a given axis.
     * Elements that roll beyond the last position are re-introduced at the first.
     * @param input_array
     * @param input_array_size
     * @param shift The number of places by which elements are shifted.
     * @returns EIDSP_OK if OK
     */
    static int roll(int16_t *input_array, size_t input_array_size, int shift) {
        if (shift < 0) {
            shift = input_array_size + shift;
        }

        if (shift == 0) {
            return EIDSP_OK;
        }

        // so we need to allocate a buffer of the size of shift...
        EI_DSP_MATRIX(shift_matrix, 1, shift);

        // we copy from the end of the buffer into the shift buffer
        memcpy(shift_matrix.buffer, input_array + input_array_size - shift, shift * sizeof(int16_t));

        // now we do a memmove to shift the array
        memmove(input_array + shift, input_array, (input_array_size - shift) * sizeof(int16_t));

        // and copy the shift buffer back to the beginning of the array
        memcpy(input_array, shift_matrix.buffer, shift * sizeof(int16_t));

        return EIDSP_OK;
    }

    static float sum(float *input_array, size_t input_array_size) {
        float res = 0.0f;
        for (size_t ix = 0; ix < input_array_size; ix++) {
            res += input_array[ix];
        }
        return res;
    }

    /**
     * Multiply two matrices (MxN * NxK matrix)
     * @param matrix1 Pointer to matrix1 (MxN)
     * @param matrix2 Pointer to matrix2 (NxK)
     * @param out_matrix Pointer to out matrix (MxK)
     * @returns EIDSP_OK if OK
     */
    static int dot(matrix_t *matrix1, matrix_t *matrix2, matrix_t *out_matrix) {
        if (matrix1->cols != matrix2->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        // no. of rows in matrix1 determines the
        if (matrix1->rows != out_matrix->rows || matrix2->cols != out_matrix->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

#if EIDSP_USE_CMSIS_DSP
        if (matrix1->rows > EI_MAX_UINT16 || matrix1->cols > EI_MAX_UINT16 || matrix2->rows > EI_MAX_UINT16 ||
            matrix2->cols > EI_MAX_UINT16 || out_matrix->rows > EI_MAX_UINT16 || out_matrix->cols > EI_MAX_UINT16) {
            return EIDSP_NARROWING;
        }

        const arm_matrix_instance_f32 m1 = { static_cast<uint16_t>(matrix1->rows), static_cast<uint16_t>(matrix1->cols), matrix1->buffer };
        const arm_matrix_instance_f32 m2 = { static_cast<uint16_t>(matrix2->rows), static_cast<uint16_t>(matrix2->cols), matrix2->buffer };
        arm_matrix_instance_f32 mo = { static_cast<uint16_t>(out_matrix->rows), static_cast<uint16_t>(out_matrix->cols), out_matrix->buffer };
        int status = arm_mat_mult_f32(&m1, &m2, &mo);
        if (status != ARM_MATH_SUCCESS) {
            EIDSP_ERR(status);
        }
#else
        memset(out_matrix->buffer, 0, out_matrix->rows * out_matrix->cols * sizeof(float));

        for (size_t i = 0; i < matrix1->rows; i++) {
            dot_by_row(i,
                matrix1->buffer + (i * matrix1->cols),
                matrix1->cols,
                matrix2,
                out_matrix);
        }
#endif

        return EIDSP_OK;
    }

    /**
     * Multiply two matrices (MxN * NxK matrix)
     * @param matrix1 Pointer to matrix1 (MxN)
     * @param matrix2 Pointer to quantized matrix2 (NxK)
     * @param out_matrix Pointer to out matrix (MxK)
     * @returns EIDSP_OK if OK
     */
    static int dot(matrix_t *matrix1,
                    quantized_matrix_t *matrix2,
                    matrix_t *out_matrix)
    {
        if (matrix1->cols != matrix2->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        // no. of rows in matrix1 determines the
        if (matrix1->rows != out_matrix->rows || matrix2->cols != out_matrix->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        memset(out_matrix->buffer, 0, out_matrix->rows * out_matrix->cols * sizeof(float));

        for (size_t i = 0; i < matrix1->rows; i++) {
            dot_by_row(i,
                matrix1->buffer + (i * matrix1->cols),
                matrix1->cols,
                matrix2,
                out_matrix);
        }

        return EIDSP_OK;
    }

    /**
     * Multiply two matrices lazily per row in matrix 1 (MxN * NxK matrix)
     * @param i matrix1 row index
     * @param row matrix1 row
     * @param matrix1_cols matrix1 row size (1xN)
     * @param matrix2 Pointer to matrix2 (NxK)
     * @param out_matrix Pointer to out matrix (MxK)
     * @returns EIDSP_OK if OK
     */
    static  int dot_by_row(int i, float *row, uint32_t matrix1_cols, matrix_t *matrix2, matrix_t *out_matrix) {
        if (matrix1_cols != matrix2->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

#if EIDSP_USE_CMSIS_DSP
        if (matrix1_cols > EI_MAX_UINT16 || matrix2->rows > EI_MAX_UINT16 || matrix2->cols > EI_MAX_UINT16 ||
            out_matrix->cols > EI_MAX_UINT16) {
            return EIDSP_NARROWING;
        }

        const arm_matrix_instance_f32 m1 = { 1, static_cast<uint16_t>(matrix1_cols), row };
        const arm_matrix_instance_f32 m2 = { static_cast<uint16_t>(matrix2->rows), static_cast<uint16_t>(matrix2->cols), matrix2->buffer };
        arm_matrix_instance_f32 mo = { 1, static_cast<uint16_t>(out_matrix->cols), out_matrix->buffer + (i * out_matrix->cols) };
        int status = arm_mat_mult_f32(&m1, &m2, &mo);
        if (status != ARM_MATH_SUCCESS) {
            EIDSP_ERR(status);
        }
#else
        for (size_t j = 0; j < matrix2->cols; j++) {
            float tmp = 0.0f;
            for (size_t k = 0; k < matrix1_cols; k++) {
                tmp += row[k] * matrix2->buffer[k * matrix2->cols + j];
            }
            out_matrix->buffer[i * matrix2->cols + j] += tmp;
        }
#endif

        return EIDSP_OK;
    }

    /**
     * Multiply two matrices lazily per row in matrix 1 (MxN * NxK matrix)
     * @param i matrix1 row index
     * @param row matrix1 row
     * @param matrix1_cols matrix1 row size
     * @param matrix2 Pointer to matrix2 (NxK)
     * @param out_matrix Pointer to out matrix (MxK)
     * @returns EIDSP_OK if OK
     */
    static  int dot_by_row(int i, float *row, size_t matrix1_cols,
        quantized_matrix_t *matrix2, matrix_t *out_matrix)
    {
        if (matrix1_cols != matrix2->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (uint16_t j = 0; j < matrix2->cols; j++) {
            float tmp = 0.0;
            for (uint16_t k = 0; k < matrix1_cols; k++) {
                uint8_t u8 = matrix2->buffer[k * matrix2->cols + j];
                if (u8) { // this matrix appears to be very sparsely populated
                    tmp += row[k] * quantized_values_one_zero[u8];
                }
            }
            out_matrix->buffer[i * matrix2->cols + j] = tmp;
        }

        return EIDSP_OK;
    }

    static void transpose_in_place(matrix_t *matrix) {
        size_t size = matrix->cols * matrix->rows - 1;
        float temp; // temp for swap
        size_t next; // next item to swap
        size_t cycleBegin; // index of start of cycle
        size_t i; // location in matrix
        size_t all_done_mark = 1;
        ei_vector<bool> done(size+1,false);

        i = 1; // Note that matrix[0] and last element of matrix won't move
        while (1)
        {
            cycleBegin = i;
            temp = matrix->buffer[i];
            do
            {
                size_t col = i % matrix->cols;
                size_t row = i / matrix->cols;
                // swap row and col to make new idx, b/c we want to know where in the transposed matrix
                next = col*matrix->rows + row;
                float temp2 = matrix->buffer[next];
                matrix->buffer[next] = temp;
                temp = temp2;
                done[next] = true;
                i = next;
            }
            while (i != cycleBegin);

            // start next cycle by find next not done
            for (i = all_done_mark; done[i]; i++) {
                all_done_mark++; // move the high water mark so we don't look again
                if(i>=size) { goto LOOP_END; }
            }
        }
        LOOP_END:
        // finally, swap the row and column dimensions
        std::swap(matrix->rows, matrix->cols);
    }

    /**
     * Transpose an array, souce is destination (from MxN to NxM)
     * Note: this temporary allocates a copy of the matrix on the heap.
     * @param matrix
     * @param rows
     * @param columns
     * @deprecated You probably want to use transpose_in_place
     * @returns EIDSP_OK if OK
     */
    static int transpose(matrix_t *matrix) {
        int r = transpose(matrix->buffer, matrix->cols, matrix->rows);
        if (r != 0) {
            return r;
        }

        uint16_t old_rows = matrix->rows;
        uint16_t old_cols = matrix->cols;

        matrix->rows = old_cols;
        matrix->cols = old_rows;

        return EIDSP_OK;
    }

    /**
     * Transpose an array, source is destination (from MxN to NxM)
     * @param matrix
     * @param rows
     * @param columns
     * @deprecated You probably want to use transpose_in_place
     * @returns EIDSP_OK if OK
     */
    static int transpose(float *matrix, int rows, int columns) {
        EI_DSP_MATRIX(temp_matrix, rows, columns);
        if (!temp_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

#if EIDSP_USE_CMSIS_DSP
        if (rows > EI_MAX_UINT16 || columns > EI_MAX_UINT16) {
            return EIDSP_NARROWING;
        }

        const arm_matrix_instance_f32 i_m = {
            static_cast<uint16_t>(columns),
            static_cast<uint16_t>(rows),
            matrix
        };
        arm_matrix_instance_f32 o_m = {
            static_cast<uint16_t>(rows),
            static_cast<uint16_t>(columns),
            temp_matrix.buffer
        };
        arm_status status = arm_mat_trans_f32(&i_m, &o_m);
        if (status != ARM_MATH_SUCCESS) {
            return status;
        }
#else
        for (int j = 0; j < rows; j++){
            for (int i = 0; i < columns; i++){
                temp_matrix.buffer[j * columns + i] = matrix[i * rows + j];
            }
        }
#endif

        memcpy(matrix, temp_matrix.buffer, rows * columns * sizeof(float));

        return EIDSP_OK;
    }

    /**
     * Transpose an array in place (from MxN to NxM)
     * Note: this temporary allocates a copy of the matrix on the heap.
     * @param matrix
     * @param rows
     * @param columns
     * @returns EIDSP_OK if OK
     */
    static int transpose(quantized_matrix_t *matrix) {
        int r = transpose(matrix->buffer, matrix->cols, matrix->rows);
        if (r != 0) {
            return r;
        }

        uint16_t old_rows = matrix->rows;
        uint16_t old_cols = matrix->cols;

        matrix->rows = old_cols;
        matrix->cols = old_rows;

        return EIDSP_OK;
    }

    /**
     * Transpose an array in place (from MxN to NxM)
     * @param matrix
     * @param rows
     * @param columns
     * @returns EIDSP_OK if OK
     */
    static int transpose(uint8_t *matrix, int rows, int columns) {
        // dequantization function is not used actually...
        EI_DSP_QUANTIZED_MATRIX(temp_matrix, rows, columns, &dequantize_zero_one);
        if (!temp_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        for (int j = 0; j < rows; j++){
            for (int i = 0; i < columns; i++){
                temp_matrix.buffer[j * columns + i] = matrix[i * rows + j];
            }
        }

        memcpy(matrix, temp_matrix.buffer, rows * columns * sizeof(uint8_t));

        return EIDSP_OK;
    }

    /**
     * Return the Discrete Cosine Transform of arbitrary type sequence 2.
     * @param input Input array (of size N)
     * @param N number of items in input and output array
     * @returns EIDSP_OK if OK
     */
    static int dct2(float *input, size_t N, DCT_NORMALIZATION_MODE normalization = DCT_NORMALIZATION_NONE) {
        if (N == 0) {
            return EIDSP_OK;
        }

        int ret = ei::dct::transform(input, N);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        // for some reason the output is 2x too low...
        for (size_t ix = 0; ix < N; ix++) {
            input[ix] *= 2;
        }

        if (normalization == DCT_NORMALIZATION_ORTHO) {
            input[0] = input[0] * sqrt(1.0f / static_cast<float>(4 * N));
            for (size_t ix = 1; ix < N; ix++) {
                input[ix] = input[ix] * sqrt(1.0f / static_cast<float>(2 * N));
            }
        }

        return EIDSP_OK;
    }

    /**
     * Discrete Cosine Transform of arbitrary type sequence 2 on a matrix.
     * @param matrix
     * @returns EIDSP_OK if OK
     */
    static int dct2(matrix_t *matrix, DCT_NORMALIZATION_MODE normalization = DCT_NORMALIZATION_NONE) {
        for (size_t row = 0; row < matrix->rows; row++) {
            int r = dct2(matrix->buffer + (row * matrix->cols), matrix->cols, normalization);
            if (r != EIDSP_OK) {
                return r;
            }
        }

        return EIDSP_OK;
    }

    /**
     * Quantize a float value between zero and one
     * @param value Float value
     */
    static uint8_t quantize_zero_one(float value) {
        const size_t length = sizeof(quantized_values_one_zero) / sizeof(float);

        // look in the table
        for (size_t ix = 0; ix < length; ix++) {
            if (quantized_values_one_zero[ix] == value) return ix;
        }

        // no match?

        if (value < quantized_values_one_zero[0]) {
            return quantized_values_one_zero[0];
        }
        if (value > quantized_values_one_zero[length - 1]) {
            return quantized_values_one_zero[length - 1];
        }

        int lo = 0;
        int hi = length - 1;

        while (lo <= hi) {
            int mid = (hi + lo) / 2;

            if (value < quantized_values_one_zero[mid]) {
                hi = mid - 1;
            } else if (value > quantized_values_one_zero[mid]) {
                lo = mid + 1;
            } else {
                return quantized_values_one_zero[mid];
            }
        }

        // lo == hi + 1
        return (quantized_values_one_zero[lo] - value) < (value - quantized_values_one_zero[hi]) ?
            lo :
            hi;
    }

    /**
     * Dequantize a float value between zero and one
     * @param value
     */
    static float dequantize_zero_one(uint8_t value) {
        return quantized_values_one_zero[value];
    }

    /**
     * Pad an array.
     * Pads with the reflection of the vector mirrored along the edge of the array.
     * @param input Input matrix (MxN)
     * @param output Output matrix of size (M+pad_before+pad_after x N)
     * @param pad_before Number of items to pad before
     * @param pad_after Number of items to pad after
     * @returns 0 if OK
     */
    static int pad_1d_symmetric(matrix_t *input, matrix_t *output, uint16_t pad_before, uint16_t pad_after) {
        if (output->cols != input->cols) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output->rows != input->rows + pad_before + pad_after) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (input->rows == 0) {
            EIDSP_ERR(EIDSP_INPUT_MATRIX_EMPTY);
        }

        uint32_t pad_before_index = 0;
        bool pad_before_direction_up = true;

        for (int32_t ix = pad_before - 1; ix >= 0; ix--) {
            memcpy(output->buffer + (input->cols * ix),
                input->buffer + (pad_before_index * input->cols),
                input->cols * sizeof(float));

            if (pad_before_index == 0 && !pad_before_direction_up) {
                pad_before_direction_up = true;
            }
            else if (pad_before_index == input->rows - 1 && pad_before_direction_up) {
                pad_before_direction_up = false;
            }
            else if (pad_before_direction_up) {
                pad_before_index++;
            }
            else {
                pad_before_index--;
            }
        }

        memcpy(output->buffer + (input->cols * pad_before),
            input->buffer,
            input->rows * input->cols * sizeof(float));

        int32_t pad_after_index = input->rows - 1;
        bool pad_after_direction_up = false;

        for (int32_t ix = 0; ix < pad_after; ix++) {
            memcpy(output->buffer + (input->cols * (ix + pad_before + input->rows)),
                input->buffer + (pad_after_index * input->cols),
                input->cols * sizeof(float));

            if (pad_after_index == 0 && !pad_after_direction_up) {
                pad_after_direction_up = true;
            }
            else if (pad_after_index == static_cast<int32_t>(input->rows) - 1 && pad_after_direction_up) {
                pad_after_direction_up = false;
            }
            else if (pad_after_direction_up) {
                pad_after_index++;
            }
            else {
                pad_after_index--;
            }
        }

        return EIDSP_OK;
    }

    /**
     * Scale a matrix in place
     * @param matrix
     * @param scale
     * @returns 0 if OK
     */
    static int scale(matrix_t *matrix, float scale) {
        if (scale == 1.0f) return EIDSP_OK;

#if EIDSP_USE_CMSIS_DSP
        if (matrix->rows > EI_MAX_UINT16 || matrix->cols > EI_MAX_UINT16) {
            return EIDSP_NARROWING;
        }

        const arm_matrix_instance_f32 mi = { static_cast<uint16_t>(matrix->rows), static_cast<uint16_t>(matrix->cols), matrix->buffer };
        arm_matrix_instance_f32 mo = { static_cast<uint16_t>(matrix->rows), static_cast<uint16_t>(matrix->cols), matrix->buffer };
        int status = arm_mat_scale_f32(&mi, scale, &mo);
        if (status != ARM_MATH_SUCCESS) {
            return status;
        }
#else
        for (size_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            matrix->buffer[ix] *= scale;
        }
#endif
        return EIDSP_OK;
    }


    /**
     * Scale a matrix in place, per row
     * @param matrix Input matrix (MxN)
     * @param scale_matrix Scale matrix (Mx1)
     * @returns 0 if OK
     */
    static int scale(matrix_t *matrix, matrix_t *scale_matrix) {
        if (matrix->rows != scale_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (scale_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < matrix->rows; row++) {
            EI_DSP_MATRIX_B(temp, 1, matrix->cols, matrix->buffer + (row * matrix->cols));
            int ret = scale(&temp, scale_matrix->buffer[row]);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }
        }

        return EIDSP_OK;
    }

    /**
     * Add on matrix in place
     * @param matrix
     * @param addition
     * @returns 0 if OK
     */
    static int add(matrix_t *matrix, float addition) {
        for (uint32_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            matrix->buffer[ix] += addition;
        }
        return EIDSP_OK;
    }

    /**
     * Add on a matrix in place, per row
     * @param matrix Input matrix (MxN)
     * @param add Scale matrix (Mx1)
     * @returns 0 if OK
     */
    static int add(matrix_t *matrix, matrix_t *add_matrix) {
        if (matrix->rows != add_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (add_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < matrix->rows; row++) {
            EI_DSP_MATRIX_B(temp, 1, matrix->cols, matrix->buffer + (row * matrix->cols));
            int ret = add(&temp, add_matrix->buffer[row]);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }
        }

        return EIDSP_OK;
    }

    /**
     * Subtract from matrix in place
     * @param matrix
     * @param subtraction
     * @returns 0 if OK
     */
    static int subtract(matrix_t *matrix, float subtraction) {
        for (uint32_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            matrix->buffer[ix] -= subtraction;
        }
        return EIDSP_OK;
    }

    /**
     * Add on a matrix in place, per row
     * @param matrix Input matrix (MxN)
     * @param add Scale matrix (Mx1)
     * @returns 0 if OK
     */
    static int subtract(matrix_t *matrix, matrix_t *subtract_matrix) {
        if (matrix->rows != subtract_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (subtract_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < matrix->rows; row++) {
            EI_DSP_MATRIX_B(temp, 1, matrix->cols, matrix->buffer + (row * matrix->cols));
            int ret = subtract(&temp, subtract_matrix->buffer[row]);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }
        }

        return EIDSP_OK;
    }

    /**
     * Calculate the root mean square of a matrix, one per row
     * @param matrix Matrix of size (MxN)
     * @param output_matrix Matrix of size (Mx1)
     * @returns 0 if OK
     */
    static int rms(matrix_t *matrix, matrix_t *output_matrix) {
        if (matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float rms_result;
            arm_rms_f32(matrix->buffer + (row * matrix->cols), matrix->cols, &rms_result);
            output_matrix->buffer[row] = rms_result;
#else
            float sum = 0.0;
            for(size_t ix = 0; ix < matrix->cols; ix++) {
                float v = matrix->buffer[(row * matrix->cols) + ix];
                sum += v * v;
            }
            output_matrix->buffer[row] = sqrt(sum / static_cast<float>(matrix->cols));
#endif
        }

        return EIDSP_OK;
    }

    /**
     * Calculate the mean over a matrix per row
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Mx1)
     */
    static int mean(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }
        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < input_matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float mean;
            arm_mean_f32(input_matrix->buffer + (row * input_matrix->cols), input_matrix->cols, &mean);
            output_matrix->buffer[row] = mean;
#else
            float sum = 0.0f;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                sum += input_matrix->buffer[( row * input_matrix->cols ) + col];
            }

            output_matrix->buffer[row] = sum / input_matrix->cols;
#endif
        }

        return EIDSP_OK;
    }

    /**
     * Calculate the mean over a matrix on axis 0
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Nx1)
     * @returns 0 if OK
     */
    static int mean_axis0(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->cols != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t col = 0; col < input_matrix->cols; col++) {
            // Note - not using CMSIS-DSP here
            // gathering up the current columnand moving it into sequential memory to use
            // SIMD to calculate the mean would take more time than the simple loop
            // so disable this case. The alternative is to use 2 transposes and on a "big" ARM
            // platform that will take more time

            float sum = 0.0f;

            for (size_t row = 0; row < input_matrix->rows; row++) {
                sum += input_matrix->buffer[( row * input_matrix->cols ) + col];
            }

            output_matrix->buffer[col] = sum / input_matrix->rows;
        }

        return EIDSP_OK;
    }

    /**
     * Calculate the standard deviation over a matrix on axis 0
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Nx1)
     * @returns 0 if OK
     */
    static int std_axis0(matrix_t *input_matrix, matrix_t *output_matrix) {
#if EIDSP_USE_CMSIS_DSP
        return std_axis0_CMSIS(input_matrix, output_matrix);
#else

        if (input_matrix->cols != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t col = 0; col < input_matrix->cols; col++) {
            float sum = 0.0f;

            for (size_t row = 0; row < input_matrix->rows; row++) {
                sum += input_matrix->buffer[(row * input_matrix->cols) + col];
            }

            float mean = sum / input_matrix->rows;

            float std = 0.0f;
            float tmp;
            for (size_t row = 0; row < input_matrix->rows; row++) {
                tmp = input_matrix->buffer[(row * input_matrix->cols) + col] - mean;
                std += tmp * tmp;
            }

            output_matrix->buffer[col] = sqrt(std / input_matrix->rows);
        }

        return EIDSP_OK;
#endif
    }

    /**
     * Get the minimum value in a matrix per row
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Mx1)
     */
    static int min(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }
        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < input_matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float min;
            uint32_t ix;
            arm_min_f32(input_matrix->buffer + (row * input_matrix->cols), input_matrix->cols, &min, &ix);
            output_matrix->buffer[row] = min;
#else
            float min = FLT_MAX;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                float v = input_matrix->buffer[( row * input_matrix->cols ) + col];
                if (v < min) {
                    min = v;
                }
            }

            output_matrix->buffer[row] = min;
#endif
        }

        return EIDSP_OK;
    }

    /**
     * Get the maximum value in a matrix per row
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Mx1)
     */
    static int max(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }
        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < input_matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float max;
            uint32_t ix;
            arm_max_f32(input_matrix->buffer + (row * input_matrix->cols), input_matrix->cols, &max, &ix);
            output_matrix->buffer[row] = max;
#else
            float max = -FLT_MAX;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                float v = input_matrix->buffer[( row * input_matrix->cols ) + col];
                if (v > max) {
                    max = v;
                }
            }

            output_matrix->buffer[row] = max;
#endif
        }

        return EIDSP_OK;
    }

    /**
     * Get the stdev value in a matrix per row
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Mx1)
     */
    static int stdev(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }
        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < input_matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float std;
            float var;
            cmsis_arm_variance(&input_matrix->buffer[(row * input_matrix->cols)], input_matrix->cols, &var);
            arm_sqrt_f32(var, &std);
            output_matrix->buffer[row] = std;
#else
            float sum = 0.0f;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                sum += input_matrix->buffer[(row * input_matrix->cols) + col];
            }

            float mean = sum / input_matrix->cols;

            float std = 0.0f;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                float diff;
                diff = input_matrix->buffer[(row * input_matrix->cols) + col] - mean;
                std += diff * diff;
            }

            output_matrix->buffer[row] = sqrt(std / input_matrix->cols);
#endif
        }

        return EIDSP_OK;
    }

    /**
     * Get the skewness value in a matrix per row
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Mx1)
     */
    static int skew(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }
        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < input_matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float mean;
            float var;

            // Calculate the mean & variance
            arm_mean_f32(input_matrix->buffer + (row * input_matrix->cols), input_matrix->cols, &mean);
            cmsis_arm_variance(&input_matrix->buffer[(row * input_matrix->cols)], input_matrix->cols, &var);

            // Calculate m_3
            float m_3;
            cmsis_arm_third_moment(&input_matrix->buffer[(row * input_matrix->cols)], input_matrix->cols, mean, &m_3);

            // Calculate (variance)^(3/2)
            arm_sqrt_f32(var * var * var, &var);

            // Calculate skew = (m_3) / (variance)^(3/2)
            if (var == 0.0f) {
                output_matrix->buffer[row] = 0.0f;
            } else {
                output_matrix->buffer[row] = m_3 / var;
            }
#else
            float sum = 0.0f;
            float mean;

            // Calculate the mean
            for (size_t col = 0; col < input_matrix->cols; col++) {
                sum += input_matrix->buffer[( row * input_matrix->cols ) + col];
            }
            mean = sum / input_matrix->cols;

            // Calculate the m values
            float m_3 = 0.0f;
            float m_2 = 0.0f;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                float diff;
                diff = input_matrix->buffer[( row * input_matrix->cols ) + col] - mean;
                m_3 += diff * diff * diff;
                m_2 += diff * diff;
            }
            m_3 = m_3 / input_matrix->cols;
            m_2 = m_2 / input_matrix->cols;

            // Calculate (m_2)^(3/2)
            m_2 = sqrt(m_2 * m_2 * m_2);

            // Calculate skew = (m_3) / (m_2)^(3/2)
            if (m_2 == 0.0f) {
                output_matrix->buffer[row] = 0.0f;
            } else {
                output_matrix->buffer[row] = m_3 / m_2;
            }
#endif
        }

        return EIDSP_OK;
    }

    /**
     * Get the kurtosis value in a matrix per row
     * @param input_matrix Input matrix (MxN)
     * @param output_matrix Output matrix (Mx1)
     */
    static int kurtosis(matrix_t *input_matrix, matrix_t *output_matrix) {
        if (input_matrix->rows != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }
        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        for (size_t row = 0; row < input_matrix->rows; row++) {
#if EIDSP_USE_CMSIS_DSP
            float mean;
            float var;

            // Calculate mean & variance
            arm_mean_f32(input_matrix->buffer + (row * input_matrix->cols), input_matrix->cols, &mean);
            cmsis_arm_variance(&input_matrix->buffer[(row * input_matrix->cols)], input_matrix->cols, &var);

            // Calculate m_4
            float m_4;
            cmsis_arm_fourth_moment(&input_matrix->buffer[(row * input_matrix->cols)], input_matrix->cols, mean, &m_4);

            // Calculate Fisher kurtosis = (m_4 / variance^2) - 3
            var = var * var;
            if (var == 0.0f) {
                output_matrix->buffer[row] = -3.0f;
            } else {
                output_matrix->buffer[row] = (m_4 / var) - 3.0f;
            }
#else
            // Calculate the mean
            float mean = 0.0f;
            float sum = 0.0f;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                sum += input_matrix->buffer[( row * input_matrix->cols ) + col];
            }
            mean = sum / input_matrix->cols;

            // Calculate m_4 & variance
            float m_4 = 0.0f;
            float variance = 0.0f;

            for (size_t col = 0; col < input_matrix->cols; col++) {
                float diff;
                diff = input_matrix->buffer[(row * input_matrix->cols) + col] - mean;
                float square_diff = diff * diff;
                variance += square_diff;
                m_4 += square_diff * square_diff;
            }
            m_4 = m_4 / input_matrix->cols;
            variance = variance / input_matrix->cols;

            // Square the variance
            variance = variance * variance;
            // Calculate Fisher kurtosis = (m_4 / variance^2) - 3
            if (variance == 0.0f) {
                output_matrix->buffer[row] = -3.0f;
            } else {
                output_matrix->buffer[row] = (m_4 / variance) - 3.0f;
            }
#endif
        }

        return EIDSP_OK;
    }


    /**
     * Compute the one-dimensional discrete Fourier Transform for real input.
     * This function computes the one-dimensional n-point discrete Fourier Transform (DFT) of
     * a real-valued array by means of an efficient algorithm called the Fast Fourier Transform (FFT).
     * @param src Source buffer
     * @param src_size Size of the source buffer
     * @param output Output buffer
     * @param output_size Size of the output buffer, should be n_fft / 2 + 1
     * @returns 0 if OK
     */
    static int rfft(const float *src, size_t src_size, float *output, size_t output_size, size_t n_fft) {
        size_t n_fft_out_features = (n_fft / 2) + 1;
        if (output_size != n_fft_out_features) {
            EIDSP_ERR(EIDSP_BUFFER_SIZE_MISMATCH);
        }

        // truncate if needed
        if (src_size > n_fft) {
            src_size = n_fft;
        }

        // declare input and output arrays
        EI_DSP_MATRIX(fft_input, 1, n_fft);
        if (!fft_input.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        // copy from src to fft_input
        memcpy(fft_input.buffer, src, src_size * sizeof(float));
        // pad to the rigth with zeros
        memset(fft_input.buffer + src_size, 0, (n_fft - src_size) * sizeof(kiss_fft_scalar));

#if EIDSP_USE_CMSIS_DSP
        if (n_fft != 32 && n_fft != 64 && n_fft != 128 && n_fft != 256 &&
            n_fft != 512 && n_fft != 1024 && n_fft != 2048 && n_fft != 4096) {
            int ret = software_rfft(fft_input.buffer, output, n_fft, n_fft_out_features);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }
        }
        else {
            // hardware acceleration only works for the powers above...
            arm_rfft_fast_instance_f32 rfft_instance;
            int status = cmsis_rfft_init_f32(&rfft_instance, n_fft);
            if (status != ARM_MATH_SUCCESS) {
                return status;
            }

            EI_DSP_MATRIX(fft_output, 1, n_fft);
            if (!fft_output.buffer) {
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }

            arm_rfft_fast_f32(&rfft_instance, fft_input.buffer, fft_output.buffer, 0);

            output[0] = fft_output.buffer[0];
            output[n_fft_out_features - 1] = fft_output.buffer[1];

            size_t fft_output_buffer_ix = 2;
            for (size_t ix = 1; ix < n_fft_out_features - 1; ix += 1) {
                float rms_result;
                arm_rms_f32(fft_output.buffer + fft_output_buffer_ix, 2, &rms_result);
                output[ix] = rms_result * sqrt(2);

                fft_output_buffer_ix += 2;
            }
        }
#else
        int ret = software_rfft(fft_input.buffer, output, n_fft, n_fft_out_features);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }
#endif

        return EIDSP_OK;
    }


    /**
     * Compute the one-dimensional discrete Fourier Transform for real input.
     * This function computes the one-dimensional n-point discrete Fourier Transform (DFT) of
     * a real-valued array by means of an efficient algorithm called the Fast Fourier Transform (FFT).
     * @param src Source buffer
     * @param src_size Size of the source buffer
     * @param output Output buffer
     * @param output_size Size of the output buffer, should be n_fft / 2 + 1
     * @returns 0 if OK
     */
    static int rfft(const float *src, size_t src_size, fft_complex_t *output, size_t output_size, size_t n_fft) {
        size_t n_fft_out_features = (n_fft / 2) + 1;
        if (output_size != n_fft_out_features) {
            EIDSP_ERR(EIDSP_BUFFER_SIZE_MISMATCH);
        }

        // truncate if needed
        if (src_size > n_fft) {
            src_size = n_fft;
        }

        // declare input and output arrays
        float *fft_input_buffer = NULL;
        if (src_size == n_fft) {
            fft_input_buffer = (float*)src;
        }

        EI_DSP_MATRIX_B(fft_input, 1, n_fft, fft_input_buffer);
        if (!fft_input.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        if (!fft_input_buffer) {
            // copy from src to fft_input
            memcpy(fft_input.buffer, src, src_size * sizeof(float));
            // pad to the rigth with zeros
            memset(fft_input.buffer + src_size, 0, (n_fft - src_size) * sizeof(float));
        }

#if EIDSP_USE_CMSIS_DSP
        if (n_fft != 32 && n_fft != 64 && n_fft != 128 && n_fft != 256 &&
            n_fft != 512 && n_fft != 1024 && n_fft != 2048 && n_fft != 4096) {
            int ret = software_rfft(fft_input.buffer, output, n_fft, n_fft_out_features);
            if (ret != EIDSP_OK) {
                EIDSP_ERR(ret);
            }
        }
        else {
            // hardware acceleration only works for the powers above...
            arm_rfft_fast_instance_f32 rfft_instance;
            int status = cmsis_rfft_init_f32(&rfft_instance, n_fft);
            if (status != ARM_MATH_SUCCESS) {
                return status;
            }

            EI_DSP_MATRIX(fft_output, 1, n_fft);
            if (!fft_output.buffer) {
                EIDSP_ERR(EIDSP_OUT_OF_MEM);
            }

            arm_rfft_fast_f32(&rfft_instance, fft_input.buffer, fft_output.buffer, 0);

            output[0].r = fft_output.buffer[0];
            output[0].i = 0.0f;
            output[n_fft_out_features - 1].r = fft_output.buffer[1];
            output[n_fft_out_features - 1].i = 0.0f;

            size_t fft_output_buffer_ix = 2;
            for (size_t ix = 1; ix < n_fft_out_features - 1; ix += 1) {
                output[ix].r = fft_output.buffer[fft_output_buffer_ix];
                output[ix].i = fft_output.buffer[fft_output_buffer_ix + 1];

                fft_output_buffer_ix += 2;
            }
        }
#else
        int ret = software_rfft(fft_input.buffer, output, n_fft, n_fft_out_features);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }
#endif

        return EIDSP_OK;
    }


    /**
     * Return evenly spaced numbers over a specified interval.
     * Returns num evenly spaced samples, calculated over the interval [start, stop].
     * The endpoint of the interval can optionally be excluded.
     *
     * Based on https://github.com/ntessore/algo/blob/master/linspace.c
     * Licensed in public domain (see LICENSE in repository above)
     *
     * @param start The starting value of the sequence.
     * @param stop The end value of the sequence.
     * @param number Number of samples to generate.
     * @param out Out array, with size `number`
     * @returns 0 if OK
     */
    static int linspace(float start, float stop, uint32_t number, float *out)
    {
        if (number < 1 || !out) {
            EIDSP_ERR(EIDSP_PARAMETER_INVALID);
        }

        if (number == 1) {
            out[0] = start;
            return EIDSP_OK;
        }

        // step size
        float step = (stop - start) / (number - 1);

        // do steps
        for (uint32_t ix = 0; ix < number - 1; ix++) {
            out[ix] = start + ix * step;
        }

        // last entry always stop
        out[number - 1] = stop;

        return EIDSP_OK;
    }

    /**
     * Return evenly spaced q31 numbers over a specified interval.
     * Returns num evenly spaced samples, calculated over the interval [start, stop].
     * The endpoint of the interval can optionally be excluded.
     *
     * Based on https://github.com/ntessore/algo/blob/master/linspace.c
     * Licensed in public domain (see LICENSE in repository above)
     *
     * @param start The starting value of the sequence.
     * @param stop The end value of the sequence.
     * @param number Number of samples to generate.
     * @param out Out array, with size `number`
     * @returns 0 if OK
     */
    static int linspace(EIDSP_i32 start, EIDSP_i32 stop, uint32_t number, EIDSP_i32 *out)
    {
        if (number < 1 || !out) {
            EIDSP_ERR(EIDSP_PARAMETER_INVALID);
        }

        if (number == 1) {
            out[0] = start;
            return EIDSP_OK;
        }

        // step size
        EIDSP_i32 step = (stop - start) / (number - 1);

        // do steps
        for (uint32_t ix = 0; ix < number - 1; ix++) {
            out[ix] = start + ix * step;
        }

        // last entry always stop
        out[number - 1] = stop;

        return EIDSP_OK;
    }

    /**
     * Convert an int32_t buffer into a float buffer, maps to -1..1
     * @param input
     * @param output
     * @param length
     * @returns 0 if OK
     */
    static int int32_to_float(const EIDSP_i32 *input, float *output, size_t length) {
#if EIDSP_USE_CMSIS_DSP
        arm_q31_to_float((q31_t *)input, output, length);
#else
        for (size_t ix = 0; ix < length; ix++) {
            output[ix] = (float)(input[ix]) / 2147483648.f;
        }
#endif
        return EIDSP_OK;
    }

    /**
     * Convert an float buffer into a fixedpoint 32 bit buffer, input values are
     * limited between -1 and 1
     * @param input
     * @param output
     * @param length
     * @returns 0 if OK
     */
    static int float_to_int32(const float *input, EIDSP_i32 *output, size_t length) {
#if EIDSP_USE_CMSIS_DSP
        arm_float_to_q31((float *)input, (q31_t *)output, length);
#else
        for (size_t ix = 0; ix < length; ix++) {
            output[ix] = (EIDSP_i32)saturate((int64_t)(input[ix] * 2147483648.f), 32);
        }
#endif
        return EIDSP_OK;
    }

    /**
     * Convert an int16_t buffer into a float buffer, maps to -1..1
     * @param input
     * @param output
     * @param length
     * @returns 0 if OK
     */
    static int int16_to_float(const EIDSP_i16 *input, float *output, size_t length) {
#if EIDSP_USE_CMSIS_DSP
        arm_q15_to_float((q15_t *)input, output, length);
#else
        for (size_t ix = 0; ix < length; ix++) {
            output[ix] = (float)(input[ix]) / 32768.f;
        }
#endif
        return EIDSP_OK;
    }

    /**
     * Convert an float buffer into a fixedpoint 16 bit buffer, input values are
     * limited between -1 and 1
     * @param input
     * @param output
     * @param length
     * @returns 0 if OK
     */
    static int float_to_int16(const float *input, EIDSP_i16 *output, size_t length) {
#if EIDSP_USE_CMSIS_DSP
        arm_float_to_q15((float *)input, output, length);
#else
        for (size_t ix = 0; ix < length; ix++) {
            output[ix] = (EIDSP_i16)saturate((int32_t)(input[ix] * 32768.f), 16);
        }
#endif
        return EIDSP_OK;
    }

    /**
     * Convert an int8_t buffer into a float buffer, maps to -1..1
     * @param input
     * @param output
     * @param length
     * @returns 0 if OK
     */
    static int int8_to_float(const EIDSP_i8 *input, float *output, size_t length) {
#if EIDSP_USE_CMSIS_DSP
        arm_q7_to_float((q7_t *)input, output, length);
#else
        for (size_t ix = 0; ix < length; ix++) {
            output[ix] = (float)(input[ix]) / 128;
        }
#endif
        return EIDSP_OK;
    }

#if EIDSP_SIGNAL_C_FN_POINTER == 0
    /**
     * Create a signal structure from a buffer.
     * This is useful for data that you keep in memory anyway. If you need to load from
     * flash, then create the structure yourself.
     * @param data Buffer, make sure to keep this pointer alive
     * @param data_size Size of the buffer
     * @param signal Output signal
     * @returns EIDSP_OK if ok
     */
    static int signal_from_buffer(const float *data, size_t data_size, signal_t *signal)
    {
        signal->total_length = data_size;
#ifdef __MBED__
        signal->get_data = mbed::callback(&numpy::signal_get_data, data);
#else
        signal->get_data = [data](size_t offset, size_t length, float *out_ptr) {
            return numpy::signal_get_data(data, offset, length, out_ptr);
        };
#endif
        return EIDSP_OK;
    }

#endif

#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    /**
     * > 50% faster then the math.h log() function
     * in return for a small loss in accuracy (0.00001 average diff with log())
     * From: https://stackoverflow.com/questions/39821367/very-fast-approximate-logarithm-natural-log-function-in-c/39822314#39822314
     * Licensed under the CC BY-SA 3.0
     * @param a Input number
     * @returns Natural log value of a
     */
    __attribute__((always_inline)) static inline float log(float a)
    {
        int32_t g = (int32_t) * ((int32_t *)&a);
        int32_t e = (g - 0x3f2aaaab) & 0xff800000;
        g = g - e;
        float m = (float) * ((float *)&g);
        float i = (float)e * 1.19209290e-7f; // 0x1.0p-23
        /* m in [2/3, 4/3] */
        float f = m - 1.0f;
        float s = f * f;
        /* Compute log1p(f) for f in [-1/3, 1/3] */
        float r = fmaf(0.230836749f, f, -0.279208571f); // 0x1.d8c0f0p-3, -0x1.1de8dap-2
        float t = fmaf(0.331826031f, f, -0.498910338f); // 0x1.53ca34p-2, -0x1.fee25ap-2
        r = fmaf(r, s, t);
        r = fmaf(r, s, f);
        r = fmaf(i, 0.693147182f, r); // 0x1.62e430p-1 // log(2)

        return r;
    }

    /**
     * Fast log10 and log2 functions, significantly faster than the ones from math.h (~6x for log10 on M4F)
     * From https://community.arm.com/developer/tools-software/tools/f/armds-forum/4292/cmsis-dsp-new-functionality-proposal/22621#22621
     * @param a Input number
     * @returns Log2 value of a
     */
    __attribute__((always_inline)) static inline float log2(float a)
    {
        int e;
        float f = frexpf(fabsf(a), &e);
        float y = 1.23149591368684f;
        y *= f;
        y += -4.11852516267426f;
        y *= f;
        y += 6.02197014179219f;
        y *= f;
        y += -3.13396450166353f;
        y += e;
        return y;
    }

    /**
     * Fast log10 and log2 functions, significantly faster than the ones from math.h (~6x for log10 on M4F)
     * From https://community.arm.com/developer/tools-software/tools/f/armds-forum/4292/cmsis-dsp-new-functionality-proposal/22621#22621
     * @param a Input number
     * @returns Log10 value of a
     */
    __attribute__((always_inline)) static inline float log10(float a)
    {
        return numpy::log2(a) * 0.3010299956639812f;
    }
#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

    /**
     * Calculate the natural log value of a matrix. Does an in-place replacement.
     * @param matrix Matrix (MxN)
     * @returns 0 if OK
     */
    static int log(matrix_t *matrix)
    {
        for (uint32_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            matrix->buffer[ix] = numpy::log(matrix->buffer[ix]);
        }

        return EIDSP_OK;
    }

    /**
     * Calculate the log10 of a matrix. Does an in-place replacement.
     * @param matrix Matrix (MxN)
     * @returns 0 if OK
     */
    static int log10(matrix_t *matrix)
    {
        for (uint32_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            matrix->buffer[ix] = numpy::log10(matrix->buffer[ix]);
        }

        return EIDSP_OK;
    }

    /**
     * @brief      Signed Saturate
     *
     * @param[in]  val   The value to be saturated
     * @param[in]  sat   Bit position to saturate to (1..32)
     *
     * @return     Saturated value
     */
    static int32_t saturate(int64_t val, uint32_t sat)
    {
        if ((sat >= 1U) && (sat <= 32U)) {
            int64_t max = (int64_t)((1U << (sat - 1U)) - 1U);
            int64_t min = -1 - max;
            if (val > max) {
                return (int32_t)max;
            } else if (val < min) {
                return (int32_t)min;
            }
        }
        return (int32_t)val;
    }

    /**
     * Normalize a matrix to 0..1. Does an in-place replacement.
     * Normalization done per row.
     * @param matrix
     */
    static int normalize(matrix_t *matrix) {
        // Python implementation:
        //  matrix = (matrix - np.min(matrix)) / (np.max(matrix) - np.min(matrix))
        int r;

        matrix_t temp_matrix(1, matrix->rows * matrix->cols, matrix->buffer);

        matrix_t min_matrix(1, 1);
        if (!min_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }
        r = min(&temp_matrix, &min_matrix);
        if (r != EIDSP_OK) {
            EIDSP_ERR(r);
        }

        matrix_t max_matrix(1, 1);
        if (!max_matrix.buffer) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }
        r = max(&temp_matrix, &max_matrix);
        if (r != EIDSP_OK) {
            EIDSP_ERR(r);
        }

        float min_max_diff = (max_matrix.buffer[0] - min_matrix.buffer[0]);
        /* Prevent divide by 0 by setting minimum value for divider */
        float row_scale = min_max_diff < 0.001 ? 1.0f : 1.0f / min_max_diff;

        r = subtract(&temp_matrix, min_matrix.buffer[0]);
        if (r != EIDSP_OK) {
            EIDSP_ERR(r);
        }

        r = scale(&temp_matrix, row_scale);
        if (r != EIDSP_OK) {
            EIDSP_ERR(r);
        }

        return EIDSP_OK;
    }

    /**
     * Clip (limit) the values in an array. Does an in-place replacement.
     * Values outside the interval are clipped to the interval edges.
     * For example, if an interval of [0, 1] is specified, values smaller than 0 become 0,
     * and values larger than 1 become 1.
     * @param matrix
     * @param min Min value to be clipped
     * @param max Max value to be clipped
     */
    static int clip(matrix_t *matrix, float min, float max) {
        if (max < min) {
            EIDSP_ERR(EIDSP_PARAMETER_INVALID);
        }

        for (size_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            if (matrix->buffer[ix] < min) {
                matrix->buffer[ix] = min;
            }
            else if (matrix->buffer[ix] > max) {
                matrix->buffer[ix] = max;
            }
        }

        return EIDSP_OK;
    }

    /**
     * Cut the data behind the comma on a matrix. Does an in-place replacement.
     * E.g. around([ 3.01, 4.89 ]) becomes [3, 4]
     * @param matrix
     */
    static int round(matrix_t *matrix) {
        for (size_t ix = 0; ix < matrix->rows * matrix->cols; ix++) {
            matrix->buffer[ix] = ::round(matrix->buffer[ix]);
        }

        return EIDSP_OK;
    }

    static int software_rfft(float *fft_input, float *output, size_t n_fft, size_t n_fft_out_features) {
        kiss_fft_cpx *fft_output = (kiss_fft_cpx*)ei_dsp_malloc(n_fft_out_features * sizeof(kiss_fft_cpx));
        if (!fft_output) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        size_t kiss_fftr_mem_length;

        // create fftr context
        kiss_fftr_cfg cfg = kiss_fftr_alloc(n_fft, 0, NULL, NULL, &kiss_fftr_mem_length);
        if (!cfg) {
            ei_dsp_free(fft_output, n_fft_out_features * sizeof(kiss_fft_cpx));
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        ei_dsp_register_alloc(kiss_fftr_mem_length, cfg);

        // execute the rfft operation
        kiss_fftr(cfg, fft_input, fft_output);

        // and write back to the output
        for (size_t ix = 0; ix < n_fft_out_features; ix++) {
            output[ix] = sqrt(pow(fft_output[ix].r, 2) + pow(fft_output[ix].i, 2));
        }

        ei_dsp_free(cfg, kiss_fftr_mem_length);
        ei_dsp_free(fft_output, n_fft_out_features * sizeof(kiss_fft_cpx));

        return EIDSP_OK;
    }

    static int software_rfft(float *fft_input, fft_complex_t *output, size_t n_fft, size_t n_fft_out_features)
    {
        // create fftr context
        size_t kiss_fftr_mem_length;

        kiss_fftr_cfg cfg = kiss_fftr_alloc(n_fft, 0, NULL, NULL, &kiss_fftr_mem_length);
        if (!cfg) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        ei_dsp_register_alloc(kiss_fftr_mem_length, cfg);

        // execute the rfft operation
        kiss_fftr(cfg, fft_input, (kiss_fft_cpx*)output);

        ei_dsp_free(cfg, kiss_fftr_mem_length);

        return EIDSP_OK;
    }

    static int signal_get_data(const float *in_buffer, size_t offset, size_t length, float *out_ptr)
    {
        memcpy(out_ptr, in_buffer + offset, length * sizeof(float));
        return 0;
    }

    static int signal_get_data_i16(int16_t *in_buffer, size_t offset, size_t length, int16_t *out_ptr)
    {
        memcpy(out_ptr, in_buffer + offset, length * sizeof(int16_t));
        return 0;
    }

#if EIDSP_USE_CMSIS_DSP
    /**
     * @brief      The CMSIS std variance function with the same behaviour as the NumPy
     * implementation
     * @details    Variance in CMSIS version is calculated using fSum / (float32_t)(blockSize - 1)
     * @param[in]  pSrc       Pointer to float block
     * @param[in]  blockSize  Number of floats in block
     * @param      pResult    The variance
     */
    static void cmsis_arm_variance(const float32_t *pSrc, uint32_t blockSize, float32_t *pResult)
    {
        uint32_t blkCnt;
        float32_t sum = 0.0f;
        float32_t fSum = 0.0f;
        float32_t fMean, fValue;
        const float32_t *pInput = pSrc;

        if (blockSize <= 1U) {
            *pResult = 0;
            return;
        }
        blkCnt = blockSize >> 2U;

        while (blkCnt > 0U) {
            sum += *pInput++;
            sum += *pInput++;
            sum += *pInput++;
            sum += *pInput++;
            blkCnt--;
        }

        /* Loop unrolling: Compute remaining outputs */
        blkCnt = blockSize % 0x4U;

        while (blkCnt > 0U) {
            sum += *pInput++;
            blkCnt--;
        }

        fMean = sum / (float32_t)blockSize;

        pInput = pSrc;

        /* Loop unrolling: Compute 4 outputs at a time */
        blkCnt = blockSize >> 2U;

        while (blkCnt > 0U) {
            fValue = *pInput++ - fMean;
            fSum += fValue * fValue;
            fValue = *pInput++ - fMean;
            fSum += fValue * fValue;
            fValue = *pInput++ - fMean;
            fSum += fValue * fValue;
            fValue = *pInput++ - fMean;
            fSum += fValue * fValue;
            blkCnt--;
        }

        /* Loop unrolling: Compute remaining outputs */
        blkCnt = blockSize % 0x4U;

        while (blkCnt > 0U) {
            fValue = *pInput++ - fMean;
            fSum += fValue * fValue;
            blkCnt--;
        }

        /* Variance */
        *pResult = fSum / (float32_t)(blockSize);
    }

    /**
     * @brief      Copy of the numpy version explicitely using the CMSIS lib
     *             for STD and Matrix transpose
     * @param      input_matrix   The input matrix
     * @param      output_matrix  The output matrix
     *
     * @return     EIDSP error
     */
    static int std_axis0_CMSIS(matrix_t *input_matrix, matrix_t *output_matrix)
    {
        arm_matrix_instance_f32 arm_in_matrix, arm_transposed_matrix;

        if (input_matrix->cols != output_matrix->rows) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        if (output_matrix->cols != 1) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        /* Copy input matrix to arm matrix */
        arm_in_matrix.numRows = input_matrix->rows;
        arm_in_matrix.numCols = input_matrix->cols;
        arm_in_matrix.pData = &input_matrix->buffer[0];
        /* Create transposed matrix */
        arm_transposed_matrix.numRows = input_matrix->cols;
        arm_transposed_matrix.numCols = input_matrix->rows;
        arm_transposed_matrix.pData = (float *)ei_calloc(input_matrix->cols * input_matrix->rows * sizeof(float), 1);

        if (arm_transposed_matrix.pData == NULL) {
            EIDSP_ERR(EIDSP_OUT_OF_MEM);
        }

        int ret = arm_mat_trans_f32(&arm_in_matrix, &arm_transposed_matrix);
        if (ret != EIDSP_OK) {
            EIDSP_ERR(ret);
        }

        for (size_t row = 0; row < arm_transposed_matrix.numRows; row++) {
            float std;
            float var;

            cmsis_arm_variance(arm_transposed_matrix.pData + (row * arm_transposed_matrix.numCols),
                               arm_transposed_matrix.numCols, &var);
            arm_sqrt_f32(var, &std);

            output_matrix->buffer[row] = std;
        }

        ei_free(arm_transposed_matrix.pData);

        return EIDSP_OK;
    }

    /**
     * @brief      A copy of the CMSIS power function, adapted to calculate the third central moment
     * @details    Calculates the sum of cubes of a block with the mean value subtracted.
     * @param[in]  pSrc       Pointer to float block
     * @param[in]  blockSize  Number of floats in block
     * @param[in]  mean       The mean to subtract from each value before cubing
     * @param      pResult    The third central moment of the input
     */
    static void cmsis_arm_third_moment(const float32_t * pSrc, uint32_t blockSize, float32_t mean, float32_t * pResult)
    {
        uint32_t blkCnt;
        float32_t sum = 0.0f;
        float32_t in;

        /* Loop unrolling: Compute 4 outputs at a time */
        blkCnt = blockSize >> 2U;

        while (blkCnt > 0U) {

            /* Compute Power and store result in a temporary variable, sum. */
            in = *pSrc++;
            in = in - mean;
            sum += in * in * in;

            in = *pSrc++;
            in = in - mean;
            sum += in * in * in;

            in = *pSrc++;
            in = in - mean;
            sum += in * in * in;

            in = *pSrc++;
            in = in - mean;
            sum += in * in * in;

            /* Decrement loop counter */
            blkCnt--;
        }

        /* Loop unrolling: Compute remaining outputs */
        blkCnt = blockSize % 0x4U;

        while (blkCnt > 0U) {
            /* Compute Power and store result in a temporary variable, sum. */
            in = *pSrc++;
            in = in - mean;
            sum += in * in * in;

            /* Decrement loop counter */
            blkCnt--;
        }

        sum = sum / blockSize;
        /* Store result to destination */
        *pResult = sum;
    }

    /**
     * @brief      A copy of the CMSIS power function, adapted to calculate the fourth central moment
     * @details    Calculates the sum of fourth powers of a block with the mean value subtracted.
     * @param[in]  pSrc       Pointer to float block
     * @param[in]  blockSize  Number of floats in block
     * @param[in]  mean       The mean to subtract from each value before calculating fourth power
     * @param      pResult    The fourth central moment of the input
     */
    static void cmsis_arm_fourth_moment(const float32_t * pSrc, uint32_t blockSize, float32_t mean, float32_t * pResult)
    {
        uint32_t blkCnt;
        float32_t sum = 0.0f;
        float32_t in;

        /* Loop unrolling: Compute 4 outputs at a time */
        blkCnt = blockSize >> 2U;

        while (blkCnt > 0U) {

            /* Compute Power and store result in a temporary variable, sum. */
            in = *pSrc++;
            in = in - mean;
            float square;
            square = in * in;
            sum += square * square;

            in = *pSrc++;
            in = in - mean;
            square = in * in;
            sum += square * square;

            in = *pSrc++;
            in = in - mean;
            square = in * in;
            sum += square * square;

            in = *pSrc++;
            in = in - mean;
            square = in * in;
            sum += square * square;

            /* Decrement loop counter */
            blkCnt--;
        }

        /* Loop unrolling: Compute remaining outputs */
        blkCnt = blockSize % 0x4U;

        while (blkCnt > 0U) {
            /* Compute Power and store result in a temporary variable, sum. */
            in = *pSrc++;
            in = in - mean;
            float square;
            square = in * in;
            sum += square * square;

            /* Decrement loop counter */
            blkCnt--;
        }

        sum = sum / blockSize;
        /* Store result to destination */
        *pResult = sum;
    }
#endif // EIDSP_USE_CMSIS_DSP

    static uint8_t count_leading_zeros(uint32_t data)
    {
      if (data == 0U) { return 32U; }

      uint32_t count = 0U;
      uint32_t mask = 0x80000000U;

      while ((data & mask) == 0U)
      {
        count += 1U;
        mask = mask >> 1U;
      }
      return count;
    }

    static void sqrt_q15(int16_t in, int16_t *pOut)
    {
        int32_t bits_val1;
        int16_t number, temp1, var1, signBits1, half;
        float temp_float1;
        union {
            int32_t fracval;
            float floatval;
        } tempconv;

        number = in;

        /* If the input is a positive number then compute the signBits. */
        if (number > 0) {
            signBits1 = count_leading_zeros(number) - 17;

            /* Shift by the number of signBits1 */
            if ((signBits1 % 2) == 0) {
                number = number << signBits1;
            } else {
                number = number << (signBits1 - 1);
            }

            /* Calculate half value of the number */
            half = number >> 1;
            /* Store the number for later use */
            temp1 = number;

            /* Convert to float */
            temp_float1 = number * 3.051757812500000e-005f;
            /* Store as integer */
            tempconv.floatval = temp_float1;
            bits_val1 = tempconv.fracval;
            /* Subtract the shifted value from the magic number to give intial guess */
            bits_val1 = 0x5f3759df - (bits_val1 >> 1); /* gives initial guess */
            /* Store as float */
            tempconv.fracval = bits_val1;
            temp_float1 = tempconv.floatval;
            /* Convert to integer format */
            var1 = (int32_t)(temp_float1 * 16384);

            /* 1st iteration */
            var1 =
                ((int16_t)(
                    (int32_t)var1 *
                        (0x3000 -
                         ((int16_t)((((int16_t)(((int32_t)var1 * var1) >> 15)) * (int32_t)half) >> 15))) >>
                    15))
                << 2;
            /* 2nd iteration */
            var1 =
                ((int16_t)(
                    (int32_t)var1 *
                        (0x3000 -
                         ((int16_t)((((int16_t)(((int32_t)var1 * var1) >> 15)) * (int32_t)half) >> 15))) >>
                    15))
                << 2;
            /* 3rd iteration */
            var1 =
                ((int16_t)(
                    (int32_t)var1 *
                        (0x3000 -
                         ((int16_t)((((int16_t)(((int32_t)var1 * var1) >> 15)) * (int32_t)half) >> 15))) >>
                    15))
                << 2;

            /* Multiply the inverse square root with the original value */
            var1 = ((int16_t)(((int32_t)temp1 * var1) >> 15)) << 1;

            /* Shift the output down accordingly */
            if ((signBits1 % 2) == 0) {
                var1 = var1 >> (signBits1 / 2);
            } else {
                var1 = var1 >> ((signBits1 - 1) / 2);
            }
            *pOut = var1;
        }
        /* If the number is a negative number then store zero as its square root value */
        else {
            *pOut = 0;
        }
    }

#if EIDSP_USE_CMSIS_DSP
    /**
     * Initialize a CMSIS-DSP fast rfft structure
     * We do it this way as this means we can compile out fast_init calls which hints the compiler
     * to which tables can be removed
     */
    static int cmsis_rfft_init_f32(arm_rfft_fast_instance_f32 *rfft_instance, const size_t n_fft)
    {
// ARM cores (ex M55) with Helium extensions (MVEF) need special treatment (Issue 2843)
#if EI_CLASSIFIER_HAS_FFT_INFO == 1 && !defined(ARM_MATH_MVEF) && !defined(EI_CLASSIFIER_LOAD_ALL_FFTS)
        arm_status status;
        switch (n_fft) {
#if EI_CLASSIFIER_LOAD_FFT_32 == 1
            case 32: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 16U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len16.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len16.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len16.pTwiddle;
                rfft_instance->fftLenRFFT = 32U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_32;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_64 == 1
            case 64: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 32U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len32.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len32.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len32.pTwiddle;
                rfft_instance->fftLenRFFT = 64U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_64;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_128 == 1
            case 128: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 64U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len64.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len64.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len64.pTwiddle;
                rfft_instance->fftLenRFFT = 128U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_128;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_256 == 1
            case 256: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 128U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len128.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len128.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len128.pTwiddle;
                rfft_instance->fftLenRFFT = 256U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_256;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_512 == 1
            case 512: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 256U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len256.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len256.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len256.pTwiddle;
                rfft_instance->fftLenRFFT = 512U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_512;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_1024 == 1
            case 1024: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 512U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len512.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len512.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len512.pTwiddle;
                rfft_instance->fftLenRFFT = 1024U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_1024;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_2048 == 1
            case 2048: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 1024U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len1024.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len1024.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len1024.pTwiddle;
                rfft_instance->fftLenRFFT = 2048U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_2048;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
#if EI_CLASSIFIER_LOAD_FFT_4096 == 1
            case 4096: {
                arm_cfft_instance_f32 *S = &(rfft_instance->Sint);
                S->fftLen = 2048U;
                S->pTwiddle = NULL;
                S->bitRevLength = arm_cfft_sR_f32_len2048.bitRevLength;
                S->pBitRevTable = arm_cfft_sR_f32_len2048.pBitRevTable;
                S->pTwiddle = arm_cfft_sR_f32_len2048.pTwiddle;
                rfft_instance->fftLenRFFT = 4096U;
                rfft_instance->pTwiddleRFFT = (float32_t *) twiddleCoef_rfft_4096;
                status = ARM_MATH_SUCCESS;
                break;
            }
#endif
            default:
                return EIDSP_FFT_TABLE_NOT_LOADED;
        }

        return status;
#else
        return arm_rfft_fast_init_f32(rfft_instance, n_fft);
#endif
    }
#endif // #if EIDSP_USE_CMSIS_DSP

    /**
     * Power spectrum of a frame
     * @param frame Row of a frame
     * @param frame_size Size of the frame
     * @param out_buffer Out buffer, size should be fft_points
     * @param out_buffer_size Buffer size
     * @param fft_points (int): The length of FFT. If fft_length is greater than frame_len, the frames will be zero-padded.
     * @returns EIDSP_OK if OK
     */
    static int power_spectrum(
        float *frame,
        size_t frame_size,
        float *out_buffer,
        size_t out_buffer_size,
        uint16_t fft_points)
    {
        if (out_buffer_size != static_cast<size_t>(fft_points / 2 + 1)) {
            EIDSP_ERR(EIDSP_MATRIX_SIZE_MISMATCH);
        }

        int r = numpy::rfft(frame, frame_size, out_buffer, out_buffer_size, fft_points);
        if (r != EIDSP_OK) {
            return r;
        }

        for (size_t ix = 0; ix < out_buffer_size; ix++) {
            out_buffer[ix] = (1.0 / static_cast<float>(fft_points)) *
                (out_buffer[ix] * out_buffer[ix]);
        }

        return EIDSP_OK;
    }

    static int welch_max_hold(
        float *input,
        size_t input_size,
        float *output,
        size_t start_bin,
        size_t stop_bin,
        size_t fft_points,
        bool do_overlap)
    {
        // save off one point to put back, b/c we're going to calculate in place
        float saved_point = 0;
        bool do_saved_point = false;
        size_t fft_out_size = fft_points / 2 + 1;
        float *fft_out;
        ei_unique_ptr_t p_fft_out(nullptr, ei_free);
        if (input_size < fft_points) {
            fft_out = (float *)ei_calloc(fft_out_size, sizeof(float));
            p_fft_out.reset(fft_out);
        }
        else {
            // set input as output for in place operation
            fft_out = input;
            // save off one point to put back, b/c we're going to calculate in place
            saved_point = input[fft_points / 2];
            do_saved_point = true;
        }

        // init the output to zeros
        memset(output, 0, sizeof(float) * (stop_bin - start_bin));
        int input_ix = 0;
        while (input_ix < (int)input_size) {
            // Figure out if we need any zero padding
            size_t n_input_points = input_ix + fft_points <= input_size ? fft_points
                                                                        : input_size - input_ix;
            EI_TRY(power_spectrum(
                input + input_ix,
                n_input_points,
                fft_out,
                fft_points / 2 + 1,
                fft_points));
            int j = 0;
            // keep the max of the last frame and everything before
            for (size_t i = start_bin; i < stop_bin; i++) {
                output[j] = std::max(output[j], fft_out[i]);
                j++;
            }
            if (do_overlap) {
                if (do_saved_point) {
                    // This step only matters first time through
                    input[fft_points / 2] = saved_point;
                    do_saved_point = false;
                }
                input_ix += fft_points / 2;
            }
            else {
                input_ix += fft_points;
            }
        }

        return EIDSP_OK;
    }

    static float variance(float *input, size_t size)
    {
        // Use CMSIS either way.  Will fall back to straight C when needed
        float temp;
#if EIDSP_USE_CMSIS_DSP
        arm_var_f32(input, size, &temp);
#else
        float mean = 0.0f;
        for (size_t i = 0; i < size; i++) {
            mean += input[i];
        }
        mean /= size;

        temp = 0.0f;
        for (size_t i = 0; i < size; i++) {
            temp += (input[i] - mean) * (input[i] - mean);
        }
        temp /= (size - 1);
#endif
        return temp;
    }

    /**
     * This function handle the issue with zero values if the are exposed
     * to become an argument for any log function.
     * @param input Array
     * @param input_size Size of array
     * @returns void
     */
    static void zero_handling(float *input, size_t input_size)
    {
        for (size_t ix = 0; ix < input_size; ix++) {
            if (input[ix] == 0) {
                input[ix] = 1e-10;
            }
        }
    }

    /**
     * This function handle the issue with zero values if the are exposed
     * to become an argument for any log function.
     * @param input Matrix
     * @returns void
     */
    static void zero_handling(matrix_t *input)
    {
        zero_handling(input->buffer, input->rows * input->cols);
    }

    __attribute__((unused)) static void scale(fvec& v, float scale) {
        for (auto& x : v) {
            x *= scale;
        }
    }

    __attribute__((unused)) static void sub(fvec& v, float b) {
        for (auto& x : v) {
            x -= b;
        }
    }

    __attribute__((unused)) static void mul(float* y, const float* x, float* b, size_t n) {
        for (size_t i = 0; i < n; i++) {
            y[i] = x[i] * b[i];
        }
    }

    __attribute__((unused)) static fvec diff(const float* v, size_t n) {
        fvec d(n - 1);
        for (size_t i = 0; i < d.size(); i++) {
            d[i] = v[i + 1] - v[i];
        }
        return d;
    }

    __attribute__((unused)) static float sum(const float* v, size_t n) {
        float sum = 0;
        for (size_t i = 0; i < n; i++) {
            sum += v[i];
        }
        return sum;
    }

    static float mean(const fvec& v) {
        float mean = 0;
        for (auto x : v) {
            mean += x;
        }
        mean /= v.size();
        return mean;
    }

    static float mean(const float* v, size_t n) {
        float mean = 0;
        for (size_t i = 0; i < n; i++) {
            mean += v[i];
        }
        mean /= n;
        return mean;
    }

    static float median(const float* v, size_t n) {
        fvec vc(n);
        std::copy(v, v + n, vc.begin());
        std::sort(vc.begin(), vc.end());
        if (vc.size() % 2 == 0) {
            return (vc[vc.size() / 2 - 1] + vc[vc.size() / 2]) / 2;
        }
        return vc[vc.size() / 2];
    }

    __attribute__((unused)) static float median(const fvec& v) {
        return median(v.data(), v.size());
    }

    static float stddev(const float* v, size_t n, float m /* mean */, int ddof = 0) {
        float var = 0;
        for (size_t i = 0; i < n; i++) {
            var += (v[i] - m) * (v[i] - m);
        }
        var /= n - ddof;
        return sqrt(var);
    }

    __attribute__((unused)) static float stddev(const float* v, size_t n) {
        return stddev(v, n, mean(v, n), 0);
    }

    __attribute__((unused)) static float stddev(const float* v, size_t n, int ddof) {
        return stddev(v, n, mean(v, n), ddof);
    }

    __attribute__((unused)) static float stddev(const fvec& v, int ddof = 0) {
        return stddev(v.data(), v.size(), mean(v), ddof);
    }

    static float rms(const float* v, size_t n) {
        float rms = 0;
        for (size_t i = 0; i < n; i++) {
            rms += v[i] * v[i];
        }
        rms /= n;
        return sqrt(rms);
    }

    __attribute__((unused)) static float rms(const fvec& v) {
        return rms(v.data(), v.size());
    }

    template <typename T>
    static float max(const ei_vector<T>& v) {
        return *std::max_element(v.begin(), v.end());
    }

    __attribute__((unused)) static float max(const float* v, size_t n) {
        return *std::max_element(v, v + n);
    }

    template <typename T>
    static float min(const ei_vector<T>& v) {
        return *std::min_element(v.begin(), v.end());
    }

    __attribute__((unused)) static float min(const float* v, size_t n) {
        return *std::min_element(v, v + n);
    }

    __attribute__((unused)) static int argmax(const fvec& v, int start, int end) {
        return std::max_element(v.begin() + start, v.begin() + end) - v.begin();
    }

    __attribute__((unused)) static fvec divide(float num, const float* den, size_t n) {
        fvec v(n);
        for (size_t i = 0; i < n; i++) {
            v[i] = num / den[i];
        }
        return v;
    }

    __attribute__((unused)) static ivec histogram(const float* x, size_t n, int a, int b, int inc) {
        int num_bins = (b - a) / inc;
        ivec bins(num_bins, 0);
        for (size_t i = 0; i < n; i++) {
            int bin = (int)((x[i] - a) / inc);
            if (bin >= 0 && bin < num_bins) {
                bins[bin]++;
            }
        }
        return bins;
    }

    __attribute__((unused)) static fvec cumsum(const float* v, size_t n) {
        fvec c(n);
        c[0] = v[0];
        for (size_t i = 1; i < n; i++) {
            c[i] = c[i - 1] + v[i];
        }
        return c;
    }

    __attribute__((unused)) static fvec arrange(float start, float end, float step) {
        assert(start < end);
        assert(step > 0);
        fvec v((size_t)((end - start) / step));
        for (size_t i = 0; i < v.size(); i++) {
            v[i] = start + i * step;
        }
        return v;
    }

    __attribute__((unused)) static void add(fvec& v, fvec& b) {
        for (size_t i = 0; i < v.size(); i++) {
            v[i] += b[i];
        }
    }

    __attribute__((unused)) static float trapz(const fvec& x, const fvec& y, size_t lo, size_t hi) {
        float area = 0;
        for (size_t i = lo; i < hi; i++) {
            area += (x[i + 1] - x[i]) * (y[i + 1] + y[i]) / 2;
        }
        return area;
    }

    __attribute__((unused)) static fvec quantile(const fvec& v, size_t start, size_t end, const fvec& q) {
        end = std::min(end, v.size());
        fvec vc(end - start);
        std::copy(v.begin() + start, v.begin() + end, vc.begin());
        std::sort(vc.begin(), vc.end());
        fvec res(q.size());
        for (size_t i = 0; i < q.size(); i++) {
            res[i] = vc[q[i] * vc.size()];
        }
        return res;
    }

    __attribute__((unused)) static fvec quantile(const float* v, size_t n, const fvec& q) {
        fvec vc(n);
        std::copy(v, v + n, vc.begin());
        std::sort(vc.begin(), vc.end());
        fvec res(q.size());
        for (size_t i = 0; i < q.size(); i++) {
            res[i] = vc[q[i] * vc.size()];
        }
        return res;
    }

    static float dot(const float* x, const float* y, size_t n) {
        float res = 0;
        for (size_t i = 0; i < n; i++) {
            res += x[i] * y[i];
        }
        return res;
    }


    __attribute__((unused)) static float cosine_similarity(const fvec& x, const fvec& y) {
        float xy = dot(x.data(), y.data(), x.size());
        float magx = dot(x.data(), x.data(), x.size());
        float magy = dot(y.data(), y.data(), y.size());
        xy /= sqrt(magx * magy);
        return xy;
    }

    __attribute__((unused)) static void ln(fvec& v) {
        for (auto& x : v) {
            x = log(x);
        }
    }

    static size_t next_power_of_2(size_t x) {
        size_t res = 1;
        while (res < x) {
            res *= 2;
        }
        return res;
    }

    static void detrend(float* data, size_t n) {
        // Calculate the mean of the data points
        float mean = 0.0;
        for (size_t i = 0; i < n; i++) {
            mean += data[i];
        }
        mean /= n;

        // Calculate the slope of the best-fit line
        float x_mean = (n + 1) / 2.0;
        float y_mean = mean;
        float numerator = 0.0;
        float denominator = 0.0;
        for (size_t i = 0; i < n; i++) {
            numerator += (i + 1 - x_mean) * (data[i] - y_mean);
            denominator += (i + 1 - x_mean) * (i + 1 - x_mean);
        }
        float slope = numerator / denominator;

        // Subtract the best-fit line from the data points to get the detrended data
        for (size_t i = 0; i < n; i++) {
            data[i] = data[i] - (slope * (i + 1));
        }

        // Calculate the mean of the detrended data
        float detrended_mean = 0.0;
        for (size_t i = 0; i < n; i++) {
            detrended_mean += data[i];
        }
        detrended_mean /= n;

        // Subtract the mean of the detrended data from each element
        for (size_t i = 0; i < n; i++) {
            data[i] -= detrended_mean;
        }
    }

    static fvec detrend(const fvec& data) {
        auto ret = data;
        detrend(ret.data(), ret.size());
        return ret;
    }

};

struct fmat {
    ei_matrix* mat = nullptr;
    fmat(size_t rows, size_t cols) {
        mat = new ei_matrix(rows, cols);
        assert(mat);
    }

    ~fmat() {
        delete mat;
    }

    void resize(size_t rows, size_t cols) {
        delete mat;
        mat = new ei_matrix(rows, cols);
    }

    float* operator[](size_t i) {
        if (mat == nullptr || i >= mat->rows) {
            return nullptr;
        }
        return mat->get_row_ptr(i);
    }

    void fill(float x) {
        if (mat == nullptr) {
            return;
        }
        for (size_t i = 0; i < mat->rows; i++) {
            for (size_t j = 0; j < mat->cols; j++) {
                (*this)[i][j] = x;
            }
        }
    }

    void fill_col(size_t col, float x) {
        if (mat == nullptr) {
            return;
        }
        for (size_t i = 0; i < mat->rows; i++) {
            (*this)[i][col] = x;
        }
    }

    void fill_row(size_t row, float x) {
        if (mat == nullptr) {
            return;
        }
        for (size_t i = 0; i < mat->cols; i++) {
            (*this)[row][i] = x;
        }
    }
};
} // namespace ei

#endif // _EIDSP_NUMPY_H_
