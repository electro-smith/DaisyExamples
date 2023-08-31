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
#if EI_PORTING_HIMAX == 1

/* Include ----------------------------------------------------------------- */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "hx_drv_tflm.h"
#include <math.h>


/* Constants ---------------------------------------------------------------- */
#define HIMAX_TIMER_CLK_FREQ_HZ  400000000
#define HIMAX_TIMER_TICK_1SEC    (HIMAX_TIMER_CLK_FREQ_HZ/1)
#define HIMAX_TIMER_TICK_1MSEC   (HIMAX_TIMER_TICK_1SEC/1000)

extern "C" void print_out(const char *format, va_list args);

/* Private variables -------------------------------------------------------- */
static uint64_t system_time_ms = 0;
static uint32_t prev_tick_us = 0;

__attribute__((weak)) EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}

/**
 * Cancelable sleep, can be triggered with signal from other thread
 */
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {
    uint64_t end_delay, cur_time = 0;

    end_delay = (uint64_t)time_ms + ei_read_timer_ms();

    do {
        cur_time = ei_read_timer_ms();
    } while (cur_time < end_delay);

    return EI_IMPULSE_OK;
}

// Should be called at least once every ~10.7 seconds
uint64_t ei_read_timer_ms()
{
    uint32_t tick_us, diff_tick_us, elapsed_time_ms;

    //  handles 32-bit overflows
    hx_drv_tick_get(&tick_us);
    diff_tick_us = (uint32_t)(tick_us - prev_tick_us);

    // integer number of ms elapsed
    elapsed_time_ms = diff_tick_us / HIMAX_TIMER_TICK_1MSEC;

    // update system time and previous tick reference
    if (elapsed_time_ms > 0) {
        system_time_ms += elapsed_time_ms;

        // use the remainder of ms elapsed
        // handles 32-bit overflows
        prev_tick_us = (uint32_t)(tick_us - (diff_tick_us % HIMAX_TIMER_TICK_1MSEC));
    }

    return system_time_ms;
}

uint64_t ei_read_timer_us()
{
    return ei_read_timer_ms() * 1000;
}

void ei_serial_set_baudrate(int baudrate)
{
    hx_drv_uart_initial((HX_DRV_UART_BAUDRATE_E)baudrate);
}

void ei_putchar(char c)
{
    /* Send char to serial output */
    hx_drv_uart_print("%c", c);
}

__attribute__((weak)) void ei_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    print_out(format, args);
    va_end(args);
}

__attribute__((weak)) void ei_printf_float(float f) {
    float n = f;

    static double PRECISION = 0.00001;
    static int MAX_NUMBER_STRING_SIZE = 32;

    char s[MAX_NUMBER_STRING_SIZE];

    if (n == 0.0) {
        ei_printf("0.00000");
    } else {
        int digit, m;  //, m1;
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
        ei_printf("%s", s);
    }
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

#endif // #if EI_PORTING_HIMAX == 1
