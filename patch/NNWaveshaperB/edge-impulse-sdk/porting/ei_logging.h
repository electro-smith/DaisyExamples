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

#ifndef _EI_LOGGING_H_
#define _EI_LOGGING_H_

#include <stdint.h>
#include <stdarg.h>

#include "ei_classifier_porting.h"

#define EI_LOG_LEVEL_NONE 0 /*!< No log output */
#define EI_LOG_LEVEL_ERROR 1 /*!< Critical errors, software module can not recover on its own */
#define EI_LOG_LEVEL_WARNING 2 /*!< Error conditions from which recovery measures have been taken */
#define EI_LOG_LEVEL_INFO 3 /*!< Information messages which describe normal flow of events */
#define EI_LOG_LEVEL_DEBUG 4 /*!< Extra information which is not necessary for normal use (values, pointers, sizes, etc). */

// if we do not want ANY logging, setting EI_LOG_LEVEL to EI_LOG_LEVEL_NONE
// will not generate any code according to
// https://stackoverflow.com/a/25021889

#define EI_LOGE(format, ...) (void)0
#define EI_LOGW(format, ...) (void)0
#define EI_LOGI(format, ...) (void)0
#define EI_LOGD(format, ...) (void)0

#ifndef EI_LOG_LEVEL
    #define EI_LOG_LEVEL EI_LOG_LEVEL_NONE
#endif

#if defined(__cplusplus) && EI_C_LINKAGE == 1
extern "C"
#endif // defined(__cplusplus) && EI_C_LINKAGE == 1

const char *debug_msgs[] =
{
    "NONE", // this one will never show
    "ERR",
    "WARNING",
    "INFO",
    "DEBUG"
};

#if EI_LOG_LEVEL >= EI_LOG_LEVEL_ERROR
    #ifdef EI_LOGE
    #undef EI_LOGE
    #endif // EI_LOGE
    #define EI_LOGE(format, ...) ei_printf("%s: ",debug_msgs[EI_LOG_LEVEL_ERROR]); ei_printf(format, ##__VA_ARGS__);
#endif

#if EI_LOG_LEVEL >= EI_LOG_LEVEL_WARNING
    #ifdef EI_LOGW
    #undef EI_LOGW
    #endif // EI_LOGW
    #define EI_LOGW(format, ...) ei_printf("%s: ",debug_msgs[EI_LOG_LEVEL_WARNING]); ei_printf(format, ##__VA_ARGS__);
#endif

#if EI_LOG_LEVEL >= EI_LOG_LEVEL_INFO
    #ifdef EI_LOGI
    #undef EI_LOGI
    #endif // EI_LOGI
    #define EI_LOGI(format, ...) ei_printf("%s: ",debug_msgs[EI_LOG_LEVEL_INFO]); ei_printf(format, ##__VA_ARGS__);
#endif

#if EI_LOG_LEVEL >= EI_LOG_LEVEL_DEBUG
    #ifdef EI_LOGD
    #undef EI_LOGD
    #endif // EI_LOGD
    #define EI_LOGD(format, ...) ei_printf("%s: ",debug_msgs[EI_LOG_LEVEL_DEBUG]); ei_printf(format, ##__VA_ARGS__);
#endif

#endif // _EI_LOGGING_H_