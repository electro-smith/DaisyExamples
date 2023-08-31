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

#ifndef _EDGE_IMPULSE_MODEL_TYPES_H_
#define _EDGE_IMPULSE_MODEL_TYPES_H_

#include <stdint.h>

#include "edge-impulse-sdk/classifier/ei_classifier_types.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#if EI_CLASSIFIER_USE_FULL_TFLITE
#include "tensorflow-lite/tensorflow/lite/c/common.h"
#else
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#endif // EI_CLASSIFIER_USE_FULL_TFLITE

#define EI_CLASSIFIER_NONE                       255
#define EI_CLASSIFIER_UTENSOR                    1
#define EI_CLASSIFIER_TFLITE                     2
#define EI_CLASSIFIER_CUBEAI                     3
#define EI_CLASSIFIER_TFLITE_FULL                4
#define EI_CLASSIFIER_TENSAIFLOW                 5
#define EI_CLASSIFIER_TENSORRT                   6
#define EI_CLASSIFIER_DRPAI                      7
#define EI_CLASSIFIER_TFLITE_TIDL                8
#define EI_CLASSIFIER_AKIDA                      9
#define EI_CLASSIFIER_SYNTIANT                   10
#define EI_CLASSIFIER_ONNX_TIDL                  11
#define EI_CLASSIFIER_MEMRYX                     12

#define EI_CLASSIFIER_SENSOR_UNKNOWN             -1
#define EI_CLASSIFIER_SENSOR_MICROPHONE          1
#define EI_CLASSIFIER_SENSOR_ACCELEROMETER       2
#define EI_CLASSIFIER_SENSOR_CAMERA              3
#define EI_CLASSIFIER_SENSOR_9DOF                4
#define EI_CLASSIFIER_SENSOR_ENVIRONMENTAL       5
#define EI_CLASSIFIER_SENSOR_FUSION              6

// These must match the enum values in TensorFlow Lite's "TfLiteType"
#define EI_CLASSIFIER_DATATYPE_FLOAT32           1
#define EI_CLASSIFIER_DATATYPE_INT8              9

#define EI_CLASSIFIER_LAST_LAYER_UNKNOWN               -1
#define EI_CLASSIFIER_LAST_LAYER_SSD                   1
#define EI_CLASSIFIER_LAST_LAYER_FOMO                  2
#define EI_CLASSIFIER_LAST_LAYER_YOLOV5                3
#define EI_CLASSIFIER_LAST_LAYER_YOLOX                 4
#define EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI       5
#define EI_CLASSIFIER_LAST_LAYER_YOLOV7                6

#define EI_CLASSIFIER_IMAGE_SCALING_NONE          0
#define EI_CLASSIFIER_IMAGE_SCALING_0_255         1
#define EI_CLASSIFIER_IMAGE_SCALING_TORCH         2

struct ei_impulse;

typedef struct {
    uint16_t implementation_version;
    bool is_configured;
    uint32_t average_window_duration_ms;
    float detection_threshold;
    uint32_t suppression_ms;
    uint32_t suppression_flags;
} ei_model_performance_calibration_t;

typedef struct {
    size_t n_output_features;
    int (*extract_fn)(ei::signal_t *signal, ei::matrix_t *output_matrix, void *config, const float frequency);
    void *config;
    uint8_t *axes;
    size_t axes_size;
} ei_model_dsp_t;

typedef struct {
    float *centroid;
    float max_error;
} ei_classifier_anom_cluster_t;

typedef struct {
    EI_IMPULSE_ERROR (*infer_fn)(const ei_impulse *impulse, ei::matrix_t *fmatrix, ei_impulse_result_t *result, void *config, bool debug);
    void *config;
    int image_scaling;
} ei_learning_block_t;

typedef struct {
    uint16_t implementation_version;
    uint8_t input_datatype;
    bool input_quantized;
    float input_scale;
    float input_zeropoint;
    uint8_t output_datatype;
    bool output_quantized;
    float output_scale;
    float output_zeropoint;
} ei_config_tensaiflow_graph_t;

typedef struct {
    uint16_t implementation_version;
    const unsigned char *model;
    size_t model_size;
    size_t arena_size;
} ei_config_tflite_graph_t;

typedef struct {
    uint16_t implementation_version;
    TfLiteStatus (*model_init)(void*(*alloc_fnc)(size_t, size_t));
    TfLiteStatus (*model_invoke)();
    TfLiteStatus (*model_reset)(void (*free)(void* ptr));
    TfLiteStatus (*model_input)(int, TfLiteTensor*);
    TfLiteStatus (*model_output)(int, TfLiteTensor*);
} ei_config_tflite_eon_graph_t;

typedef struct {
    uint16_t implementation_version;
    uint32_t block_id;
    /* object detection */
    bool object_detection;
    int8_t object_detection_last_layer;
    uint8_t output_data_tensor;
    uint8_t output_labels_tensor;
    uint8_t output_score_tensor;
    /* tflite graph params */
    bool quantized;
    bool compiled;
    /* tflite graph config pointer */
    void *graph_config;
} ei_learning_block_config_tflite_graph_t;

typedef struct {
    uint16_t implementation_version;
    const uint16_t *anom_axis;
    uint16_t anom_axes_size;
    const ei_classifier_anom_cluster_t *anom_clusters;
    uint16_t anom_cluster_count;
    const float *anom_scale;
    const float *anom_mean;
} ei_learning_block_config_anomaly_kmeans_t;

typedef struct {
    uint16_t implementation_version;
    const uint16_t *anom_axis;
    uint16_t anom_axes_size;
    void* graph_config;
} ei_learning_block_config_anomaly_gmm_t;

typedef struct ei_impulse {
    /* project details */
    uint32_t project_id;
    const char *project_owner;
    const char *project_name;
    uint32_t deploy_version;

    /* DSP details */
    uint32_t nn_input_frame_size;
    uint32_t raw_sample_count;
    uint32_t raw_samples_per_frame;
    uint32_t dsp_input_frame_size;
    uint32_t input_width;
    uint32_t input_height;
    uint32_t input_frames;
    float interval_ms;
    float frequency;
    size_t dsp_blocks_size;
    ei_model_dsp_t *dsp_blocks;

    /* object detection */
    bool object_detection;
    uint16_t object_detection_count;
    float object_detection_threshold;
    int8_t object_detection_last_layer;
    uint32_t fomo_output_size;
    uint32_t tflite_output_features_count;

    /* learning blocks */
    const size_t learning_blocks_size;
    const ei_learning_block_t *learning_blocks;

    /* inference parameters */
    uint32_t inferencing_engine;

    /* sensors and on-device inference */
    uint32_t sensor;
    const char *fusion_string;
    uint32_t slice_size;
    uint32_t slices_per_model_window;

    /* output details */
    bool has_anomaly;
    uint16_t label_count;
    const ei_model_performance_calibration_t calibration;
    const char **categories;
} ei_impulse_t;

typedef struct {
    uint32_t block_id;
    uint16_t implementation_version;
    int axes;
    const unsigned char *model;
    size_t model_size;
    size_t arena_size;
} ei_dsp_config_tflite_t;

typedef struct {
    uint32_t block_id;
    uint16_t implementation_version;
    int axes;
    TfLiteStatus (*init_fn)(void*(*alloc_fnc)(size_t, size_t));
    TfLiteStatus (*invoke_fn)();
    TfLiteStatus (*reset_fn)(void (*free)(void* ptr));
    TfLiteStatus (*input_fn)(int, TfLiteTensor*);
    TfLiteStatus (*output_fn)(int, TfLiteTensor*);
} ei_dsp_config_tflite_eon_t;

#endif // _EDGE_IMPULSE_MODEL_TYPES_H_
