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

#ifndef _EDGE_IMPULSE_INFERENCING_ANOMALY_H_
#define _EDGE_IMPULSE_INFERENCING_ANOMALY_H_

#if (EI_CLASSIFIER_HAS_ANOMALY == 1)

#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

#include "edge-impulse-sdk/classifier/ei_classifier_types.h"
#include "edge-impulse-sdk/classifier/ei_aligned_malloc.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/inferencing_engines/engines.h"

#ifdef __cplusplus
namespace {
#endif // __cplusplus

/**
 * Standard scaler, scales all values in the input vector
 * Note that this *modifies* the array in place!
 * @param input Array of input values
 * @param scale Array of scale values (obtain from StandardScaler in Python)
 * @param mean Array of mean values (obtain from StandardScaler in Python)
 * @param input_size Size of input, scale and mean arrays
 */
void standard_scaler(float *input, const float *scale, const float *mean, size_t input_size) {
    for (size_t ix = 0; ix < input_size; ix++) {
        input[ix] = (input[ix] - mean[ix]) / scale[ix];
    }
}

/**
 * Calculate the distance between input vector and the cluster
 * @param input Array of input values (already scaled by standard_scaler)
 * @param input_size Size of the input array
 * @param cluster A cluster (number of centroids should match input_size)
 */
float calculate_cluster_distance(float *input, size_t input_size, const ei_classifier_anom_cluster_t *cluster) {
    // todo: check input_size and centroid size?

    float dist = 0.0f;
    for (size_t ix = 0; ix < input_size; ix++) {
        dist += pow(input[ix] - cluster->centroid[ix], 2);
    }
    return sqrt(dist) - cluster->max_error;
}

/**
 * Get minimum distance to a cluster
 * @param input Array of input values (already scaled by standard_scaler)
 * @param input_size Size of the input array
 * @param clusters Array of clusters
 * @param cluster_size Size of cluster array
 */
float get_min_distance_to_cluster(float *input, size_t input_size, const ei_classifier_anom_cluster_t *clusters, size_t cluster_size) {
    float min = 1000.0f;
    for (size_t ix = 0; ix < cluster_size; ix++) {
        float dist = calculate_cluster_distance(input, input_size, &clusters[ix]);
        if (dist < min) {
            min = dist;
        }
    }
    return min;
}

#ifdef __cplusplus
}
#endif // __cplusplus

EI_IMPULSE_ERROR run_kmeans_anomaly(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    ei_learning_block_config_anomaly_kmeans_t *block_config = (ei_learning_block_config_anomaly_kmeans_t*)config_ptr;

    uint64_t anomaly_start_ms = ei_read_timer_ms();

    float *input = (float*)ei_malloc(block_config->anom_axes_size * sizeof(float));
    if (!input) {
        ei_printf("Failed to allocate memory for anomaly input buffer");
        return EI_IMPULSE_OUT_OF_MEMORY;
    }

    for (size_t ix = 0; ix < block_config->anom_axes_size; ix++) {
        input[ix] = fmatrix->buffer[block_config->anom_axis[ix]];
    }
    standard_scaler(input, block_config->anom_scale, block_config->anom_mean, block_config->anom_axes_size);
    float anomaly = get_min_distance_to_cluster(
        input, block_config->anom_axes_size, block_config->anom_clusters, block_config->anom_cluster_count);

    uint64_t anomaly_end_ms = ei_read_timer_ms();

    if (debug) {
        ei_printf("Anomaly score (time: %d ms.): ", static_cast<int>(anomaly_end_ms - anomaly_start_ms));
        ei_printf_float(anomaly);
        ei_printf("\n");
    }

    result->timing.anomaly = anomaly_end_ms - anomaly_start_ms;

    result->anomaly = anomaly;

    ei_free(input);

    return EI_IMPULSE_OK;
}

#if (EI_CLASSIFIER_INFERENCING_ENGINE != EI_CLASSIFIER_NONE)
EI_IMPULSE_ERROR run_gmm_anomaly(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    ei_learning_block_config_anomaly_gmm_t *block_config = (ei_learning_block_config_anomaly_gmm_t*)config_ptr;

    ei_learning_block_config_tflite_graph_t ei_learning_block_config_gmm = {
    .implementation_version = 1,
    .block_id = 0,
    .object_detection = 0,
    .object_detection_last_layer = EI_CLASSIFIER_LAST_LAYER_UNKNOWN,
    .output_data_tensor = 0,
    .output_labels_tensor = 0,
    .output_score_tensor = 0,
    .quantized = 0,
    .compiled = 0,
    .graph_config = block_config->graph_config
    };

    ei_impulse_result_t anomaly_result = {0};
    memset(&anomaly_result, 0, sizeof(ei_impulse_result_t));

    ei::matrix_t features_matrix(1, block_config->anom_axes_size);

    for (size_t ix = 0; ix < block_config->anom_axes_size; ix++) {
        features_matrix.buffer[ix] = fmatrix->buffer[block_config->anom_axis[ix]];
    }

    EI_IMPULSE_ERROR res = run_nn_inference(impulse, &features_matrix, &anomaly_result, (void*)&ei_learning_block_config_gmm, debug);
    if (res != EI_IMPULSE_OK) {
            return res;
        }

    if (debug) {
        ei_printf("Anomaly score (time: %d ms.): ", anomaly_result.timing.classification);
        ei_printf_float(anomaly_result.classification[0].value);
        ei_printf("\n");
    }

    result->timing.anomaly = anomaly_result.timing.classification;

    result->anomaly = anomaly_result.classification[0].value;

    return EI_IMPULSE_OK;
}
#endif // (EI_CLASSIFIER_INFERENCING_ENGINE != EI_CLASSIFIER_NONE)

#endif //#if (EI_CLASSIFIER_HAS_ANOMALY == 1)
#endif // _EDGE_IMPULSE_INFERENCING_ANOMALY_H_
