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

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_HELPER_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_HELPER_H_

#include "edge-impulse-sdk/classifier/ei_quantize.h"
#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL) || (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE)

#if EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL
#include <thread>
#include "tensorflow-lite/tensorflow/lite/c/common.h"
#include "tensorflow-lite/tensorflow/lite/interpreter.h"
#include "tensorflow-lite/tensorflow/lite/kernels/register.h"
#include "tensorflow-lite/tensorflow/lite/model.h"
#include "tensorflow-lite/tensorflow/lite/optional_debug_tools.h"
#endif // EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL

#if EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE
#include <cmath>
#include "edge-impulse-sdk/tensorflow/lite/micro/all_ops_resolver.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/micro_error_reporter.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/micro_interpreter.h"
#include "edge-impulse-sdk/tensorflow/lite/schema/schema_generated.h"
#endif // EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE

EI_IMPULSE_ERROR fill_input_tensor_from_matrix(
    matrix_t *fmatrix,
    TfLiteTensor *input
) {
    const size_t matrix_els = fmatrix->rows * fmatrix->cols;

    switch (input->type) {
        case kTfLiteFloat32: {
            if (input->bytes / 4 != matrix_els) {
                ei_printf("ERR: input tensor has size %d, but input matrix has has size %d\n",
                    (int)input->bytes / 4, (int)matrix_els);
                return EI_IMPULSE_INVALID_SIZE;
            }

            for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix++) {
                input->data.f[ix] = fmatrix->buffer[ix];
            }
            break;
        }
        case kTfLiteInt8: {
            if (input->bytes != matrix_els) {
                ei_printf("ERR: input tensor has size %d, but input matrix has has size %d\n",
                    (int)input->bytes, (int)matrix_els);
                return EI_IMPULSE_INVALID_SIZE;
            }

            for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix++) {
                float val = (float)fmatrix->buffer[ix];
                input->data.int8[ix] = static_cast<int8_t>(
                    pre_cast_quantize(val, input->params.scale, input->params.zero_point, true));
            }
            break;
        }
        case kTfLiteUInt8: {
            if (input->bytes != matrix_els) {
                ei_printf("ERR: input tensor has size %d, but input matrix has has size %d\n",
                    (int)input->bytes, (int)matrix_els);
                return EI_IMPULSE_INVALID_SIZE;
            }

            for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix++) {
                float val = (float)fmatrix->buffer[ix];
                input->data.uint8[ix] = static_cast<uint8_t>(
                    pre_cast_quantize(val, input->params.scale, input->params.zero_point, false));            }
            break;
        }
        default: {
            ei_printf("ERR: Cannot handle input type (%d)\n", input->type);
            return EI_IMPULSE_INPUT_TENSOR_WAS_NULL;
        }
    }

    return EI_IMPULSE_OK;
}

EI_IMPULSE_ERROR fill_input_tensor_from_signal(
    signal_t *signal,
    TfLiteTensor *input
) {
    switch (input->type) {
        case kTfLiteFloat32: {
            if (input->bytes / 4 != signal->total_length) {
                ei_printf("ERR: input tensor has size %d, but signal has size %d\n",
                    (int)input->bytes / 4, (int)signal->total_length);
                return EI_IMPULSE_INVALID_SIZE;
            }

            auto x = signal->get_data(0, signal->total_length, input->data.f);
            if (x != EIDSP_OK) {
                return EI_IMPULSE_DSP_ERROR;
            }
            break;
        }
        case kTfLiteInt8:
        case kTfLiteUInt8: {
            // we don't have a good signaling way here (this is DSP blocks where
            // we don't understand the input very well; guess whether this is an RGB input)
            bool is_rgb = input->bytes / 3 == signal->total_length;

            if (!is_rgb) {
                // otherwise expect an exact match in length
                if (input->bytes != signal->total_length) {
                    ei_printf("ERR: input tensor has size %d, but signal has size %d\n",
                        (int)input->bytes, (int)signal->total_length);
                    return EI_IMPULSE_INVALID_SIZE;
                }
            }

            float scale = input->params.scale;
            int zero_point = input->params.zero_point;
            if (scale == 0.0f) { // not quantized?
                if (is_rgb) {
                    scale = 0.003921568859368563f;
                }
                else {
                    scale = 1.0f;
                }

                if (input->type == kTfLiteInt8 && zero_point == 0) {
                    zero_point = -128;
                }
            }

            size_t output_ix = 0;
            const size_t page_size = 1024;

            // buffered read from the signal
            size_t bytes_left = signal->total_length;
            for (size_t ix = 0; ix < signal->total_length; ix += page_size) {
                size_t elements_to_read = bytes_left > page_size ? page_size : bytes_left;

                matrix_t input_matrix(elements_to_read, 1);
                if (!input_matrix.buffer) {
                    return EI_IMPULSE_ALLOC_FAILED;
                }
                signal->get_data(ix, elements_to_read, input_matrix.buffer);

                for (size_t jx = 0; jx < elements_to_read; jx++) {
                    if (is_rgb) {
                        uint32_t value = static_cast<uint32_t>(input_matrix.buffer[jx]);

                        // fast code path
                        if (scale == 0.003921568859368563f && zero_point == -128) {
                            int32_t r = static_cast<int32_t>(value >> 16 & 0xff);
                            int32_t g = static_cast<int32_t>(value >> 8 & 0xff);
                            int32_t b = static_cast<int32_t>(value & 0xff);

                            if (input->type == kTfLiteInt8) {
                                input->data.int8[output_ix++] = static_cast<int8_t>(r + zero_point);
                                input->data.int8[output_ix++] = static_cast<int8_t>(g + zero_point);
                                input->data.int8[output_ix++] = static_cast<int8_t>(b + zero_point);
                            }
                            else {
                                input->data.uint8[output_ix++] = static_cast<uint8_t>(r + zero_point);
                                input->data.uint8[output_ix++] = static_cast<uint8_t>(g + zero_point);
                                input->data.uint8[output_ix++] = static_cast<uint8_t>(b + zero_point);
                            }
                        }
                        // slow code path
                        else {
                            float r = static_cast<float>(value >> 16 & 0xff) / 255.0f;
                            float g = static_cast<float>(value >> 8 & 0xff) / 255.0f;
                            float b = static_cast<float>(value & 0xff) / 255.0f;

                            if (input->type == kTfLiteInt8) {
                                input->data.int8[output_ix++] = static_cast<int8_t>(round(r / scale) + zero_point);
                                input->data.int8[output_ix++] = static_cast<int8_t>(round(g / scale) + zero_point);
                                input->data.int8[output_ix++] = static_cast<int8_t>(round(b / scale) + zero_point);
                            }
                            else {
                                input->data.uint8[output_ix++] = static_cast<uint8_t>(round(r / scale) + zero_point);
                                input->data.uint8[output_ix++] = static_cast<uint8_t>(round(g / scale) + zero_point);
                                input->data.uint8[output_ix++] = static_cast<uint8_t>(round(b / scale) + zero_point);
                            }
                        }
                    }
                    else {
                        float value = input_matrix.buffer[jx];
                        if (input->type == kTfLiteInt8) {
                            input->data.int8[output_ix++] = static_cast<int8_t>(round(value / scale) + zero_point);
                        }
                        else { // uint8
                            input->data.uint8[output_ix++] = static_cast<uint8_t>((value / scale) + zero_point);
                        }
                    }
                }
            }
            break;
        }
        default: {
            ei_printf("ERR: Cannot handle input type (%d)\n", input->type);
            return EI_IMPULSE_INPUT_TENSOR_WAS_NULL;
        }
    }

    return EI_IMPULSE_OK;
}

EI_IMPULSE_ERROR fill_output_matrix_from_tensor(
    TfLiteTensor *output,
    matrix_t *output_matrix
) {
    const size_t matrix_els = output_matrix->rows * output_matrix->cols;

    switch (output->type) {
        case kTfLiteFloat32: {
            if (output->bytes / 4 != matrix_els) {
                ei_printf("ERR: output tensor has size %d, but input matrix has has size %d\n",
                    (int)output->bytes / 4, (int)matrix_els);
                return EI_IMPULSE_INVALID_SIZE;
            }

            memcpy(output_matrix->buffer, output->data.f, output->bytes);
            break;
        }
        case kTfLiteInt8: {
            if (output->bytes != matrix_els) {
                ei_printf("ERR: output tensor has size %d, but input matrix has has size %d\n",
                    (int)output->bytes, (int)matrix_els);
                return EI_IMPULSE_INVALID_SIZE;
            }

            for (size_t ix = 0; ix < output->bytes; ix++) {
                float value = static_cast<float>(output->data.int8[ix] - output->params.zero_point) * output->params.scale;
                output_matrix->buffer[ix] = value;
            }
            break;
        }
        case kTfLiteUInt8: {
            if (output->bytes != matrix_els) {
                ei_printf("ERR: output tensor has size %d, but input matrix has has size %d\n",
                    (int)output->bytes, (int)matrix_els);
                return EI_IMPULSE_INVALID_SIZE;
            }

            for (size_t ix = 0; ix < output->bytes; ix++) {
                float value = static_cast<float>(output->data.uint8[ix] - output->params.zero_point) * output->params.scale;
                output_matrix->buffer[ix] = value;
            }
            break;
        }
        default: {
            ei_printf("ERR: Cannot handle output type (%d)\n", output->type);
            return EI_IMPULSE_OUTPUT_TENSOR_WAS_NULL;
        }
    }

    return EI_IMPULSE_OK;
}

EI_IMPULSE_ERROR fill_result_struct_from_output_tensor_tflite(
    const ei_impulse_t *impulse,
    TfLiteTensor* output,
    TfLiteTensor* labels_tensor,
    TfLiteTensor* scores_tensor,
    ei_impulse_result_t *result,
    bool debug
) {

    EI_IMPULSE_ERROR fill_res = EI_IMPULSE_OK;

    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
            case EI_CLASSIFIER_LAST_LAYER_FOMO: {
                bool int8_output = output->type == TfLiteType::kTfLiteInt8;
                if (int8_output) {
                    fill_res = fill_result_struct_i8_fomo(impulse, result, output->data.int8, output->params.zero_point, output->params.scale,
                        impulse->fomo_output_size, impulse->fomo_output_size);
                }
                else {
                    fill_res = fill_result_struct_f32_fomo(impulse, result, output->data.f,
                        impulse->fomo_output_size, impulse->fomo_output_size);
                }
                break;
            }
#if EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL
            case EI_CLASSIFIER_LAST_LAYER_SSD: {
                if (!scores_tensor->data.f) {
                    return EI_IMPULSE_SCORE_TENSOR_WAS_NULL;
                }
                if (!labels_tensor->data.f) {
                    return EI_IMPULSE_LABEL_TENSOR_WAS_NULL;
                }
                if (output->type == kTfLiteFloat32) {
                    fill_res = fill_result_struct_f32_object_detection(impulse, result, output->data.f, scores_tensor->data.f, labels_tensor->data.f, debug);
                }
                else {
                    ei_printf("ERR: MobileNet SSD does not support quantized inference\n");
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                }
                break;
            }
#else
            case EI_CLASSIFIER_LAST_LAYER_SSD: {
                ei_printf("ERR: MobileNet SSD is not supported in EON or TensorFlow Lite Micro\n");
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
#endif // EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL
            case EI_CLASSIFIER_LAST_LAYER_YOLOV5:
            case EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI: {
                int version = impulse->object_detection_last_layer == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI ?
                    5 : 6;

                if (output->type == kTfLiteInt8) {
                    fill_res = fill_result_struct_quantized_yolov5(
                        impulse,
                        result,
                        version,
                        output->data.int8,
                        output->params.zero_point,
                        output->params.scale,
                        impulse->tflite_output_features_count);
                }
                else if (output->type == kTfLiteUInt8) {
                    fill_res = fill_result_struct_quantized_yolov5(
                        impulse,
                        result,
                        version,
                        output->data.uint8,
                        output->params.zero_point,
                        output->params.scale,
                        impulse->tflite_output_features_count);
                }
                else if (output->type == kTfLiteFloat32) {
                    fill_res = fill_result_struct_f32_yolov5(
                        impulse,
                        result,
                        version,
                        output->data.f,
                        impulse->tflite_output_features_count);
                }
                else {
                    ei_printf("ERR: Invalid output type (%d) for YOLOv5 last layer\n", output->type);
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                }
                break;
            }
            case EI_CLASSIFIER_LAST_LAYER_YOLOX: {
                #if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
                    ei_printf("ERR: YOLOX does not support quantized inference\n");
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                #else
                    fill_res = fill_result_struct_f32_yolox(
                        impulse,
                        result,
                        output->data.f,
                        impulse->tflite_output_features_count);
                #endif
                break;
            }
            case EI_CLASSIFIER_LAST_LAYER_YOLOV7: {
                #if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
                    ei_printf("ERR: YOLOV7 does not support quantized inference\n");
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                #else
                    size_t output_feature_count = 1;
                    for (int ix = 0; ix < output->dims->size; ix++) {
                        output_feature_count *= output->dims->data[ix];
                    }
                    fill_res = fill_result_struct_f32_yolov7(
                        impulse,
                        result,
                        output->data.f,
                        output_feature_count);
                #endif
                break;
            }
            default: {
                ei_printf("ERR: Unsupported object detection last layer (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
        }
    }
    else {
        bool int8_output = output->type == TfLiteType::kTfLiteInt8;
        if (int8_output) {
            fill_res = fill_result_struct_i8(impulse, result, output->data.int8, output->params.zero_point, output->params.scale, debug);
        }
        else {
            fill_res = fill_result_struct_f32(impulse, result, output->data.f, debug);
        }
    }

    return fill_res;
}
#endif // #if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL) || (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE)

#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_TFLITE_HELPER_H_
