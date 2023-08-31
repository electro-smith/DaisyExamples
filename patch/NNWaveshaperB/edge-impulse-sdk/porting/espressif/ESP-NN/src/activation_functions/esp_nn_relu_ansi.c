#include "edge-impulse-sdk/classifier/ei_classifier_config.h"
#if EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN
// Copyright 2020-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <stdlib.h>

#include <edge-impulse-sdk/porting/espressif/ESP-NN/src/common/common_functions.h>

void esp_nn_relu6_s8_ansi(int8_t *data, uint16_t size)
{
    int32_t i;

    for (i = 0; i < size; i++) {
        int32_t ip = data[i];

        ip = max(ip, 0);
        data[i] = min(ip, 6);
    }
}

#endif // EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN
