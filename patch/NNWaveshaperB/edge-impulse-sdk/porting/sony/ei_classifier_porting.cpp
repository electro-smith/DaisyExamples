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
#if EI_PORTING_SONY_SPRESENSE == 1

#include <stdarg.h>
#include <stdlib.h>
#include <cstdio>

extern "C" void spresense_time_cb(uint32_t *sec, uint32_t *nano);
extern "C" void spresense_putchar(char cChar);
extern "C" char spresense_getchar(void);

__attribute__((weak)) EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}

/**
 * Cancelable sleep, can be triggered with signal from other thread
 */
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {

    uint64_t end_ms = ei_read_timer_ms() + time_ms;

    while(end_ms > ei_read_timer_ms()){};

    return EI_IMPULSE_OK;
}

uint64_t ei_read_timer_ms() {

    uint64_t time_ms;
    uint32_t seconds, nano_seconds;

    spresense_time_cb(&seconds, &nano_seconds);

    time_ms = (seconds * 1000) + (nano_seconds / 1000000);
    return time_ms;
}

uint64_t ei_read_timer_us() {

    uint64_t time_us;
    uint32_t seconds, nano_seconds;

    spresense_time_cb(&seconds, &nano_seconds);

    time_us = (seconds * 1000000) + (nano_seconds / 1000);
    return time_us;
}

__attribute__((weak)) void ei_printf(const char *format, ...) {

    char buffer[256];
    int length;
    va_list myargs;
    va_start(myargs, format);
    length = vsprintf(buffer, format, myargs);
    va_end(myargs);

    for(int i = 0; i < length; i++) {
        spresense_putchar(buffer[i]);
    }
}

__attribute__((weak)) void ei_printf_float(float f) {
    ei_printf("%f", f);
}

/**
 * @brief      Write single character to serial output
 *
 * @param[in]  cChar  The character
 */
__attribute__((weak)) void ei_putchar(char cChar)
{
    spresense_putchar(cChar);
}

__attribute__((weak)) char ei_getchar(void)
{
    return spresense_getchar();
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

#endif // EI_PORTING_SONY_SPRESENSE == 1
