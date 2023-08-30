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
#if EI_PORTING_INFINEONPSOC62 == 1

#include <stdarg.h>
#include <stdlib.h>
#include <cstdio>
#include "unistd.h"
#include "cyhal.h"
#ifdef FREERTOS_ENABLED
#include <FreeRTOS.h>
#include <timers.h>
#include <task.h>
#else /* bare-metal */
#include "cyhal_lptimer.h"

static bool timer_init = false;
static volatile uint64_t tick = 0;

static void systick_isr(void)
{
    tick++;
}
#endif

__attribute__((weak)) EI_IMPULSE_ERROR ei_run_impulse_check_canceled() {
    return EI_IMPULSE_OK;
}

#ifdef FREERTOS_ENABLED
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {
    vTaskDelay(time_ms / portTICK_PERIOD_MS);

    return EI_IMPULSE_OK;
}

__attribute__((weak)) uint64_t ei_read_timer_ms() {

    return xTaskGetTickCount();
}

__attribute__((weak)) uint64_t ei_read_timer_us() {

    return xTaskGetTickCount()*1000;
}
#else /* Bare-metal */
__attribute__((weak)) EI_IMPULSE_ERROR ei_sleep(int32_t time_ms) {
    cyhal_system_delay_ms(time_ms);
    return EI_IMPULSE_OK;
}

uint64_t ei_read_timer_ms() {
    if(timer_init == false) {
        cyhal_clock_t clock;
        uint32_t freq;

        // get IMO clock frequency
        cyhal_clock_reserve(&clock, &CYHAL_CLOCK_IMO);
        freq = cyhal_clock_get_frequency(&clock);
        cyhal_clock_free(&clock);

        // set SysTick to 1 ms
        Cy_SysTick_Init(CY_SYSTICK_CLOCK_SOURCE_CLK_IMO, (freq / 1000) - 1);
        Cy_SysTick_SetCallback(0, systick_isr);
        timer_init = true;
        return 0;
    }
    return tick;
}

uint64_t ei_read_timer_us() {
    return ei_read_timer_ms() * 1000;
}
#endif /* FREERTOS_ENABLED */

void ei_putchar(char c)
{
    putchar(c);
}

__attribute__((weak)) char ei_getchar(void)
{
    return getchar();
}

__attribute__((weak)) void ei_printf(const char *format, ...) {

    char buffer[256];
    va_list myargs;
    va_start(myargs, format);
    vsnprintf(buffer, 256, format, myargs);
    va_end(myargs);

    printf("%s", buffer);
}

__attribute__((weak)) void ei_printf_float(float f) {
    ei_printf("%f", f);
}

#ifdef FREERTOS_ENABLED
__attribute__((weak)) void *ei_malloc(size_t size) {
    return pvPortMalloc(size);
}

__attribute__((weak)) void *ei_calloc(size_t nitems, size_t size) {
    void *mem = NULL;

    /* Infineon port of FreeRTOS does not support pvPortCalloc */
    mem = pvPortMalloc(nitems * size);
    if (mem) {
        /* zero the memory */
        memset(mem, 0, nitems * size);
    }
    return mem;
}

__attribute__((weak)) void ei_free(void *ptr) {
    vPortFree(ptr);
}
#else
__attribute__((weak)) void *ei_malloc(size_t size) {
    return malloc(size);
}

__attribute__((weak)) void *ei_calloc(size_t nitems, size_t size) {
    return calloc(nitems, size);
}

__attribute__((weak)) void ei_free(void *ptr) {
    free(ptr);
}
#endif

#if defined(__cplusplus) && EI_C_LINKAGE == 1
extern "C"
#endif
__attribute__((weak)) void DebugLog(const char* s) {
    ei_printf("%s", s);
}

#endif // EI_PORTING_INFINEONPSOC62
