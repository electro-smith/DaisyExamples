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

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_TENSORRT_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_TENSORRT_H_

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSORRT)

#include "model-parameters/model_metadata.h"

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"

#include <stdlib.h>
#include "tflite/linux-jetson-nano/libeitrt.h"

EiTrt *ei_trt_handle = NULL;

inline bool file_exists(char *model_file_name)
{
    if (FILE *file = fopen(model_file_name, "r")) {
        fclose(file);
        return true;
    }
    else {
        return false;
    }
}

/**
 * @brief      Do neural network inferencing over the processed feature matrix
 *
 * @param      fmatrix  Processed matrix
 * @param      result   Output classifier results
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
EI_IMPULSE_ERROR run_nn_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    ei_learning_block_config_tflite_graph_t *block_config = (ei_learning_block_config_tflite_graph_t*)config_ptr;
    ei_config_tflite_graph_t *graph_config = (ei_config_tflite_graph_t*)block_config->graph_config;

    #if EI_CLASSIFIER_QUANTIZATION_ENABLED == 1
    #error "TensorRT requires an unquantized network"
    #endif

    static char model_file_name[128];
    snprintf(
        model_file_name,
        128,
        "/tmp/%s-%d-%d.engine",
        impulse->project_name,
        impulse->project_id,
        impulse->deploy_version);

    static bool first_run = !file_exists(model_file_name);
    if (first_run) {
        ei_printf("INFO: Model file '%s' does not exist, creating now. \n", model_file_name);

        FILE *file = fopen(model_file_name, "w");
        if (!file) {
            ei_printf("ERR: TensorRT init failed to open '%s'\n", model_file_name);
            return EI_IMPULSE_TENSORRT_INIT_FAILED;
        }

        if (fwrite(graph_config->model, graph_config->model_size, 1, file) != 1) {
            ei_printf("ERR: TensorRT init fwrite failed.\n");
            return EI_IMPULSE_TENSORRT_INIT_FAILED;
        }

        if (fclose(file) != 0) {
            ei_printf("ERR: TensorRT init fclose failed.\n");
            return EI_IMPULSE_TENSORRT_INIT_FAILED;
        }
    }

    uint32_t out_data_size = 0;

    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
            case EI_CLASSIFIER_LAST_LAYER_FOMO: {
                out_data_size = impulse->tflite_output_features_count;
                break;
            }
            case EI_CLASSIFIER_LAST_LAYER_SSD: {
                ei_printf("ERR: SSD models are not supported using TensorRT \n");
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                break;
            }
            case EI_CLASSIFIER_LAST_LAYER_YOLOV5:
            case EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI: {
                ei_printf("ERR: YOLOv5 models are not supported using TensorRT \n");
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
            default: {
                ei_printf(
                    "ERR: Unsupported object detection last layer (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
        }
    }
    else {
        out_data_size = impulse->label_count;
    }

    float *out_data = (float*)ei_malloc(out_data_size * sizeof(float));
    if (out_data == nullptr) {
        ei_printf("ERR: Cannot allocate memory for output data \n");
    }

    // lazy initialize tensorRT context
    if (ei_trt_handle == nullptr) {
        ei_trt_handle = libeitrt::create_EiTrt(model_file_name, debug);
    }

    uint64_t ctx_start_us = ei_read_timer_us();

    libeitrt::infer(ei_trt_handle, fmatrix->buffer, out_data, out_data_size);

    uint64_t ctx_end_us = ei_read_timer_us();

    result->timing.classification_us = ctx_end_us - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);

    EI_IMPULSE_ERROR fill_res = EI_IMPULSE_OK;

    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
        case EI_CLASSIFIER_LAST_LAYER_FOMO: {
            fill_res = fill_result_struct_f32_fomo(
                impulse,
                result,
                out_data,
                impulse->fomo_output_size,
                impulse->fomo_output_size);
            break;
        }
        case EI_CLASSIFIER_LAST_LAYER_SSD: {
            ei_printf("ERR: SSD models are not supported using TensorRT \n");
            return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            break;
        }
        case EI_CLASSIFIER_LAST_LAYER_YOLOV5:
        case EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI: {
            ei_printf("ERR: YOLOv5 models are not supported using TensorRT \n");
            return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
        }
        default: {
            ei_printf(
                "ERR: Unsupported object detection last layer (%d)\n",
                impulse->object_detection_last_layer);
            return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
        }
        }
    }
    else {
        fill_res = fill_result_struct_f32(impulse, result, out_data, debug);
    }

    ei_free(out_data);

    if (fill_res != EI_IMPULSE_OK) {
        return fill_res;
    }

    return EI_IMPULSE_OK;
}

/**
 * Special function to run the classifier on images, only works on TFLite models (either interpreter or EON or for tensaiflow)
 * that allocates a lot less memory by quantizing in place. This only works if 'can_run_classifier_image_quantized'
 * returns EI_IMPULSE_OK.
 */
EI_IMPULSE_ERROR run_nn_inference_image_quantized(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
}

#endif // #if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSORRT)
#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_TENSORRT_H_
