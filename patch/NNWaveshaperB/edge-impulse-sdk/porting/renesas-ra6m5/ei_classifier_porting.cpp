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

/* Includes */
#include "../ei_classifier_porting.h"
#if EI_PORTING_RENESASRA65 == 1

#include "common_utils.h"
#include <stdarg.h>
#include <stdlib.h>
#include <cstdio>
#include "unistd.h"
#include "peripheral/uart_ep.h"
#include <math.h>

// extern "C" void Serial_Out(char *string, int length);
extern "C" uint64_t timer_get_ms(void);
// extern "C" fsp_err_t uart_print_user_msg(uint8_t *p_msg);

__attribute__((weak)) EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}

/**
 * Cancelable sleep, can be triggered with signal from other thread
 */
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {

    uint64_t start_time = ei_read_timer_ms();

    while(start_time + time_ms > ei_read_timer_ms()){};

    return EI_IMPULSE_OK;
}

uint64_t ei_read_timer_ms() {

    return timer_get_ms();//Timer_getMs();
}

uint64_t ei_read_timer_us() {

    return timer_get_ms()*1000;
}

__attribute__((weak)) char ei_getchar()
{
    // dummy implementation
    char ch = 0;
    return ch;
}

__attribute__((weak)) void ei_printf(const char *format, ...) {

    char buffer[256] = {0};
    int length;
    va_list myargs;
    va_start(myargs, format);
    length = vsnprintf(buffer, sizeof(buffer), format, myargs);
    va_end(myargs);

    if (length > 0){
        uart_print_user_msg((uint8_t *)buffer);
    }

}

__attribute__((weak)) void ei_printf_float(float f) {
    float n = f;

    static double PRECISION = 0.00001;
    static int MAX_NUMBER_STRING_SIZE = 32;

    char s[MAX_NUMBER_STRING_SIZE];

    if (n == 0.0) {
        strcpy(s, "0");
    }
    else {
        int digit, m;
        char *c = s;
        int neg = (n < 0);
        if (neg) {
            n = -n;
        }
        // calculate magnitude
        m = log10(n);
        if (neg) {
            *(c++) = '-';
        }
        if (m < 1.0) {
            m = 0;
        }
        // convert the number
        while (n > PRECISION || m >= 0) {
            double weight = pow(10.0, m);
            if (weight > 0 && !isinf(weight)) {
                digit = floor(n / weight);
                n -= (digit * weight);
                *(c++) = '0' + digit;
            }
            if (m == 0 && n > 0) {
                *(c++) = '.';
            }
            m--;
        }
        *(c) = '\0';
    }

    ei_printf("%s", s);
}

/**
 *
 * @param c
 */
void ei_putchar(char c)
{
    ei_printf("%c", c);
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

#endif // EI_PORTING_RENESASRA65 == 1
