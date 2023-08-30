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

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_TENSAILFOW_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_TENSAILFOW_H_

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSAIFLOW)

#include "model-parameters/model_metadata.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"
#include "edge-impulse-sdk/classifier/ei_run_dsp.h"

#include "mcu.h"

extern "C" void infer(const void *impulse_arg, uint32_t* time, uint32_t* cycles);
int8_t *processed_features;

#ifdef EI_CLASSIFIER_NN_OUTPUT_COUNT
int8_t infer_result[EI_CLASSIFIER_NN_OUTPUT_COUNT];
#else
int8_t infer_result[EI_CLASSIFIER_LABEL_COUNT];
#endif

extern "C" void get_data(const void *impulse_arg, int8_t *in_buf_0, uint16_t in_buf_0_dim_0, uint16_t in_buf_0_dim_1, uint16_t in_buf_0_dim_2)
{
    ei_impulse_t *impulse = (ei_impulse_t *) impulse_arg;

    if ((impulse->sensor == EI_CLASSIFIER_SENSOR_CAMERA) &&
        ((impulse->dsp_blocks_size == 1) ||
        (impulse->dsp_blocks[0].extract_fn == extract_image_features))) {

        memcpy(in_buf_0, processed_features, impulse->nn_input_frame_size);
    }
}

extern "C" void post_process(const void *impulse_arg, int8_t *out_buf_0, int8_t *out_buf_1)
{
    ei_impulse_t *impulse = (ei_impulse_t *) impulse_arg;

    #ifdef EI_CLASSIFIER_NN_OUTPUT_COUNT
    memcpy(infer_result, out_buf_0, impulse->tflite_output_features_count);
    #else
    memcpy(infer_result, out_buf_0, impulse->label_count);
    #endif
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
    ei_config_tensaiflow_graph_t *graph_config = (ei_config_tensaiflow_graph_t*)block_config->graph_config;

    if (impulse->object_detection) {
        ei_printf("ERR: Object detection models are not supported with TensaiFlow\n");
        return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
    }

    uint64_t ctx_start_us = ei_read_timer_us();
    uint32_t time, cycles;

    /* Run tensaiflow inference */
    infer((const void *)impulse, &time, &cycles);

    // Inference results returned by post_process() and copied into infer_results

    result->timing.classification_us = ei_read_timer_us() - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);

    for (uint32_t ix = 0; ix < impulse->label_count; ix++) {
        float value;
        // Dequantize the output if it is int8
        value = static_cast<float>(infer_result[ix] - graph_config->output_zeropoint) *
            graph_config->output_scale;

        if (debug) {
            ei_printf("%s:\t", impulse->categories[ix]);
            ei_printf_float(value);
            ei_printf("\n");
        }
        result->classification[ix].label = impulse->categories[ix];
        result->classification[ix].value = value;
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
    ei_learning_block_config_tflite_graph_t *block_config = (ei_learning_block_config_tflite_graph_t*)config_ptr;
    ei_config_tensaiflow_graph_t *graph_config = (ei_config_tensaiflow_graph_t*)block_config->graph_config;

    uint64_t ctx_start_us;
    uint64_t dsp_start_us = ei_read_timer_us();

    ei::matrix_i8_t features_matrix(1, impulse->nn_input_frame_size);
    processed_features = (int8_t *) features_matrix.buffer;

    // run DSP process and quantize automatically
    int ret = extract_image_features_quantized(signal, &features_matrix, impulse->dsp_blocks[0].config, graph_config->input_scale, graph_config->input_zeropoint,
        impulse->frequency, impulse->learning_blocks[0].image_scaling);
    if (ret != EIDSP_OK) {
        ei_printf("ERR: Failed to run DSP process (%d)\n", ret);
        return EI_IMPULSE_DSP_ERROR;
    }

    if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
        return EI_IMPULSE_CANCELED;
    }

    result->timing.dsp_us = ei_read_timer_us() - dsp_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);

    if (debug) {
        ei_printf("Features (%d ms.): ", result->timing.dsp);
        for (size_t ix = 0; ix < features_matrix.cols; ix++) {
            ei_printf_float((features_matrix.buffer[ix] - graph_config->input_zeropoint) * graph_config->input_scale);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    uint32_t time, cycles;
    ctx_start_us = ei_read_timer_us();

    /* Run tensaiflow inference */
    infer((const void *)impulse, &time, &cycles);

    // Inference results returned by post_process() and copied into infer_results

    EI_IMPULSE_ERROR fill_res = EI_IMPULSE_OK;

    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
            case EI_CLASSIFIER_LAST_LAYER_FOMO: {
                #if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
                    fill_res = fill_result_struct_i8_fomo(impulse, result, infer_result,
                        graph_config->output_zeropoint, graph_config->output_scale,
                        impulse->fomo_output_size, impulse->fomo_output_size);
                #else
                    ei_printf("ERR: TensaiFlow does not support float32 inference\n");
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
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
        #if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
            fill_res = fill_result_struct_i8(impulse, result, infer_result,
                graph_config->output_zeropoint, graph_config->output_scale, debug);
        #else
            ei_printf("ERR: TensaiFlow does not support float32 inference\n");
            return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
        #endif
    }

    if (fill_res != EI_IMPULSE_OK) {
        return fill_res;
    }


    result->timing.classification_us = ei_read_timer_us() - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);
    return EI_IMPULSE_OK;

}

#endif // #if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSAILFOW)
#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_TENSAILFOW_H_
