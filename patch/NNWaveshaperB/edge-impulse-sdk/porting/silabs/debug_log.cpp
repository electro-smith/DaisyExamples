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

#include "../ei_classifier_porting.h"
#if EI_PORTING_SILABS == 1

#include "edge-impulse-sdk/tensorflow/lite/micro/debug_log.h"
#include <stdio.h>
#include <stdarg.h>

// Redirect TFLite DebugLog to ei_printf
#if defined(__cplusplus) && EI_C_LINKAGE == 1
extern "C"
#endif // defined(__cplusplus) && EI_C_LINKAGE == 1
void DebugLog(const char* s) {
    ei_printf("%s", s);
}

#endif // EI_PORTING_SILABS == 1
