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

#ifndef __EIPROFILER__H__
#define __EIPROFILER__H__

#include "edge-impulse-sdk/porting/ei_classifier_porting.h"

class EiProfiler {
public:
    EiProfiler()
    {
        reset();
    }
    void reset()
    {
        timestamp = ei_read_timer_ms();
    }
    void report(const char *message)
    {
        ei_printf("%s took %llu\r\n", message, ei_read_timer_ms() - timestamp);
        timestamp = ei_read_timer_ms(); //read again to not count printf time
    }

private:
    uint64_t timestamp;
};

#endif  //!__EIPROFILER__H__
