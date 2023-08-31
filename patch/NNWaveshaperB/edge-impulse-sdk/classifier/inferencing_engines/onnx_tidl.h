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

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_ONNX_TIDL_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_ONNX_TIDL_H_

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_ONNX_TIDL) && (EI_CLASSIFIER_COMPILED != 1)

#include "model-parameters/model_metadata.h"
#if EI_CLASSIFIER_HAS_MODEL_VARIABLES == 1
#include "model-parameters/model_variables.h"
#endif

#include <getopt.h>
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <list>
#include <sys/stat.h>
#include <vector>
#include <string>

#include "itidl_rt.h"
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <onnxruntime/core/providers/tidl/tidl_provider_factory.h>
#include <onnxruntime/core/providers/cpu/cpu_provider_factory.h>

#include <cmath>
#include "edge-impulse-sdk/classifier/ei_aligned_malloc.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"
#include "edge-impulse-sdk/classifier/ei_model_types.h"

#include "onnx-model/tidl-model.h"
#include "utils/model_header_utils.h"

#define TI_PREPROC_DEFAULT_WIDTH 320
#define TI_PREPROC_DEFAULT_HEIGHT 240

using namespace std;

/**
 *  \brief returns time in micro sec
 * @returns void
 */
double getUs(struct timeval t)
{
    return(t.tv_sec * 1000000 + t.tv_usec);
}

/**
 *  \brief  print tensor info
 *  \param  session onnx session
 *  \param  input_node_names input array node names
 * @returns int status
 */
int printTensorInfo(Ort::Session *session, std::vector<const char *> *input_node_names, std::vector<const char *> *output_node_names)
{
    size_t num_input_nodes = (*session).GetInputCount();
    size_t num_output_nodes = (*session).GetOutputCount();
    Ort::TypeInfo type_info = (*session).GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> input_node_dims = tensor_info.GetShape();
    ei_printf("LOG_INFO: number of inputs:%d \n", num_input_nodes);
    ei_printf("LOG_INFO: number of outputs: %d\n", num_output_nodes);
    ei_printf("LOG_INFO: input(0) name: %s\n", (*input_node_names)[0]);

    Ort::TypeInfo type_info_out = (*session).GetOutputTypeInfo(0);
    auto tensor_info_out = type_info_out.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> output_node_dims = tensor_info_out.GetShape();
    /* iterate over all input nodes */
    for (int i = 0; i < num_input_nodes; i++)
    {
        /* print input node names */
        ei_printf("LOG_INFO: Input %d : name=%s\n", i, (*input_node_names)[i]);

        /* print input node types */
        Ort::TypeInfo type_info = (*session).GetInputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType type = tensor_info.GetElementType();
        ei_printf("LOG_INFO: Input %d : type=%d\n", i, type);
        /* print input shapes/dims */
        input_node_dims = tensor_info.GetShape();
        ei_printf("LOG_INFO: Input %d : num_dims=%zu\n", i, input_node_dims.size());
        for (int j = 0; j < input_node_dims.size(); j++)
        {
            ei_printf("LOG_INFO: Input %d : dim %d=%jd\n", i, j, input_node_dims[j]);
        }
    }
    if (num_input_nodes != 1)
    {
        ei_printf("LOG_INFO: supports only single input model \n");
        return EI_IMPULSE_ONNX_ERROR;
    }

    for (int i = 0; i < num_output_nodes; i++)
    {
        /* print output node names */
        ei_printf("LOG_INFO: Output %d : name=%s\n", i, (*output_node_names)[i]);

        /* print output node types */
        Ort::TypeInfo type_info = (*session).GetOutputTypeInfo(i);
        auto tensor_info = type_info.GetTensorTypeAndShapeInfo();

        ONNXTensorElementDataType type = tensor_info.GetElementType();
        ei_printf("LOG_INFO: Output %d : type=%d\n", i, type);
        /* print output shapes/dims */
        output_node_dims = tensor_info.GetShape();
        ei_printf("LOG_INFO: Output %d : num_dims=%zu\n", i, output_node_dims.size());
        for (int j = 0; j < output_node_dims.size(); j++)
        {
            ei_printf("LOG_INFO: Output %d : dim %d=%jd\n", i, j, output_node_dims[j]);
        }
    }
    return EI_IMPULSE_OK;
}

void * allocTensorMem(int size, int accel)
{
    void * ptr = NULL;
    if (accel)
    {
        #ifdef DEVICE_AM62
        ei_printf("LOG_INFO: TIDL Delgate mode is not allowed on AM62 devices...\n");
        ei_printf("LOG_ERROR: Could not allocate memory for a Tensor of size %d \n ", size);
        exit(0);
        #else
        ptr = TIDLRT_allocSharedMem(64, size);
        #endif
    }
    else
    {
        ptr = malloc(size);
    }
    if (ptr == NULL)
    {
        ei_printf("LOG_ERROR: Could not allocate memory for a Tensor of size %d \n ", size);
        exit(0);
    }
    return ptr;
}

void freeTensorMem(void * ptr, int accel)
{
    if (accel)
    {
        #ifndef DEVICE_AM62
        TIDLRT_freeSharedMem(ptr);
        #endif
    }
    else
    {
        free(ptr);
    }
}

/**
 * Setup the ONNX runtime
 *
 * @param      ctx_start_us       Pointer to the start time
 * @param      input              Pointer to input tensor
 * @param      output             Pointer to output tensor
 * @param      micro_interpreter  Pointer to interpreter (for non-compiled models)
 * @param      micro_tensor_arena Pointer to the arena that will be allocated
 *
 * @return  EI_IMPULSE_OK if successful
 */
static EI_IMPULSE_ERROR inference_onnx_setup(
    const ei_impulse_t *impulse,
    uint64_t *ctx_start_us,
    std::vector<Ort::Value>* input_tensors,
    std::vector<Ort::Value>* output_tensors,
    Ort::Session** session_ptr,
    Ort::RunOptions** run_options_ptr,
    Ort::IoBinding** binding_ptr) {

    static bool onnx_first_run = true;
    // Nothing to do after first run
    if (!onnx_first_run) {
       return EI_IMPULSE_OK;
    }

    std::string proj_artifacts_path = "/tmp/" + std::string(impulse->project_name) + "-" + std::to_string(impulse->project_id) + "-" + std::to_string(impulse->deploy_version);

    create_project_if_not_exists(proj_artifacts_path, model_h_files, model_h_files_len);

    std::string proj_model_path = proj_artifacts_path + "/model.onnx";

    ei_printf("test onnx tidl: %s\n", __FUNCTION__);
    #pragma message ( "test onnx tidl: run_nn_inference")

    /* Initialize  enviroment, maintains thread pools and state info */
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    /* Initialize session options */
    Ort::SessionOptions session_options;
    //TODO: from where do we load number of threads?
    session_options.SetIntraOpNumThreads(1);

    ei_printf("LOG_INFO: model accelerated \n");
    c_api_tidl_options *options = (c_api_tidl_options *)malloc(sizeof(c_api_tidl_options));
    OrtStatus *def_status = OrtSessionsOptionsSetDefault_Tidl(options);
    ei_printf("LOG_INFO: artifacts: %s \n", proj_artifacts_path.c_str());
    strcpy(options->artifacts_folder, proj_artifacts_path.c_str());
    if(NULL == options){
        ei_printf("LOG_ERROR: faild to allocate c_api_tidl_options \n");
        return EI_IMPULSE_ONNX_ERROR;
    }
    OrtStatus *status = OrtSessionOptionsAppendExecutionProvider_Tidl(session_options, options);

    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    Ort::AllocatorWithDefaultOptions allocator;

    /* ORT Session */
    Ort::Session* session = new Ort::Session(env, proj_model_path.c_str(), session_options);
    *session_ptr = session;
    ei_printf("LOG_INFO: Loaded model %s\n", proj_model_path.c_str());

    /* Input information */
    size_t num_input_nodes = session->GetInputCount();
    std::vector<const char *> input_node_names(num_input_nodes);
    Ort::TypeInfo type_info = session->GetInputTypeInfo(0);
    auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> input_node_dims = tensor_info.GetShape();
    ONNXTensorElementDataType input_tensor_type = tensor_info.GetElementType();

    /* output information */
    size_t num_output_nodes = session->GetOutputCount();
    std::vector<const char *> output_node_names(num_output_nodes);
    for (int i = 0; i < num_output_nodes; i++)
    {
        output_node_names[i] = session->GetOutputName(i, allocator);
    }
    for (int i = 0; i < num_input_nodes; i++)
    {
        input_node_names[i] = session->GetInputName(i, allocator);
    }

    type_info = session->GetOutputTypeInfo(0);
    auto output_tensor_info = type_info.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> output_node_dims = output_tensor_info.GetShape();
    size_t output_tensor_size = output_node_dims[1];

    if (EI_IMPULSE_ONNX_ERROR == printTensorInfo(session, &input_node_names, &output_node_names)) {
        ei_printf("LOG_ERROR: print tensor information failed!\n");
        return EI_IMPULSE_ONNX_ERROR;
    }

    ssize_t input_tensor_size_bytes;
    /* simplify ... using known dim values to calculate size */
    size_t input_tensor_size = impulse->nn_input_frame_size;
    void *inData;
    if (input_tensor_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
    {
        input_tensor_size_bytes = input_tensor_size * sizeof(float);
        inData = allocTensorMem(input_tensor_size_bytes, true);
    }
    else if (input_tensor_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8)
    {
        input_tensor_size_bytes = input_tensor_size * sizeof(uint8_t);
        inData = allocTensorMem(input_tensor_size_bytes, true);
    }
    else
    {
        ei_printf("LOG_ERROR: indata type not supported yet \n ");
        return EI_IMPULSE_ONNX_ERROR;
    }
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value input_tensor = Ort::Value::CreateTensor(memory_info, inData, input_tensor_size_bytes, input_node_dims.data(), 4, input_tensor_type);
    input_tensors->push_back(std::move(input_tensor));

    Ort::RunOptions* run_options = new Ort::RunOptions();
    *run_options_ptr = run_options;
    run_options->SetRunLogVerbosityLevel(2);
    auto output_tensors_warm_up = session->Run(*run_options, input_node_names.data(), input_tensors->data(), 1, output_node_names.data(), num_output_nodes);

    //void *outData = allocTensorMem(output_tensor_size * sizeof(float), true);
    Ort::IoBinding* binding = new Ort::IoBinding(*session);
    *binding_ptr = binding;
    binding->BindInput(input_node_names[0], (*input_tensors)[0]);

    for(int idx=0; idx < num_output_nodes; idx++)
    {
        auto node_dims = output_tensors_warm_up[idx].GetTypeInfo().GetTensorTypeAndShapeInfo().GetShape();
        size_t tensor_size = 1;
        for(int j = node_dims.size()-1; j >= 0; j--)
        {
            tensor_size *= node_dims[j];
        }
        ONNXTensorElementDataType tensor_type  = output_tensors_warm_up[idx].GetTypeInfo().GetTensorTypeAndShapeInfo().GetElementType();
        if(tensor_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
        {
            tensor_size *= sizeof(float);
        }
        else if(tensor_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8)
        {
            tensor_size *= sizeof(uint8_t);
        }
        else if(tensor_type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64)
        {
            tensor_size *= sizeof(int64_t);
        }
        else
        {
            ei_printf("LOG_ERROR: Un Supported output tensor_type\n");
            return EI_IMPULSE_ONNX_ERROR;
        }

        void * outData = allocTensorMem(tensor_size, true);
        auto output_tensor = Ort::Value::CreateTensor(memory_info, (void *)outData, tensor_size, node_dims.data(), node_dims.size(),tensor_type);
        output_tensors->push_back(std::move(output_tensor));
        binding->BindOutput(output_node_names[idx], (*output_tensors)[idx]);
    }

    onnx_first_run = false;

    return EI_IMPULSE_OK;
}

/**
 * Run ONNX model
 *
 * @param   ctx_start_us    Start time of the setup function (see above)
 * @param   output_tensors  Output tensors
 * @param   session         ONNX session
 * @param   run_options     ONNX run options
 * @param   binding         IO bindings
 * @param   debug           Whether to print debug info
 *
 * @return  EI_IMPULSE_OK if successful
 */
static EI_IMPULSE_ERROR inference_onnx_run(const ei_impulse_t *impulse,
    uint64_t ctx_start_us,
    std::vector<Ort::Value>* input_tensors,
    std::vector<Ort::Value>* output_tensors,
    Ort::Session* session,
    Ort::RunOptions* run_options,
    Ort::IoBinding* binding,
    ei_impulse_result_t *result,
    bool debug) {

    session->Run(*run_options, *binding);

    uint64_t ctx_end_us = ei_read_timer_us();
    result->timing.classification_us = ctx_end_us - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);

    ONNXTensorElementDataType output_tensor_type = (*output_tensors).at(0).GetTypeInfo().GetTensorTypeAndShapeInfo().GetElementType();
    void *out_data = output_tensors->front().GetTensorMutableData<void>();

    // get output features count from model
    auto node_dims = (*output_tensors).at(0).GetTypeInfo().GetTensorTypeAndShapeInfo().GetShape();
    size_t output_tensor_features_count = 1;
    for(int j = node_dims.size()-1; j >= 0; j--)
    {
        output_tensor_features_count *= node_dims[j];
    }

    // Read the predicted y value from the model's output tensor
    if (debug) {
        ei_printf("Predictions (time: %d ms.):\n", result->timing.classification);
    }

    EI_IMPULSE_ERROR fill_res = EI_IMPULSE_OK;

    // NOTE: for now only yolox object detection supported
    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
            case EI_CLASSIFIER_LAST_LAYER_YOLOX: {
                #if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
                    ei_printf("ERR: YOLOX does not support quantized inference\n");
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                #else

                    if (debug) {
                        ei_printf("YOLOX OUTPUT (%d ms.): ", result->timing.classification);
                        for (size_t ix = 0; ix < output_tensor_features_count; ix++) {
                            ei_printf_float(((float*)out_data)[ix]);
                            ei_printf(" ");
                        }
                        ei_printf("\n");
                    }
                    fill_res = fill_result_struct_f32_yolox_detect(
                        impulse,
                        result,
                        (float*)out_data,
                        output_tensor_features_count);
                #endif
                break;
            }
            default: {
                ei_printf("ERR: Unsupported object detection last layer (%d)\n",
                    impulse->object_detection_last_layer);
                break;
            }
        }
    }
    else {
#if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1

    switch (output_tensor_type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: {
            fill_res = fill_result_struct_i8(impulse, result, (int8_t*)out_data, impulse->tflite_output_zeropoint, impulse->tflite_output_scale, debug);
            break;
        }
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8: {
            fill_res = fill_result_struct_i8(impulse, result, (int8_t*)out_data, impulse->tflite_output_zeropoint, impulse->tflite_output_scale, debug);
            break;
        }
        default: {
            ei_printf("ERR: Cannot handle output type (%d)\n", output_tensor_type);
            return EI_IMPULSE_OUTPUT_TENSOR_WAS_NULL;
        }
    }

#else
    switch (output_tensor_type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: {
            fill_res = fill_result_struct_f32(impulse, result, (float*)out_data, debug);
            break;
        }
        default: {
            ei_printf("ERR: Cannot handle output type (%d)\n", output_tensor_type);
            return EI_IMPULSE_OUTPUT_TENSOR_WAS_NULL;
        }
    }
#endif
    }

    ///* freeing shared mem*/
    //for (size_t i = 0; i < output_tensors->size(); i++)
    //{
    //    void *ptr = (*output_tensors)[i].GetTensorMutableData<void>();
    //    freeTensorMem(ptr, true);
    //}
    //for (size_t i = 0; i < input_tensors->size(); i++)
    //{
    //    void *ptr = (*input_tensors)[i].GetTensorMutableData<void>();
    //    freeTensorMem(ptr, true);
    //}

    if (fill_res != EI_IMPULSE_OK) {
        return fill_res;
    }

    return EI_IMPULSE_OK;
}

/**
 * @brief      Do neural network inferencing over the processed feature matrix
 *
 * @param      fmatrix  Processed matrix >> features [array of features] this is input
 * @param      result   Output classifier results >> output
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
EI_IMPULSE_ERROR run_nn_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *afmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    static std::vector<Ort::Value> input_tensors;
    static std::vector<Ort::Value> output_tensors;
    static Ort::Session* session;
    static Ort::RunOptions* run_options;
    static Ort::IoBinding* binding;
    uint64_t ctx_start_us;

    ei_printf("test onnx tidl: %s\n", __FUNCTION__);
    #pragma message ( "test onnx tidl: run_nn_inference")

    EI_IMPULSE_ERROR init_res = inference_onnx_setup(impulse,
        &ctx_start_us,
        &input_tensors,
        &output_tensors,
        &session,
        &run_options,
        &binding);

    if (init_res != EI_IMPULSE_OK || session == NULL || run_options == NULL ||
        binding == NULL) {
        ei_printf("LOG_ERROR: ONNX inference setup failed!\n");
        return EI_IMPULSE_ONNX_ERROR;
    }

    uint64_t dsp_chw_start_us;
    dsp_chw_start_us = ei_read_timer_us();

    /*
    ** Convert to CHW from HWC
     */
    // features matrix maps around the input tensor to not allocate any memory
    float *input_buffer = input_tensors.front().GetTensorMutableData<float>();
    ei::matrix_t fmatrix(1, impulse->nn_input_frame_size, input_buffer);

    ei_dsp_config_image_t *config = (ei_dsp_config_image_t *)impulse->dsp_blocks[0].config;

    size_t channels = strcmp(config->channels, "Grayscale") == 0 ? 1 : 3;
    size_t height = impulse->input_height;
    size_t width = impulse->input_width;

    int dest_ix = 0;
    for (size_t c=0; c < channels; c++) {
        for (size_t h=0; h < height; h++) {
            for (size_t w=0; w < width; w++) {
                uint32_t src_ix = channels * width * h + w*channels + c;
                fmatrix.buffer[dest_ix++] = afmatrix->buffer[src_ix];
            }
        }
    }

    uint64_t dsp_chw_end_us = ei_read_timer_us();
    result->timing.dsp_us += dsp_chw_end_us - dsp_chw_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);

    if (debug) {
        ei_printf("After Features (%ld us.): ", result->timing.dsp_us);
        for (size_t ix = 0; ix < fmatrix.cols; ix++) {
            ei_printf_float(fmatrix.buffer[ix]);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    ctx_start_us = ei_read_timer_us();
    EI_IMPULSE_ERROR run_res = inference_onnx_run(impulse,
        ctx_start_us,
        &input_tensors,
        &output_tensors,
        session,
        run_options,
        binding,
        result, debug);

    if (run_res != EI_IMPULSE_OK) {
        return run_res;
    }

    return EI_IMPULSE_OK;
}

#if EI_CLASSIFIER_QUANTIZATION_ENABLED == 1
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
    static std::vector<Ort::Value> input_tensors;
    static std::vector<Ort::Value> output_tensors;
    static Ort::Session* session;
    static Ort::RunOptions* run_options;
    static Ort::IoBinding* binding;
    uint64_t ctx_start_us;

    ei_printf("test onnx tidl: %s\n", __FUNCTION__);
    #pragma message ( "test onnx tidl: run_nn_inference_image_quantized")

    EI_IMPULSE_ERROR init_res = inference_onnx_setup(impulse,
        &ctx_start_us,
        &input_tensors, &output_tensors,
        &session,
        &run_options,
        &binding);

    if (init_res != EI_IMPULSE_OK || session == NULL || run_options == NULL ||
        binding == NULL) {
        ei_printf("LOG_ERROR: ONNX inference setup failed!\n");
        return EI_IMPULSE_ONNX_ERROR;
    }

    ONNXTensorElementDataType input_tensor_type = input_tensors.at(0).GetTypeInfo().GetTensorTypeAndShapeInfo().GetElementType();
    if (input_tensor_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8 &&
        input_tensor_type != ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8) {
        return EI_IMPULSE_ONLY_SUPPORTED_FOR_IMAGES;
    }

    uint64_t dsp_start_us = ei_read_timer_us();

    ei::matrix_i8_t a_features_matrix(1, impulse->nn_input_frame_size);

    // run DSP process and quantize automatically
    int ret = extract_image_features_quantized(impulse, signal, &a_features_matrix, impulse->dsp_blocks[0].config, impulse->frequency,
        impulse->learning_blocks[0].image_scaling);
    if (ret != EIDSP_OK) {
        ei_printf("ERR: Failed to run DSP process (%d)\n", ret);
        return EI_IMPULSE_DSP_ERROR;
    }

    if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
        return EI_IMPULSE_CANCELED;
    }

    if (debug) {
        ei_printf("Before Features: ");
        for (size_t ix = 0; ix < a_features_matrix.cols; ix++) {
            ei_printf("%d", (uint8_t)a_features_matrix.buffer[ix]);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    /*
    ** Convert to CHW from HWC
     */
    // features matrix maps around the input tensor to not allocate any memory
    uint8_t *input_buffer = input_tensors.front().GetTensorMutableData<uint8_t>();
    ei::matrix_i8_t features_matrix(1, impulse->nn_input_frame_size, (int8_t*) input_buffer);

    ei_dsp_config_image_t *config = (ei_dsp_config_image_t *)impulse->dsp_blocks[0].config;

    size_t channels = strcmp(config->channels, "Grayscale") == 0 ? 1 : 3;
    size_t height = impulse->input_height;
    size_t width = impulse->input_width;

    int dest_ix = 0;
    for (size_t c=0; c < channels; c++) {
        for (size_t h=0; h < height; h++) {
            for (size_t w=0; w < width; w++) {
                uint32_t src_ix = channels * width * h + w*channels + c;
                features_matrix.buffer[dest_ix++] = a_features_matrix.buffer[src_ix];
            }
        }
    }

    if (debug) {
        ei_printf("After Features: ");
        for (size_t ix = 0; ix < features_matrix.cols; ix++) {
            ei_printf("%d", (uint8_t)features_matrix.buffer[ix]);
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    result->timing.dsp_us = ei_read_timer_us() - dsp_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);

    if (debug) {
        ei_printf("Features (%d ms.): ", result->timing.dsp);
        for (size_t ix = 0; ix < features_matrix.cols; ix++) {
            // expects scale of (1/255) and zeropoint of 0
            ei_printf_float(static_cast<float>(((uint8_t)features_matrix.buffer[ix] - impulse->tflite_input_zeropoint) * impulse->tflite_input_scale));
            ei_printf(" ");
        }
        ei_printf("\n");
    }

    ctx_start_us = ei_read_timer_us();
    EI_IMPULSE_ERROR run_res = inference_onnx_run(impulse,
        ctx_start_us,
        &input_tensors,
        &output_tensors,
        session,
        run_options,
        binding,
        result, debug);

    if (run_res != EI_IMPULSE_OK) {
        return run_res;
    }

    return EI_IMPULSE_OK;
}
#endif // EI_CLASSIFIER_QUANTIZATION_ENABLED == 1

#endif // #if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_ONNX_TIDL) && (EI_CLASSIFIER_COMPILED != 1)
#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_ONNX_TIDL_H_
