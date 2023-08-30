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

#if defined(__cplusplus) && EI_C_LINKAGE == 1

#include "ei_run_classifier_c.h"

/**
 * This function definition is just there to make sure
 * that the symbol is not removed from the library.
 */
EI_IMPULSE_ERROR ei_run_classifier(
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug) {

    return run_classifier(signal, result, debug);
}

#endif // #if defined(__cplusplus) && EI_C_LINKAGE == 1
