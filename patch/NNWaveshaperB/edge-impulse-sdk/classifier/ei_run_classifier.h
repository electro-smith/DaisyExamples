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

#ifndef _EDGE_IMPULSE_RUN_CLASSIFIER_H_
#define _EDGE_IMPULSE_RUN_CLASSIFIER_H_

#include "model-parameters/model_metadata.h"

#include "ei_run_dsp.h"
#include "ei_classifier_types.h"
#include "ei_signal_with_axes.h"
#include "ei_performance_calibration.h"

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

#if EI_CLASSIFIER_HAS_ANOMALY == 1
#include "inferencing_engines/anomaly.h"
#endif

#if defined(EI_CLASSIFIER_HAS_SAMPLER) && EI_CLASSIFIER_HAS_SAMPLER == 1
#include "ei_sampler.h"
#endif

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE) && (EI_CLASSIFIER_COMPILED != 1)
#include "edge-impulse-sdk/classifier/inferencing_engines/tflite_micro.h"
#elif EI_CLASSIFIER_COMPILED == 1
#include "edge-impulse-sdk/classifier/inferencing_engines/tflite_eon.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_FULL
#include "edge-impulse-sdk/classifier/inferencing_engines/tflite_full.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE_TIDL
#include "edge-impulse-sdk/classifier/inferencing_engines/tflite_tidl.h"
#elif (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSORRT)
#include "edge-impulse-sdk/classifier/inferencing_engines/tensorrt.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSAIFLOW
#include "edge-impulse-sdk/classifier/inferencing_engines/tensaiflow.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_DRPAI
#include "edge-impulse-sdk/classifier/inferencing_engines/drpai.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_AKIDA
#include "edge-impulse-sdk/classifier/inferencing_engines/akida.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_ONNX_TIDL
#include "edge-impulse-sdk/classifier/inferencing_engines/onnx_tidl.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_MEMRYX
#include "edge-impulse-sdk/classifier/inferencing_engines/memryx.h"
#elif EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_NONE
// noop
#else
#error "Unknown inferencing engine"
#endif

#include "model-parameters/model_variables.h"

#if ECM3532
void*   __dso_handle = (void*) &__dso_handle;
#endif

#ifdef __cplusplus
namespace {
#endif // __cplusplus

/* Function prototypes ----------------------------------------------------- */
extern "C" EI_IMPULSE_ERROR run_inference(const ei_impulse_t *impulse, ei::matrix_t *fmatrix, ei_impulse_result_t *result, bool debug);
extern "C" EI_IMPULSE_ERROR run_classifier_image_quantized(const ei_impulse_t *impulse, signal_t *signal, ei_impulse_result_t *result, bool debug);
static EI_IMPULSE_ERROR can_run_classifier_image_quantized(const ei_impulse_t *impulse, ei_learning_block_t block_ptr);

/* Private variables ------------------------------------------------------- */

static uint64_t classifier_continuous_features_written = 0;
static RecognizeEvents *avg_scores = NULL;

/* Private functions ------------------------------------------------------- */

/* These functions (up to Public functions section) are not exposed to end-user,
therefore changes are allowed. */

#if EI_CLASSIFIER_LOAD_IMAGE_SCALING
static const float torch_mean[] = { 0.485, 0.456, 0.406 };
static const float torch_std[] = { 0.229, 0.224, 0.225 };

static EI_IMPULSE_ERROR scale_fmatrix(ei_learning_block_t *block, ei::matrix_t *fmatrix) {
    if (block->image_scaling == EI_CLASSIFIER_IMAGE_SCALING_TORCH) {
        // @todo; could we write some faster vector math here?
        for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix += 3) {
            fmatrix->buffer[ix + 0] = (fmatrix->buffer[ix + 0] - torch_mean[0]) / torch_std[0];
            fmatrix->buffer[ix + 1] = (fmatrix->buffer[ix + 1] - torch_mean[1]) / torch_std[1];
            fmatrix->buffer[ix + 2] = (fmatrix->buffer[ix + 2] - torch_mean[2]) / torch_std[2];
        }
    }
    else if (block->image_scaling == EI_CLASSIFIER_IMAGE_SCALING_0_255) {
        int scale_res = numpy::scale(fmatrix, 255.0f);
        if (scale_res != EIDSP_OK) {
            ei_printf("ERR: Failed to scale matrix (%d)\n", scale_res);
            return EI_IMPULSE_DSP_ERROR;
        }
    }

    return EI_IMPULSE_OK;
}

static EI_IMPULSE_ERROR unscale_fmatrix(ei_learning_block_t *block, ei::matrix_t *fmatrix) {
    if (block->image_scaling == EI_CLASSIFIER_IMAGE_SCALING_TORCH) {
        // @todo; could we write some faster vector math here?
        for (size_t ix = 0; ix < fmatrix->rows * fmatrix->cols; ix += 3) {
            fmatrix->buffer[ix + 0] = (fmatrix->buffer[ix + 0] * torch_std[0]) + torch_mean[0];
            fmatrix->buffer[ix + 1] = (fmatrix->buffer[ix + 1] * torch_std[1]) + torch_mean[1];
            fmatrix->buffer[ix + 2] = (fmatrix->buffer[ix + 2] * torch_std[2]) + torch_mean[2];
        }
    }
    else if (block->image_scaling == EI_CLASSIFIER_IMAGE_SCALING_0_255) {
        int scale_res = numpy::scale(fmatrix, 1 / 255.0f);
        if (scale_res != EIDSP_OK) {
            ei_printf("ERR: Failed to scale matrix (%d)\n", scale_res);
            return EI_IMPULSE_DSP_ERROR;
        }
    }
    return EI_IMPULSE_OK;
}
#endif

/**
 * @brief      Do inferencing over the processed feature matrix
 *
 * @param      impulse  struct with information about model and DSP
 * @param      fmatrix  Processed matrix
 * @param      result   Output classifier results
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
extern "C" EI_IMPULSE_ERROR run_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    bool debug = false)
{
    for (size_t ix = 0; ix < impulse->learning_blocks_size; ix++) {
        ei_learning_block_t block = impulse->learning_blocks[ix];

#if EI_CLASSIFIER_LOAD_IMAGE_SCALING
        EI_IMPULSE_ERROR scale_res = scale_fmatrix(&block, fmatrix);
        if (scale_res != EI_IMPULSE_OK) {
            return scale_res;
        }
#endif

        EI_IMPULSE_ERROR res = block.infer_fn(impulse, fmatrix, result, block.config, debug);
        if (res != EI_IMPULSE_OK) {
            return res;
        }

#if EI_CLASSIFIER_LOAD_IMAGE_SCALING
        // undo scaling
        scale_res = unscale_fmatrix(&block, fmatrix);
        if (scale_res != EI_IMPULSE_OK) {
            return scale_res;
        }
#endif
    }

    if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
        return EI_IMPULSE_CANCELED;
    }

    return EI_IMPULSE_OK;
}

/**
 * @brief      Process a complete impulse
 *
 * @param      impulse  struct with information about model and DSP
 * @param      signal   Sample data
 * @param      result   Output classifier results
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
extern "C" EI_IMPULSE_ERROR process_impulse(const ei_impulse_t *impulse,
                                            signal_t *signal,
                                            ei_impulse_result_t *result,
                                            bool debug = false)
{

#if (EI_CLASSIFIER_QUANTIZATION_ENABLED == 1 && (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSAIFLOW || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_ONNX_TIDL)) || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_DRPAI
    // Shortcut for quantized image models
    ei_learning_block_t block = impulse->learning_blocks[0];
    if (can_run_classifier_image_quantized(impulse, block) == EI_IMPULSE_OK) {
        return run_classifier_image_quantized(impulse, signal, result, debug);
    }
#endif

    memset(result, 0, sizeof(ei_impulse_result_t));

    ei::matrix_t features_matrix(1, impulse->nn_input_frame_size);

    uint64_t dsp_start_us = ei_read_timer_us();

    size_t out_features_index = 0;

    for (size_t ix = 0; ix < impulse->dsp_blocks_size; ix++) {
        ei_model_dsp_t block = impulse->dsp_blocks[ix];

        if (out_features_index + block.n_output_features > impulse->nn_input_frame_size) {
            ei_printf("ERR: Would write outside feature buffer\n");
            return EI_IMPULSE_DSP_ERROR;
        }

        ei::matrix_t fm(1, block.n_output_features, features_matrix.buffer + out_features_index);

#if EIDSP_SIGNAL_C_FN_POINTER
        if (block.axes_size != impulse->raw_samples_per_frame) {
            ei_printf("ERR: EIDSP_SIGNAL_C_FN_POINTER can only be used when all axes are selected for DSP blocks\n");
            return EI_IMPULSE_DSP_ERROR;
        }
        int ret = block.extract_fn(signal, &fm, block.config, impulse->frequency);
#else
        SignalWithAxes swa(signal, block.axes, block.axes_size, impulse);
        int ret = block.extract_fn(swa.get_signal(), &fm, block.config, impulse->frequency);
#endif

        if (ret != EIDSP_OK) {
            ei_printf("ERR: Failed to run DSP process (%d)\n", ret);
            return EI_IMPULSE_DSP_ERROR;
        }

        if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
            return EI_IMPULSE_CANCELED;
        }

        out_features_index += block.n_output_features;
    }

    result->timing.dsp_us = ei_read_timer_us() - dsp_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);

    if (debug) {
        ei_printf("Features (%d ms.): ", result->timing.dsp);
        for (size_t ix = 0; ix < features_matrix.cols; ix++) {
            ei_printf_float(features_matrix.buffer[ix]);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    if (debug) {
        ei_printf("Running impulse...\n");
    }

    return run_inference(impulse, &features_matrix, result, debug);

}

/**
 * @brief      Process a complete impulse for continuous inference
 *
 * @param      impulse  struct with information about model and DSP
 * @param      signal   Sample data
 * @param      result   Output classifier results
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
extern "C" EI_IMPULSE_ERROR process_impulse_continuous(const ei_impulse_t *impulse,
                                            signal_t *signal,
                                            ei_impulse_result_t *result,
                                            bool debug,
                                            bool enable_maf)
{

    static ei::matrix_t static_features_matrix(1, impulse->nn_input_frame_size);
    if (!static_features_matrix.buffer) {
        return EI_IMPULSE_ALLOC_FAILED;
    }

    memset(result, 0, sizeof(ei_impulse_result_t));

    EI_IMPULSE_ERROR ei_impulse_error = EI_IMPULSE_OK;

    uint64_t dsp_start_us = ei_read_timer_us();

    size_t out_features_index = 0;
    bool is_mfcc = false;
    bool is_mfe = false;
    bool is_spectrogram = false;

    for (size_t ix = 0; ix < impulse->dsp_blocks_size; ix++) {
        ei_model_dsp_t block = impulse->dsp_blocks[ix];

        if (out_features_index + block.n_output_features > impulse->nn_input_frame_size) {
            ei_printf("ERR: Would write outside feature buffer\n");
            return EI_IMPULSE_DSP_ERROR;
        }

        ei::matrix_t fm(1, block.n_output_features,
                        static_features_matrix.buffer + out_features_index);

        int (*extract_fn_slice)(ei::signal_t *signal, ei::matrix_t *output_matrix, void *config, const float frequency, matrix_size_t *out_matrix_size);

        /* Switch to the slice version of the mfcc feature extract function */
        if (block.extract_fn == extract_mfcc_features) {
            extract_fn_slice = &extract_mfcc_per_slice_features;
            is_mfcc = true;
        }
        else if (block.extract_fn == extract_spectrogram_features) {
            extract_fn_slice = &extract_spectrogram_per_slice_features;
            is_spectrogram = true;
        }
        else if (block.extract_fn == extract_mfe_features) {
            extract_fn_slice = &extract_mfe_per_slice_features;
            is_mfe = true;
        }
        else {
            ei_printf("ERR: Unknown extract function, only MFCC, MFE and spectrogram supported\n");
            return EI_IMPULSE_DSP_ERROR;
        }

        matrix_size_t features_written;

#if EIDSP_SIGNAL_C_FN_POINTER
        if (block.axes_size != impulse->raw_samples_per_frame) {
            ei_printf("ERR: EIDSP_SIGNAL_C_FN_POINTER can only be used when all axes are selected for DSP blocks\n");
            return EI_IMPULSE_DSP_ERROR;
        }
        int ret = extract_fn_slice(signal, &fm, block.config, impulse->frequency, &features_written);
#else
        SignalWithAxes swa(signal, block.axes, block.axes_size, impulse);
        int ret = extract_fn_slice(swa.get_signal(), &fm, block.config, impulse->frequency, &features_written);
#endif

        if (ret != EIDSP_OK) {
            ei_printf("ERR: Failed to run DSP process (%d)\n", ret);
            return EI_IMPULSE_DSP_ERROR;
        }

        if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
            return EI_IMPULSE_CANCELED;
        }

        classifier_continuous_features_written += (features_written.rows * features_written.cols);

        out_features_index += block.n_output_features;
    }

    result->timing.dsp_us = ei_read_timer_us() - dsp_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);

    if (debug) {
        ei_printf("\r\nFeatures (%d ms.): ", result->timing.dsp);
        for (size_t ix = 0; ix < static_features_matrix.cols; ix++) {
            ei_printf_float(static_features_matrix.buffer[ix]);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    if (classifier_continuous_features_written >= impulse->nn_input_frame_size) {
        dsp_start_us = ei_read_timer_us();
        ei::matrix_t classify_matrix(1, impulse->nn_input_frame_size);

        /* Create a copy of the matrix for normalization */
        for (size_t m_ix = 0; m_ix < impulse->nn_input_frame_size; m_ix++) {
            classify_matrix.buffer[m_ix] = static_features_matrix.buffer[m_ix];
        }

        if (is_mfcc) {
            calc_cepstral_mean_and_var_normalization_mfcc(&classify_matrix, impulse->dsp_blocks[0].config);
        }
        else if (is_spectrogram) {
            calc_cepstral_mean_and_var_normalization_spectrogram(&classify_matrix, impulse->dsp_blocks[0].config);
        }
        else if (is_mfe) {
            calc_cepstral_mean_and_var_normalization_mfe(&classify_matrix, impulse->dsp_blocks[0].config);
        }
        result->timing.dsp_us += ei_read_timer_us() - dsp_start_us;
        result->timing.dsp = (int)(result->timing.dsp_us / 1000);

        if (debug) {
            ei_printf("Running impulse...\n");
        }

        ei_impulse_error = run_inference(impulse, &classify_matrix, result, debug);

#if EI_CLASSIFIER_CALIBRATION_ENABLED
        if (impulse->sensor == EI_CLASSIFIER_SENSOR_MICROPHONE) {
            if((void *)avg_scores != NULL && enable_maf == true) {
                if (enable_maf && !impulse->calibration.is_configured) {
                    // perfcal is not configured, print msg first time
                    static bool has_printed_msg = false;

                    if (!has_printed_msg) {
                        ei_printf("WARN: run_classifier_continuous, enable_maf is true, but performance calibration is not configured.\n");
                        ei_printf("       Previously we'd run a moving-average filter over your outputs in this case, but this is now disabled.\n");
                        ei_printf("       Go to 'Performance calibration' in your Edge Impulse project to configure post-processing parameters.\n");
                        ei_printf("       (You can enable this from 'Dashboard' if it's not visible in your project)\n");
                        ei_printf("\n");

                        has_printed_msg = true;
                    }
                }
                else {
                    // perfcal is configured
                    static bool has_printed_msg = false;

                    if (!has_printed_msg) {
                        ei_printf("\nPerformance calibration is configured for your project. If no event is detected, all values are 0.\r\n\n");
                        has_printed_msg = true;
                    }

                    int label_detected = avg_scores->trigger(result->classification);

                    if (avg_scores->should_boost()) {
                        for (int i = 0; i < impulse->label_count; i++) {
                            if (i == label_detected) {
                                result->classification[i].value = 1.0f;
                            }
                            else {
                                result->classification[i].value = 0.0f;
                            }
                        }
                    }
                }
            }
        }
#endif
    }
    else {
        if (!impulse->object_detection) {
            for (int i = 0; i < impulse->label_count; i++) {
                // set label correctly in the result struct if we have no results (otherwise is nullptr)
                result->classification[i].label = impulse->categories[(uint32_t)i];
            }
        }
    }

    return ei_impulse_error;


}

/**
 * Check if the current impulse could be used by 'run_classifier_image_quantized'
 */
__attribute__((unused)) static EI_IMPULSE_ERROR can_run_classifier_image_quantized(const ei_impulse_t *impulse, ei_learning_block_t block_ptr) {

    if (impulse->inferencing_engine != EI_CLASSIFIER_TFLITE
        && impulse->inferencing_engine != EI_CLASSIFIER_TENSAIFLOW
        && impulse->inferencing_engine != EI_CLASSIFIER_DRPAI
        && impulse->inferencing_engine != EI_CLASSIFIER_ONNX_TIDL) // check later
    {
        return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
    }

    if (impulse->has_anomaly == 1){
        return EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES;
    }

        // Check if we have tflite graph
    if (block_ptr.infer_fn != run_nn_inference) {
        return EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES;
    }

    // Check if we have a quantized NN Input layer (input is always quantized for DRP-AI)
    ei_learning_block_config_tflite_graph_t *block_config = (ei_learning_block_config_tflite_graph_t*)block_ptr.config;
    if (block_config->quantized != 1) {
        return EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES;
    }

    // And if we have one DSP block which operates on images...
    if (impulse->dsp_blocks_size != 1 || impulse->dsp_blocks[0].extract_fn != extract_image_features) {
        return EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES;
    }

    return EI_IMPULSE_OK;
}

#if EI_CLASSIFIER_QUANTIZATION_ENABLED == 1 && (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSAIFLOW || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_DRPAI || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_ONNX_TIDL)

/**
 * Special function to run the classifier on images, only works on TFLite models (either interpreter, EON, tensaiflow, drpai, tidl, memryx)
 * that allocates a lot less memory by quantizing in place. This only works if 'can_run_classifier_image_quantized'
 * returns EI_IMPULSE_OK.
 */
extern "C" EI_IMPULSE_ERROR run_classifier_image_quantized(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false)
{
    memset(result, 0, sizeof(ei_impulse_result_t));

    return run_nn_inference_image_quantized(impulse, signal, result, impulse->learning_blocks[0].config, debug);
}

#endif // #if EI_CLASSIFIER_QUANTIZATION_ENABLED == 1 && (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TFLITE || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_TENSAIFLOW || EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_DRPAI)

/* Public functions ------------------------------------------------------- */

/* Thread carefully: public functions are not to be changed
to preserve backwards compatibility. */

/**
 * @brief      Init static vars
 */
extern "C" void run_classifier_init()
{

    classifier_continuous_features_written = 0;
    ei_dsp_clear_continuous_audio_state();

#if EI_CLASSIFIER_CALIBRATION_ENABLED

    const ei_impulse_t impulse = ei_default_impulse;
    const ei_model_performance_calibration_t *calibration = &impulse.calibration;

    if(calibration != NULL) {
        avg_scores = new RecognizeEvents(calibration,
            impulse.label_count, impulse.slice_size, impulse.interval_ms);
    }
#endif
}

/**
 * @brief      Init static vars, for multi-model support
 */
__attribute__((unused)) void run_classifier_init(const ei_impulse_t *impulse)
{
    classifier_continuous_features_written = 0;
    ei_dsp_clear_continuous_audio_state();

#if EI_CLASSIFIER_CALIBRATION_ENABLED
    const ei_model_performance_calibration_t *calibration = &impulse->calibration;

    if(calibration != NULL) {
        avg_scores = new RecognizeEvents(calibration,
            impulse->label_count, impulse->slice_size, impulse->interval_ms);
    }
#endif
}

extern "C" void run_classifier_deinit(void)
{
    if((void *)avg_scores != NULL) {
        delete avg_scores;
    }
}

/**
 * @brief      Fill the complete matrix with sample slices. From there, run inference
 *             on the matrix.
 *
 * @param      signal  Sample data
 * @param      result  Classification output
 * @param[in]  debug   Debug output enable boot
 *
 * @return     The ei impulse error.
 */
extern "C" EI_IMPULSE_ERROR run_classifier_continuous(
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false,
    bool enable_maf = true)
{
    const ei_impulse_t impulse = ei_default_impulse;
    return process_impulse_continuous(&impulse, signal, result, debug, enable_maf);
}

/**
 * @brief      Fill the complete matrix with sample slices. From there, run impulse
 *             on the matrix.
 *
 * @param      impulse struct with information about model and DSP
 * @param      signal  Sample data
 * @param      result  Classification output
 * @param[in]  debug   Debug output enable boot
 *
 * @return     The ei impulse error.
 */
__attribute__((unused)) EI_IMPULSE_ERROR run_classifier_continuous(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false,
    bool enable_maf = true)
{
    return process_impulse_continuous(impulse, signal, result, debug, enable_maf);
}

/**
 * Run the classifier over a raw features array
 * @param raw_features Raw features array
 * @param raw_features_size Size of the features array
 * @param result Object to store the results in
 * @param debug Whether to show debug messages (default: false)
 */
extern "C" EI_IMPULSE_ERROR run_classifier(
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false)
{
    const ei_impulse_t impulse = ei_default_impulse;
    return process_impulse(&impulse, signal, result, debug);
}

/**
 * Run the impulse over a raw features array
 * @param impulse struct with information about model and DSP
 * @param raw_features Raw features array
 * @param raw_features_size Size of the features array
 * @param result Object to store the results in
 * @param debug Whether to show debug messages (default: false)
 */
__attribute__((unused)) EI_IMPULSE_ERROR run_classifier(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false)
{
    return process_impulse(impulse, signal, result, debug);
}

/* Deprecated functions ------------------------------------------------------- */

/* These functions are being deprecated and possibly will be removed or moved in future.
Do not use these - if possible, change your code to reflect the upcoming changes. */

#if EIDSP_SIGNAL_C_FN_POINTER == 0

/**
 * Run the impulse, if you provide an instance of sampler it will also persist the data for you
 * @param sampler Instance to an **initialized** sampler
 * @param result Object to store the results in
 * @param data_fn Function to retrieve data from sensors
 * @param debug Whether to log debug messages (default false)
 */
__attribute__((unused)) EI_IMPULSE_ERROR run_impulse(
#if defined(EI_CLASSIFIER_HAS_SAMPLER) && EI_CLASSIFIER_HAS_SAMPLER == 1
        EdgeSampler *sampler,
#endif
        ei_impulse_result_t *result,
#ifdef __MBED__
        mbed::Callback<void(float*, size_t)> data_fn,
#else
        std::function<void(float*, size_t)> data_fn,
#endif
        bool debug = false) {

    const ei_impulse_t impulse = ei_default_impulse;

    float *x = (float*)calloc(impulse.dsp_input_frame_size, sizeof(float));
    if (!x) {
        return EI_IMPULSE_OUT_OF_MEMORY;
    }

    uint64_t next_tick = 0;

    uint64_t sampling_us_start = ei_read_timer_us();

    // grab some data
    for (int i = 0; i < (int)impulse.dsp_input_frame_size; i += impulse.raw_samples_per_frame) {
        uint64_t curr_us = ei_read_timer_us() - sampling_us_start;

        next_tick = curr_us + (impulse.interval_ms * 1000);

        data_fn(x + i, impulse.raw_samples_per_frame);
#if defined(EI_CLASSIFIER_HAS_SAMPLER) && EI_CLASSIFIER_HAS_SAMPLER == 1
        if (sampler != NULL) {
            sampler->write_sensor_data(x + i, impulse.raw_samples_per_frame);
        }
#endif

        if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
            free(x);
            return EI_IMPULSE_CANCELED;
        }

        while (next_tick > ei_read_timer_us() - sampling_us_start);
    }

    result->timing.sampling = (ei_read_timer_us() - sampling_us_start) / 1000;

    signal_t signal;
    int err = numpy::signal_from_buffer(x, impulse.dsp_input_frame_size, &signal);
    if (err != 0) {
        free(x);
        ei_printf("ERR: signal_from_buffer failed (%d)\n", err);
        return EI_IMPULSE_DSP_ERROR;
    }

    EI_IMPULSE_ERROR r = run_classifier(&signal, result, debug);
    free(x);
    return r;
}

#if defined(EI_CLASSIFIER_HAS_SAMPLER) && EI_CLASSIFIER_HAS_SAMPLER == 1
/**
 * Run the impulse, does not persist data
 * @param result Object to store the results in
 * @param data_fn Function to retrieve data from sensors
 * @param debug Whether to log debug messages (default false)
 */
__attribute__((unused)) EI_IMPULSE_ERROR run_impulse(
        ei_impulse_result_t *result,
#ifdef __MBED__
        mbed::Callback<void(float*, size_t)> data_fn,
#else
        std::function<void(float*, size_t)> data_fn,
#endif
        bool debug = false) {
    return run_impulse(NULL, result, data_fn, debug);
}
#endif

#endif // #if EIDSP_SIGNAL_C_FN_POINTER == 0

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _EDGE_IMPULSE_RUN_CLASSIFIER_H_
