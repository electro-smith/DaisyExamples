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
#if EI_PORTING_TI == 1

#include <ti/drivers/UART.h>

#include <stdarg.h>
#include <stdlib.h>
#include <cstdio>
#include "unistd.h"

extern "C" void Serial_Out(char *string, int length);
extern "C" uint64_t Timer_getMs(void);

__attribute__((weak)) EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}

/**
 * Cancelable sleep, can be triggered with signal from other thread
 */
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {

    usleep(time_ms * 1000);

    return EI_IMPULSE_OK;
}

uint64_t ei_read_timer_ms() {

    return Timer_getMs();
}

uint64_t ei_read_timer_us() {

    /* TI board hangs when trying to call callback function each micro second */
    return Timer_getMs() * 1000;
}

__attribute__((weak)) void ei_printf(const char *format, ...) {

    char buffer[256];
    int length;
    va_list myargs;
    va_start(myargs, format);
    length = vsnprintf(buffer, 256, format, myargs);
    va_end(myargs);

    Serial_Out(buffer, length);
}

__attribute__((weak)) void ei_printf_float(float f) {
    ei_printf("%f", f);
}

__attribute__((weak)) void ei_putchar(char data)
{
    Serial_Out(&data, 1);
}

__attribute__((weak)) void *ei_malloc(size_t size) {
    return malloc(size);
}

__attribute__((weak)) void *ei_calloc(size_t nitems, size_t size) {
    return calloc(nitems, size);
}

__attribute__((weak)) void ei_free(void *ptr) {
    free(ptr);
}

#if defined(__cplusplus) && EI_C_LINKAGE == 1
extern "C"
#endif
__attribute__((weak)) void DebugLog(const char* s) {
    ei_printf("%s", s);
}

#endif // EI_PORTING_TI == 1
