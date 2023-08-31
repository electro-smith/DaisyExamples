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

#ifndef _EDGE_IMPULSE_RUN_CLASSIFIER_C_H_
#define _EDGE_IMPULSE_RUN_CLASSIFIER_C_H_

#if defined(__cplusplus) && EI_C_LINKAGE == 1

#include "ei_run_classifier.h"

/**
 * Run the classifier over a raw features array
 * @param raw_features Raw features array
 * @param raw_features_size Size of the features array
 * @param result Object to store the results in
 * @param debug Whether to show debug messages (default: false)
 */
extern "C" EI_IMPULSE_ERROR ei_run_classifier(
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false);

#endif // #if defined(__cplusplus) && EI_C_LINKAGE == 1

#endif // _EDGE_IMPULSE_RUN_CLASSIFIER_H_
