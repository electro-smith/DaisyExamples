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

#ifndef _EI_CLASSIFIER_FILL_RESULT_STRUCT_H_
#define _EI_CLASSIFIER_FILL_RESULT_STRUCT_H_

using namespace ei;

#include "model-parameters/model_metadata.h"
#if EI_CLASSIFIER_HAS_MODEL_VARIABLES == 1
#include "model-parameters/model_variables.h"
#endif
#include "edge-impulse-sdk/classifier/ei_model_types.h"
#include "edge-impulse-sdk/classifier/ei_classifier_types.h"
#include "edge-impulse-sdk/classifier/ei_nms.h"

#ifndef EI_HAS_OBJECT_DETECTION
    #if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_SSD)
    #define EI_HAS_SSD 1
    #endif
    #if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_FOMO)
    #define EI_HAS_FOMO 1
    #endif
    #if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5) || (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI)
    #define EI_HAS_YOLOV5 1
    #endif
    #if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOX)
    #define EI_HAS_YOLOX 1
    #endif
    #if (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV7)
    #define EI_HAS_YOLOV7 1
    #endif
#endif

#ifdef EI_HAS_FOMO
typedef struct cube {
    size_t x;
    size_t y;
    size_t width;
    size_t height;
    float confidence;
    const char *label;
} ei_classifier_cube_t;

/**
 * Checks whether a new section overlaps with a cube,
 * and if so, will **update the cube**
 */
__attribute__((unused)) static bool ei_cube_check_overlap(ei_classifier_cube_t *c, int x, int y, int width, int height, float confidence) {
    bool is_overlapping = !(c->x + c->width < x || c->y + c->height < y || c->x > x + width || c->y > y + height);
    if (!is_overlapping) return false;

    // if we overlap, but the x of the new box is lower than the x of the current box
    if (x < c->x) {
        // update x to match new box and make width larger (by the diff between the boxes)
        c->x = x;
        c->width += c->x - x;
    }
    // if we overlap, but the y of the new box is lower than the y of the current box
    if (y < c->y) {
        // update y to match new box and make height larger (by the diff between the boxes)
        c->y = y;
        c->height += c->y - y;
    }
    // if we overlap, and x+width of the new box is higher than the x+width of the current box
    if (x + width > c->x + c->width) {
        // just make the box wider
        c->width += (x + width) - (c->x + c->width);
    }
    // if we overlap, and y+height of the new box is higher than the y+height of the current box
    if (y + height > c->y + c->height) {
        // just make the box higher
        c->height += (y + height) - (c->y + c->height);
    }
    // if the new box has higher confidence, then override confidence of the whole box
    if (confidence > c->confidence) {
        c->confidence = confidence;
    }
    return true;
}

__attribute__((unused)) static void ei_handle_cube(std::vector<ei_classifier_cube_t*> *cubes, int x, int y, float vf, const char *label, float detection_threshold) {
    if (vf < detection_threshold) return;

    bool has_overlapping = false;
    int width = 1;
    int height = 1;

    for (auto c : *cubes) {
        // not cube for same class? continue
        if (strcmp(c->label, label) != 0) continue;

        if (ei_cube_check_overlap(c, x, y, width, height, vf)) {
            has_overlapping = true;
            break;
        }
    }

    if (!has_overlapping) {
        ei_classifier_cube_t *cube = new ei_classifier_cube_t();
        cube->x = x;
        cube->y = y;
        cube->width = 1;
        cube->height = 1;
        cube->confidence = vf;
        cube->label = label;
        cubes->push_back(cube);
    }
}

__attribute__((unused)) static void fill_result_struct_from_cubes(ei_impulse_result_t *result, std::vector<ei_classifier_cube_t*> *cubes, int out_width_factor, uint32_t object_detection_count) {
    std::vector<ei_classifier_cube_t*> bbs;
    static std::vector<ei_impulse_result_bounding_box_t> results;
    int added_boxes_count = 0;
    results.clear();
    for (auto sc : *cubes) {
        bool has_overlapping = false;

        int x = sc->x;
        int y = sc->y;
        int width = sc->width;
        int height = sc->height;
        const char *label = sc->label;
        float vf = sc->confidence;

        for (auto c : bbs) {
            // not cube for same class? continue
            if (strcmp(c->label, label) != 0) continue;

            if (ei_cube_check_overlap(c, x, y, width, height, vf)) {
                has_overlapping = true;
                break;
            }
        }

        if (has_overlapping) {
            continue;
        }

        bbs.push_back(sc);

        ei_impulse_result_bounding_box_t tmp = {
            .label = sc->label,
            .x = (uint32_t)(sc->x * out_width_factor),
            .y = (uint32_t)(sc->y * out_width_factor),
            .width = (uint32_t)(sc->width * out_width_factor),
            .height = (uint32_t)(sc->height * out_width_factor),
            .value = sc->confidence
        };

        results.push_back(tmp);
        added_boxes_count++;
    }

    // if we didn't detect min required objects, fill the rest with fixed value
    if (added_boxes_count < object_detection_count) {
        results.resize(object_detection_count);
        for (size_t ix = added_boxes_count; ix < object_detection_count; ix++) {
            results[ix].value = 0.0f;
        }
    }

    for (auto c : *cubes) {
        delete c;
    }

    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();
}
#endif

__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32_fomo(const ei_impulse_t *impulse,
                                                                            ei_impulse_result_t *result,
                                                                            float *data,
                                                                            int out_width,
                                                                            int out_height) {
#ifdef EI_HAS_FOMO
    std::vector<ei_classifier_cube_t*> cubes;

    int out_width_factor = impulse->input_width / out_width;

    for (size_t y = 0; y < out_width; y++) {
        // ei_printf("    [ ");
        for (size_t x = 0; x < out_height; x++) {
            size_t loc = ((y * out_height) + x) * (impulse->label_count + 1);

            for (size_t ix = 1; ix < impulse->label_count + 1; ix++) {
                float vf = data[loc+ix];

                ei_handle_cube(&cubes, x, y, vf, impulse->categories[ix - 1], impulse->object_detection_threshold);
            }
        }
    }

    fill_result_struct_from_cubes(result, &cubes, out_width_factor, impulse->object_detection_count);

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif
}

__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_i8_fomo(const ei_impulse_t *impulse,
                                                                           ei_impulse_result_t *result,
                                                                           int8_t *data,
                                                                           float zero_point,
                                                                           float scale,
                                                                           int out_width,
                                                                           int out_height) {
#ifdef EI_HAS_FOMO
    std::vector<ei_classifier_cube_t*> cubes;

    int out_width_factor = impulse->input_width / out_width;

    for (size_t y = 0; y < out_width; y++) {
        // ei_printf("    [ ");
        for (size_t x = 0; x < out_height; x++) {
            size_t loc = ((y * out_height) + x) * (impulse->label_count + 1);

            for (size_t ix = 1; ix < impulse->label_count + 1; ix++) {
                int8_t v = data[loc+ix];
                float vf = static_cast<float>(v - zero_point) * scale;

                ei_handle_cube(&cubes, x, y, vf, impulse->categories[ix - 1], impulse->object_detection_threshold);
            }
        }
    }

    fill_result_struct_from_cubes(result, &cubes, out_width_factor, impulse->object_detection_count);

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif
}

/**
 * Fill the result structure from an unquantized output tensor
 * (we don't support quantized here a.t.m.)
 */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32_object_detection(const ei_impulse_t *impulse,
                                                                                        ei_impulse_result_t *result,
                                                                                        float *data,
                                                                                        float *scores,
                                                                                        float *labels,
                                                                                        bool debug) {
#ifdef EI_HAS_SSD
    static std::vector<ei_impulse_result_bounding_box_t> results;
    results.clear();
    results.resize(impulse->object_detection_count);
    for (size_t ix = 0; ix < impulse->object_detection_count; ix++) {

        float score = scores[ix];
        float label = labels[ix];

        if (score >= impulse->object_detection_threshold) {
            float ystart = data[(ix * 4) + 0];
            float xstart = data[(ix * 4) + 1];
            float yend = data[(ix * 4) + 2];
            float xend = data[(ix * 4) + 3];

            if (xstart < 0) xstart = 0;
            if (xstart > 1) xstart = 1;
            if (ystart < 0) ystart = 0;
            if (ystart > 1) ystart = 1;
            if (yend < 0) yend = 0;
            if (yend > 1) yend = 1;
            if (xend < 0) xend = 0;
            if (xend > 1) xend = 1;
            if (xend < xstart) xend = xstart;
            if (yend < ystart) yend = ystart;

            if (debug) {
                ei_printf("%s (", impulse->categories[(uint32_t)label]);
                ei_printf_float(label);
                ei_printf("): ");
                ei_printf_float(score);
                ei_printf(" [ ");
                ei_printf_float(xstart);
                ei_printf(", ");
                ei_printf_float(ystart);
                ei_printf(", ");
                ei_printf_float(xend);
                ei_printf(", ");
                ei_printf_float(yend);
                ei_printf(" ]\n");
            }

            results[ix].label = impulse->categories[(uint32_t)label];
            results[ix].x = static_cast<uint32_t>(xstart * static_cast<float>(impulse->input_width));
            results[ix].y = static_cast<uint32_t>(ystart * static_cast<float>(impulse->input_height));
            results[ix].width = static_cast<uint32_t>((xend - xstart) * static_cast<float>(impulse->input_width));
            results[ix].height = static_cast<uint32_t>((yend - ystart) * static_cast<float>(impulse->input_height));
            results[ix].value = score;
        }
        else {
            results[ix].value = 0.0f;
        }
    }
    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif
}

/**
 * Fill the result structure from a quantized output tensor
 */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_i8(const ei_impulse_t *impulse,
                                                                      ei_impulse_result_t *result,
                                                                      int8_t *data,
                                                                      float zero_point,
                                                                      float scale,
                                                                      bool debug) {
    for (uint32_t ix = 0; ix < impulse->label_count; ix++) {
        float value = static_cast<float>(data[ix] - zero_point) * scale;

        if (debug) {
            ei_printf("%s:\t", impulse->categories[ix]);
            ei_printf_float(value);
            ei_printf("\n");
        }
        result->classification[ix].label = impulse->categories[ix];
        result->classification[ix].value = value;
    }

    return EI_IMPULSE_OK;
}

/**
 * Fill the result structure from an unquantized output tensor
 */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32(const ei_impulse_t *impulse,
                                                                       ei_impulse_result_t *result,
                                                                       float *data,
                                                                       bool debug) {
    for (uint32_t ix = 0; ix < impulse->label_count; ix++) {
        float value = data[ix];

        if (debug) {
            ei_printf("%s:\t", impulse->categories[ix]);
            ei_printf_float(value);
            ei_printf("\n");
        }
        result->classification[ix].label = impulse->categories[ix];
        result->classification[ix].value = value;
    }

    return EI_IMPULSE_OK;
}

/**
  * Fill the result structure from an unquantized output tensor
  */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32_yolov5(const ei_impulse_t *impulse,
                                                                              ei_impulse_result_t *result,
                                                                              int version,
                                                                              float *data,
                                                                              size_t output_features_count) {
#ifdef EI_HAS_YOLOV5
    static std::vector<ei_impulse_result_bounding_box_t> results;
    results.clear();

    size_t col_size = 5 + impulse->label_count;
    size_t row_count = output_features_count / col_size;

    for (size_t ix = 0; ix < row_count; ix++) {
        size_t base_ix = ix * col_size;
        float xc = data[base_ix + 0];
        float yc = data[base_ix + 1];
        float w = data[base_ix + 2];
        float h = data[base_ix + 3];
        float x = xc - (w / 2.0f);
        float y = yc - (h / 2.0f);
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if (x + w > impulse->input_width) {
            w = impulse->input_width - x;
        }
        if (y + h > impulse->input_height) {
            h = impulse->input_height - y;
        }

        if (w < 0 || h < 0) {
            continue;
        }

        float score = data[base_ix + 4];

        uint32_t label = 0;
        for (size_t lx = 0; lx < impulse->label_count; lx++) {
            float l = data[base_ix + 5 + lx];
            if (l > 0.5f) {
                label = lx;
                break;
            }
        }

        if (score >= impulse->object_detection_threshold && score <= 1.0f) {
            ei_impulse_result_bounding_box_t r;
            r.label = impulse->categories[label];

            if (version != 5) {
                x *= static_cast<float>(impulse->input_width);
                y *= static_cast<float>(impulse->input_height);
                w *= static_cast<float>(impulse->input_width);
                h *= static_cast<float>(impulse->input_height);
            }

            r.x = static_cast<uint32_t>(x);
            r.y = static_cast<uint32_t>(y);
            r.width = static_cast<uint32_t>(w);
            r.height = static_cast<uint32_t>(h);
            r.value = score;
            results.push_back(r);
        }
    }

    EI_IMPULSE_ERROR nms_res = ei_run_nms(&results);
    if (nms_res != EI_IMPULSE_OK) {
        return nms_res;
    }

    // if we didn't detect min required objects, fill the rest with fixed value
    size_t added_boxes_count = results.size();
    size_t min_object_detection_count = impulse->object_detection_count;
    if (added_boxes_count < min_object_detection_count) {
        results.resize(min_object_detection_count);
        for (size_t ix = added_boxes_count; ix < min_object_detection_count; ix++) {
            results[ix].value = 0.0f;
        }
    }

    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif
}

/**
 * Fill the result structure from a quantized output tensor
*/
template<typename T>
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_quantized_yolov5(const ei_impulse_t *impulse,
                                                                                    ei_impulse_result_t *result,
                                                                                    int version,
                                                                                    T *data,
                                                                                    float zero_point,
                                                                                    float scale,
                                                                                    size_t output_features_count) {
#ifdef EI_HAS_YOLOV5
    static std::vector<ei_impulse_result_bounding_box_t> results;
    results.clear();

    size_t col_size = 5 + impulse->label_count;
    size_t row_count = output_features_count / col_size;

    for (size_t ix = 0; ix < row_count; ix++) {
        size_t base_ix = ix * col_size;
        float xc = (data[base_ix + 0] - zero_point) * scale;
        float yc = (data[base_ix + 1] - zero_point) * scale;
        float w = (data[base_ix + 2] - zero_point) * scale;
        float h = (data[base_ix + 3] - zero_point) * scale;
        float x = xc - (w / 2.0f);
        float y = yc - (h / 2.0f);
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if (x + w > impulse->input_width) {
            w = impulse->input_width - x;
        }
        if (y + h > impulse->input_height) {
            h = impulse->input_height - y;
        }

        if (w < 0 || h < 0) {
            continue;
        }

        float score = (data[base_ix + 4] - zero_point) * scale;

        uint32_t label = 0;
        for (size_t lx = 0; lx < impulse->label_count; lx++) {
            float l = (data[base_ix + 5 + lx] - zero_point) * scale;
            if (l > 0.5f) {
                label = lx;
                break;
            }
        }

        if (score >= impulse->object_detection_threshold && score <= 1.0f) {
            ei_impulse_result_bounding_box_t r;
            r.label = ei_classifier_inferencing_categories[label];

            if (version != 5) {
                x *= static_cast<float>(impulse->input_width);
                y *= static_cast<float>(impulse->input_height);
                w *= static_cast<float>(impulse->input_width);
                h *= static_cast<float>(impulse->input_height);
            }

            r.x = static_cast<uint32_t>(x);
            r.y = static_cast<uint32_t>(y);
            r.width = static_cast<uint32_t>(w);
            r.height = static_cast<uint32_t>(h);
            r.value = score;
            results.push_back(r);
        }
    }

    EI_IMPULSE_ERROR nms_res = ei_run_nms(&results);
    if (nms_res != EI_IMPULSE_OK) {
        return nms_res;
    }

    // if we didn't detect min required objects, fill the rest with fixed value
    size_t added_boxes_count = results.size();
    size_t min_object_detection_count = impulse->object_detection_count;
    if (added_boxes_count < min_object_detection_count) {
        results.resize(min_object_detection_count);
        for (size_t ix = added_boxes_count; ix < min_object_detection_count; ix++) {
            results[ix].value = 0.0f;
        }
    }

    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif
}

/**
  * Fill the result structure from an unquantized output tensor
  * (we don't support quantized here a.t.m.)
  */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32_yolox(const ei_impulse_t *impulse, ei_impulse_result_t *result,
                                                                             float *data,
                                                                             size_t output_features_count) {
#ifdef EI_HAS_YOLOX
    static std::vector<ei_impulse_result_bounding_box_t> results;
    results.clear();

    // START: def yolox_postprocess()

    // if not p6:
    //     strides = [8, 16, 32]
    // else:
    //     strides = [8, 16, 32, 64]
    const std::vector<int> strides { 8, 16, 32 };

    // hsizes = [img_size[0] // stride for stride in strides]
    // wsizes = [img_size[1] // stride for stride in strides]
    std::vector<int> hsizes(strides.size());
    std::vector<int> wsizes(strides.size());
    for (int ix = 0; ix < (int)strides.size(); ix++) {
        hsizes[ix] = (int)floor((float)impulse->input_width / (float)strides[ix]);
        wsizes[ix] = (int)floor((float)impulse->input_height / (float)strides[ix]);
    }

    // for hsize, wsize, stride in zip(hsizes, wsizes, strides):
    //      grid = np.stack((xv, yv), 2).reshape(1, -1, 2)
    //      grids.append(grid)
    //      shape = grid.shape[:2]
    //      expanded_strides.append(np.full((*shape, 1), stride))
    std::vector<matrix_i32_t*> grids;
    std::vector<matrix_i32_t*> expanded_strides;

    for (int ix = 0; ix < (int)strides.size(); ix++) {
        int hsize = hsizes.at(ix);
        int wsize = wsizes.at(ix);
        int stride = strides.at(ix);

        // xv, yv = np.meshgrid(np.arange(wsize), np.arange(hsize))
        // grid = np.stack((xv, yv), 2).reshape(1, -1, 2)
        matrix_i32_t *grid = new matrix_i32_t(hsize * wsize, 2);
        int grid_ix = 0;
        for (int h = 0; h < hsize; h++) {
            for (int w = 0; w < wsize; w++) {
                grid->buffer[grid_ix + 0] = w;
                grid->buffer[grid_ix + 1] = h;
                grid_ix += 2;
            }
        }
        grids.push_back(grid);

        // shape = grid.shape[:2]
        // expanded_strides.append(np.full((*shape, 1), stride))
        matrix_i32_t *expanded_stride = new matrix_i32_t(hsize * wsize, 1);
        for (int ix = 0; ix < hsize * wsize; ix++) {
            expanded_stride->buffer[ix] = stride;
        }
        expanded_strides.push_back(expanded_stride);
    }

    // grids = np.concatenate(grids, 1)
    int total_grid_rows = 0;
    for (auto g : grids) {
        total_grid_rows += g->rows;
    }
    matrix_i32_t c_grid(total_grid_rows, 2);
    int c_grid_ix = 0;
    for (auto g : grids) {
        for (int row = 0; row < (int)g->rows; row++) {
            c_grid.buffer[c_grid_ix + 0] = g->buffer[(row * 2) + 0];
            c_grid.buffer[c_grid_ix + 1] = g->buffer[(row * 2) + 1];
            c_grid_ix += 2;
        }
        delete g;
    }

    // expanded_strides = np.concatenate(expanded_strides, 1)
    int total_stride_rows = 0;
    for (auto g : expanded_strides) {
        total_stride_rows += g->rows;
    }
    matrix_i32_t c_expanded_strides(total_stride_rows, 1);
    int c_expanded_strides_ix = 0;
    for (auto g : expanded_strides) {
        for (int row = 0; row < (int)g->rows; row++) {
            c_expanded_strides.buffer[c_expanded_strides_ix + 0] = g->buffer[(row * 1) + 0];
            c_expanded_strides_ix += 1;
        }
        delete g;
    }

    const int output_rows = output_features_count / (5 + impulse->label_count);
    matrix_t outputs(output_rows, 5 + impulse->label_count, data);
    for (int row = 0; row < (int)outputs.rows; row++) {
        float v0 = outputs.buffer[(row * outputs.cols) + 0];
        float v1 = outputs.buffer[(row * outputs.cols) + 1];
        float v2 = outputs.buffer[(row * outputs.cols) + 2];
        float v3 = outputs.buffer[(row * outputs.cols) + 3];

        float cgrid0 = (float)c_grid.buffer[(row * c_grid.cols) + 0];
        float cgrid1 = (float)c_grid.buffer[(row * c_grid.cols) + 1];

        float stride = (float)c_expanded_strides.buffer[row];

        // outputs[..., :2] = (outputs[..., :2] + grids) * expanded_strides
        outputs.buffer[(row * outputs.cols) + 0] = (v0 + cgrid0) * stride;
        outputs.buffer[(row * outputs.cols) + 1] = (v1 + cgrid1) * stride;

        // outputs[..., 2:4] = np.exp(outputs[..., 2:4]) * expanded_strides
        outputs.buffer[(row * outputs.cols) + 2] = exp(v2) * stride;
        outputs.buffer[(row * outputs.cols) + 3] = exp(v3) * stride;
    }

    // END: def yolox_postprocess()

    // boxes = predictions[:, :4]
    matrix_t boxes(outputs.rows, 4);
    for (int row = 0; row < (int)outputs.rows; row++) {
        boxes.buffer[(row * boxes.cols) + 0] = outputs.buffer[(row * outputs.cols) + 0];
        boxes.buffer[(row * boxes.cols) + 1] = outputs.buffer[(row * outputs.cols) + 1];
        boxes.buffer[(row * boxes.cols) + 2] = outputs.buffer[(row * outputs.cols) + 2];
        boxes.buffer[(row * boxes.cols) + 3] = outputs.buffer[(row * outputs.cols) + 3];
    }

    // scores = predictions[:, 4:5] * predictions[:, 5:]
    matrix_t scores(outputs.rows, impulse->label_count);
    for (int row = 0; row < (int)outputs.rows; row++) {
        float confidence = outputs.buffer[(row * outputs.cols) + 4];
        for (int cc = 0; cc < impulse->label_count; cc++) {
            scores.buffer[(row * scores.cols) + cc] = confidence * outputs.buffer[(row * outputs.cols) + (5 + cc)];
        }
    }

    // iterate through scores to see if we have anything with confidence
    for (int row = 0; row < (int)scores.rows; row++) {
        for (int col = 0; col < (int)scores.cols; col++) {
            float confidence = scores.buffer[(row * scores.cols) + col];

            if (confidence >= impulse->object_detection_threshold && confidence <= 1.0f) {
                ei_impulse_result_bounding_box_t r;
                r.label = impulse->categories[col];
                r.value = confidence;

                // now find the box...
                float xcenter = boxes.buffer[(row * boxes.cols) + 0];
                float ycenter = boxes.buffer[(row * boxes.cols) + 1];
                float width = boxes.buffer[(row * boxes.cols) + 2];
                float height = boxes.buffer[(row * boxes.cols) + 3];

                int x = (int)(xcenter - (width / 2.0f));
                int y = (int)(ycenter - (height / 2.0f));

                if (x < 0) {
                    x = 0;
                }
                if (x > (int)impulse->input_width) {
                    x = impulse->input_width;
                }
                if (y < 0) {
                    y = 0;
                }
                if (y > (int)impulse->input_height) {
                    y = impulse->input_height;
                }

                r.x = x;
                r.y = y;
                r.width = (int)round(width);
                r.height = (int)round(height);

                results.push_back(r);
            }
        }
    }

    EI_IMPULSE_ERROR nms_res = ei_run_nms(&results);
    if (nms_res != EI_IMPULSE_OK) {
        return nms_res;
    }

    // if we didn't detect min required objects, fill the rest with fixed value
    size_t added_boxes_count = results.size();
    size_t min_object_detection_count = impulse->object_detection_count;
    if (added_boxes_count < min_object_detection_count) {
        results.resize(min_object_detection_count);
        for (size_t ix = added_boxes_count; ix < min_object_detection_count; ix++) {
            results[ix].value = 0.0f;
        }
    }

    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif // EI_HAS_YOLOX
}

/**
  * Fill the result structure from an unquantized output tensor
  * (we don't support quantized here a.t.m.)
  */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32_yolox_detect(const ei_impulse_t *impulse, ei_impulse_result_t *result,
                                                                             float *data,
                                                                             size_t output_features_count) {
#ifdef EI_HAS_YOLOX
    static std::vector<ei_impulse_result_bounding_box_t> results;
    results.clear();

    // expected format [xmin ymin xmax ymax score label]
    const int output_rows = output_features_count / 6;
    matrix_t outputs(output_rows, 6, data);

    // iterate through scores to see if we have anything with confidence
    for (int row = 0; row < (int)outputs.rows; row++) {
        float confidence = outputs.buffer[(row * outputs.cols) + 4];
        int class_idx = (int)outputs.buffer[(row * outputs.cols) + 5];

        if (confidence >= impulse->object_detection_threshold && confidence <= 1.0f) {
            ei_impulse_result_bounding_box_t r;
            r.label = ei_classifier_inferencing_categories[class_idx];
            r.value = confidence;

            // now find the box...
            float xmin = outputs.buffer[(row * outputs.cols) + 0];
            float ymin = outputs.buffer[(row * outputs.cols) + 1];
            float xmax = outputs.buffer[(row * outputs.cols) + 2];
            float ymax = outputs.buffer[(row * outputs.cols) + 3];

            float width  = xmax - xmin;
            float height = ymax - ymin;

            int x = (int)xmin;
            int y = (int)ymin;

            if (x < 0) {
                x = 0;
            }
            if (x > (int)impulse->input_width) {
                x = impulse->input_width;
            }
            if (y < 0) {
                y = 0;
            }
            if (y > (int)impulse->input_height) {
                y = impulse->input_height;
            }

            r.x = x;
            r.y = y;
            r.width = (int)round(width);
            r.height = (int)round(height);

            results.push_back(r);
        }
    }

    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif // EI_HAS_YOLOX
}

/**
  * Fill the result structure from an unquantized output tensor
  * (we don't support quantized here a.t.m.)
  */
__attribute__((unused)) static EI_IMPULSE_ERROR fill_result_struct_f32_yolov7(const ei_impulse_t *impulse, ei_impulse_result_t *result,
                                                                              float *data,
                                                                              size_t output_features_count) {
#ifdef EI_HAS_YOLOV7
    static std::vector<ei_impulse_result_bounding_box_t> results;
    results.clear();

    size_t col_size = 7;
    size_t row_count = output_features_count / col_size;

    // output is:
    // batch_id, xmin, ymin, xmax, ymax, cls_id, score
    for (size_t ix = 0; ix < row_count; ix++) {
        size_t base_ix = ix * col_size;
        float xmin = data[base_ix + 1];
        float ymin = data[base_ix + 2];
        float xmax = data[base_ix + 3];
        float ymax = data[base_ix + 4];
        uint32_t label = (uint32_t)data[base_ix + 5];
        float score = data[base_ix + 6];

        if (score >= impulse->object_detection_threshold && score <= 1.0f) {
            ei_impulse_result_bounding_box_t r;
            r.label = ei_classifier_inferencing_categories[label];

            r.x = static_cast<uint32_t>(xmin);
            r.y = static_cast<uint32_t>(ymin);
            r.width = static_cast<uint32_t>(xmax - xmin);
            r.height = static_cast<uint32_t>(ymax - ymin);
            r.value = score;
            results.push_back(r);
        }
    }

    // if we didn't detect min required objects, fill the rest with fixed value
    size_t added_boxes_count = results.size();
    size_t min_object_detection_count = impulse->object_detection_count;
    if (added_boxes_count < min_object_detection_count) {
        results.resize(min_object_detection_count);
        for (size_t ix = added_boxes_count; ix < min_object_detection_count; ix++) {
            results[ix].value = 0.0f;
        }
    }

    result->bounding_boxes = results.data();
    result->bounding_boxes_count = results.size();

    return EI_IMPULSE_OK;
#else
    return EI_IMPULSE_LAST_LAYER_NOT_AVAILABLE;
#endif // #ifdef EI_HAS_YOLOV7
}

#endif // _EI_CLASSIFIER_FILL_RESULT_STRUCT_H_
