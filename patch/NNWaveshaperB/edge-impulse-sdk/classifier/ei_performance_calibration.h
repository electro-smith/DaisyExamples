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

#ifndef EI_PERFORMANCE_CALIBRATION_H
#define EI_PERFORMANCE_CALIBRATION_H

/* Includes ---------------------------------------------------------------- */
#include "edge-impulse-sdk/dsp/numpy_types.h"
#include "edge-impulse-sdk/dsp/returntypes.hpp"
#include "ei_model_types.h"

/* Private const types ----------------------------------------------------- */
#define MEM_ERROR   "ERR: Failed to allocate memory for performance calibration\r\n"

#define EI_PC_RET_NO_EVENT_DETECTED    -1
#define EI_PC_RET_MEMORY_ERROR         -2

class RecognizeEvents {

public:
    RecognizeEvents(
        const ei_model_performance_calibration_t *config,
        uint32_t n_labels,
        uint32_t sample_length,
        float sample_interval_ms)
    {
        this->_score_array = nullptr;
        this->_running_sum = nullptr;
        this->_detection_threshold = config->detection_threshold;
        this->_suppression_flags = config->suppression_flags;
        this->_should_boost = config->is_configured;
        this->_n_labels = n_labels;

        /* Determine sample length in ms */
        float sample_length_ms = (static_cast<float>(sample_length) * sample_interval_ms);

        /* Calculate number of inference runs needed for the duration window */
        this->_average_window_duration_samples =
            (config->average_window_duration_ms < static_cast<uint32_t>(sample_length_ms))
            ? 1
            : static_cast<uint32_t>(static_cast<float>(config->average_window_duration_ms) / sample_length_ms);

        /* Calculate number of inference runs for suppression */
        this->_suppression_samples = (config->suppression_ms < static_cast<uint32_t>(sample_length_ms))
            ? 0
            : static_cast<uint32_t>(static_cast<float>(config->suppression_ms) / sample_length_ms);

        /* Detection threshold should be high enough to only classifiy 1 possibly output */
        if (this->_detection_threshold <= (1.f / this->_n_labels)) {
            ei_printf("ERR: Classifier detection threshold too low\r\n");
            return;
        }

        /* Array to store scores for all labels */
        this->_score_array = (float *)ei_malloc(
            this->_average_window_duration_samples * this->_n_labels * sizeof(float));

        if (this->_score_array == NULL) {
            ei_printf(MEM_ERROR);
            return;
        }

        for (uint32_t i = 0; i < this->_average_window_duration_samples * this->_n_labels; i++) {
            this->_score_array[i] = 0.f;
        }
        this->_score_idx = 0;

        /* Running sum for all labels */
        this->_running_sum = (float *)ei_malloc(this->_n_labels * sizeof(float));

        if (this->_running_sum != NULL) {
            for (uint32_t i = 0; i < this->_n_labels; i++) {
                this->_running_sum[i] = 0.f;
            }
        }
        else {
            ei_printf(MEM_ERROR);
            return;
        }

        this->_suppression_count = this->_suppression_samples;
        this->_n_scores_in_array = 0;
    }

    ~RecognizeEvents()
    {
        if (this->_score_array) {
            ei_free((void *)this->_score_array);
        }
        if (this->_running_sum) {
            ei_free((void *)this->_running_sum);
        }
    }

    bool should_boost()
    {
        return this->_should_boost;
    }

    int32_t trigger(ei_impulse_result_classification_t *scores)
    {
        int32_t recognized_event = EI_PC_RET_NO_EVENT_DETECTED;
        float current_top_score = 0.f;
        uint32_t current_top_index = 0;

        /* Check pointers */
        if (this->_score_array == NULL || this->_running_sum == NULL) {
            return EI_PC_RET_MEMORY_ERROR;
        }

        /* Update the score array and running sum */
        for (uint32_t i = 0; i < this->_n_labels; i++) {
            this->_running_sum[i] -= this->_score_array[(this->_score_idx * this->_n_labels) + i];
            this->_running_sum[i] += scores[i].value;
            this->_score_array[(this->_score_idx * this->_n_labels) + i] = scores[i].value;
        }

        if (++this->_score_idx >= this->_average_window_duration_samples) {
            this->_score_idx = 0;
        }

        /* Number of samples to average, increases until the buffer is full */
        if (this->_n_scores_in_array < this->_average_window_duration_samples) {
            this->_n_scores_in_array++;
        }

        /* Average data and place in scores & determine top score */
        for (uint32_t i = 0; i < this->_n_labels; i++) {
            scores[i].value = this->_running_sum[i] / this->_n_scores_in_array;

            if (scores[i].value > current_top_score) {
                if(this->_suppression_flags == 0) {
                    current_top_score = scores[i].value;
                    current_top_index = i;
                }
                else if(this->_suppression_flags & (1 << i)) {
                    current_top_score = scores[i].value;
                    current_top_index = i;
                }
            }
        }

        /* Check threshold, suppression */
        if (this->_suppression_samples && this->_suppression_count < this->_suppression_samples) {
            this->_suppression_count++;
        }
        else {
            if (current_top_score >= this->_detection_threshold) {
                recognized_event = current_top_index;

                if (this->_suppression_flags & (1 << current_top_index)) {
                    this->_suppression_count = 0;
                }
            }
        }

        return recognized_event;
    };

    void *operator new(size_t size)
    {
        void *p = ei_malloc(size);
        return p;
    }

    void operator delete(void *p)
    {
        ei_free(p);
    }

private:
    uint32_t _average_window_duration_samples;
    float _detection_threshold;
    bool _should_boost;
    uint32_t _suppression_samples;
    uint32_t _suppression_count;
    uint32_t _suppression_flags;
    uint32_t _n_labels;
    float *_score_array;
    uint32_t _score_idx;
    float *_running_sum;
    uint32_t _n_scores_in_array;
};

#endif //EI_PERFORMANCE_CALIBRATION
