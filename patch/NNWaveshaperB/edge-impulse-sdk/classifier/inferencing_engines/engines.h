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

#ifndef _EI_CLASSIFIER_ENGINES_H_
#define _EI_CLASSIFIER_ENGINES_H_

#include "edge-impulse-sdk/classifier/ei_model_types.h"

EI_IMPULSE_ERROR run_kmeans_anomaly(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug);

EI_IMPULSE_ERROR run_gmm_anomaly(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug);

EI_IMPULSE_ERROR run_nn_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug);

int extract_tflite_eon_features(signal_t *signal, matrix_t *output_matrix,
                                void *config_ptr, const float frequency);

int extract_tflite_features(signal_t *signal, matrix_t *output_matrix,
                            void *config_ptr, const float frequency);

#endif // _EI_CLASSIFIER_ENGINES_H_s