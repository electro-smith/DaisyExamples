/* Edge Impulse inferencing library
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EI_CLASSIFIER_INFERENCING_ENGINE_AKIDA_H
#define EI_CLASSIFIER_INFERENCING_ENGINE_AKIDA_H

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_AKIDA)

/**
 * @brief if we are not forcing SOFTWARE inference (simulation)
 *  then make sure we will try to use hardware
 *
 */
#ifndef EI_CLASSIFIER_USE_AKIDA_SOFTWARE
#define EI_CLASSIFIER_USE_AKIDA_HARDWARE 1
#endif

/**
 * @brief If more than one device is present in system
 * setting this to device index can select a proper device.
 * e.g.: set to 1 to selct /dev/akida1
 *
 */
#ifndef EI_CLASSIFIER_USE_AKIDA_HARDWARE_NO
#define EI_CLASSIFIER_USE_AKIDA_HARDWARE_NO 0
#endif

#include "model-parameters/model_metadata.h"
#include "edge-impulse-sdk/porting/ei_classifier_porting.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/softmax.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <math.h>
#include <algorithm>
#include "pybind11/embed.h"
#include "pybind11/numpy.h"
#include "pybind11/stl.h"

namespace py = pybind11;

std::stringstream engine_info;

static py::module_ akida;
static py::object model;
static py::object model_predict;
static py::object model_forward;
static py::object device;
static bool akida_initialized = false;
static std::vector<size_t> input_shape;
static tflite::RuntimeShape softmax_shape;
static tflite::SoftmaxParams dummy_params;
static int model_input_bits = 0;
static float scale;
static int down_scale;

bool init_akida(const uint8_t *model_arr, size_t model_arr_size, bool debug)
{
    py::module_ sys;
    py::list path;
    constexpr char model_file_path[] = "/tmp/akida_model.fbz";

    if(debug) {
        try {
            sys = py::module_::import("sys");
            path = sys.attr("path");
            ei_printf("DEBUG: sys.path:");
            for (py::handle p: path) {
                ei_printf("\t%s\n", p.cast<std::string>().c_str());
            }
        }
        catch (py::error_already_set &e) {
            ei_printf("ERR: Importing 'sys' library failed:\n%s\n", e.what());
            // as it is only for debug purposes, continue
        }
    }

    try {
        // import Python's akida module
        akida = py::module_::import("akida");
    }
    catch (py::error_already_set &e) {
        ei_printf("ERR: Importing 'akida' library failed:\n%s\n", e.what());
        return false;
    }

    py::object Model = akida.attr("Model");

    // deploy akida model file into temporary file
    std::ofstream model_file(model_file_path, std::ios::out | std::ios::binary);
    model_file.write(reinterpret_cast<const char*>(model_arr), model_arr_size);
    if(model_file.bad()) {
        ei_printf("ERR: failed to unpack model ile into %s\n", model_file_path);
        model_file.close();
        return false;
    }
    model_file.close();

    // load model
    try {
        model = Model(model_file_path);
    }
    catch (py::error_already_set &e) {
        ei_printf("ERR: Can't load model file from %s\n", model_file_path);
        return false;
    }

    // get input shape from model
    input_shape = model.attr("input_shape").cast<std::vector<size_t>>();
    //TODO: temporarily only 3D input data is supported (see note in run_nn_inference)
    if(input_shape.size() != 3) {
        ei_printf("ERR: Unsupported input data shape. Expected 3 dimensions got %d\n", (int)input_shape.size());
        return false;
    }
    // extend input by (N, ...) - hardcoded to (1, ...)
    input_shape.insert(input_shape.begin(), (size_t)1);

    // get model input_bits
    std::vector<py::object> layers = model.attr("layers").cast<std::vector<py::object>>();
    auto input_layer = layers[0];
    model_input_bits = input_layer.attr("input_bits").cast<int>();
    if((model_input_bits != 8) && (model_input_bits != 4)) {
        ei_printf("ERR: Unsupported input_bits. Expected 4 or 8 got %d\n", model_input_bits);
        return false;
    }

    // initialize scale coefficients
    if(model_input_bits == 8) {
        scale = 255;
        down_scale = 1;
    }
    else if(model_input_bits == 4) {
        // these values are recommended by BrainChip
        scale = 15;
        down_scale = 16;
    }

    if(debug) {
        ei_printf("INFO: Model input_bits: %d\n", model_input_bits);
        ei_printf("INFO: Scale: %f\n", scale);
        ei_printf("INFO: Down scale: %d\n", down_scale);
    }

#if (defined(EI_CLASSIFIER_USE_AKIDA_HARDWARE) && (EI_CLASSIFIER_USE_AKIDA_HARDWARE == 1))
    // get list of available devices
    py::list devices = akida.attr("devices")();
    if(devices.empty() == true) {
        ei_printf("ERR: AKD1000 device not found!\n");
        return false;
    }

    if(devices.size() > 1) {
        ei_printf("More than one device found! Using /dev/akida%d\n", EI_CLASSIFIER_USE_AKIDA_HARDWARE_NO);
        device = devices[EI_CLASSIFIER_USE_AKIDA_HARDWARE_NO];
    }
    else {
        device = devices[0];
    }
    //TODO: check if selected device is correct (compare versions)
    // enable power measurement
    device.attr("soc").attr("power_measurement_enabled") = true;

    // map model to the device
    try {
        model.attr("map")(device);
    }
    catch (py::error_already_set &e) {
        ei_printf("ERR: Can't load the ML model onto the AKD1000 SoC\n");
        return false;
    }
#elif (defined(EI_CLASSIFIER_USE_AKIDA_SOFTWARE) && (EI_CLASSIFIER_USE_AKIDA_SOFTWARE == 1))
#warning "Akida model will be run in SIMULATION mode (not on real hardware)!"
#else
#error "Neither EI_CLASSIFIER_USE_AKIDA_HARDWARE or EI_CLASSIFIER_USE_AKIDA_SOFTWARE are defined or set to 1"
#endif

    // init softmax shape
    std::vector<size_t> tmp = model.attr("output_shape").cast<std::vector<size_t>>();
    softmax_shape.BuildFrom(tmp);
    // dumy beta parameter for softmax purposes
    dummy_params.beta = 1;

    // get reference to predict function
    model_predict = model.attr("predict");
    model_forward = model.attr("forward");

    // clear info stream
    engine_info.str("");

    return true;
}

template<typename T>
void debug_print(const std::vector<T> vec, const int val_per_row = 3)
{
    int n = 0;
    for(auto it = vec.begin(); it != vec.end(); it++) {
        ei_printf("%f ", *it);
        if(++n > val_per_row - 1) {
            ei_printf("\n");
            n = 0;
        }
    }
}

/**
 * @brief      Do neural network inferencing over the processed feature matrix
 *
 * @param      impulse  Struct describing impulse architecture
 * @param      fmatrix  Processed matrix
 * @param      result   Output classifier results
 * @param[in]  debug    Debug output enable
 *
 * @return     The ei impulse error.
 */
EI_IMPULSE_ERROR run_nn_inference(
    const ei_impulse_t *impulse,
    ei::matrix_t *fmatrix,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    ei_learning_block_config_tflite_graph_t *block_config = ((ei_learning_block_config_tflite_graph_t*)config_ptr);
    ei_config_tflite_graph_t *graph_config = ((ei_config_tflite_graph_t*)block_config->graph_config);
    EI_IMPULSE_ERROR fill_res = EI_IMPULSE_OK;

    // init Python embedded interpreter (should be called once!)
    static py::scoped_interpreter guard{};

    // check if we've initialized the interpreter and device?
    if (akida_initialized == false) {
        if(init_akida(graph_config->model, graph_config->model_size, debug) == false) {
            return EI_IMPULSE_AKIDA_ERROR;
        }
        akida_initialized = true;
    }

    // according to:
    // https://doc.brainchipinc.com/api_reference/akida_apis.html#akida.Model.predict
    // input type is always uint8
    py::array_t<uint8_t> input_data(input_shape);

    /*
     * convert data to uint8 and copy features into input tensor
     * For images RGB shape is (width, height, colors)
     * For images BW shape is (width, height, 1)
     * For Audio shape is (width, height, 1) - spectrogram
     * TODO: test with other ML models/data types
     * For details see:
     * https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html#direct-access
     */
    auto r = input_data.mutable_unchecked<4>();
    float temp;
    for (py::ssize_t x = 0; x < r.shape(1); x++) {
        for (py::ssize_t y = 0; y < r.shape(2); y++) {
            for(py::ssize_t z = 0; z < r.shape(3); z++) {
                temp = (fmatrix->buffer[x * r.shape(2) * r.shape(3) + y * r.shape(3) + z] * scale);
                temp = std::max(0.0f, std::min(temp, 255.0f));
                r(0, x, y, z) = (uint8_t)(temp / down_scale);
            }
        }
    }

    // Run inference on AKD1000
    uint64_t ctx_start_us = ei_read_timer_us();
    py::array_t<float> potentials;
    try {
        potentials = model_predict(input_data);
    }
    catch (py::error_already_set &e) {
        ei_printf("ERR: Inference error:\n%s\n", e.what());
        return EI_IMPULSE_AKIDA_ERROR;
    }
    // TODO: 'forward' is returning int8 or int32, but EI SDK supports int8 or float32 only
    // py::array_t<float> potentials = model_forward(input_data);
    uint64_t ctx_end_us = ei_read_timer_us();

    potentials = potentials.squeeze();

    if(debug) {
        std::string ret_str = py::str(potentials).cast<std::string>();
        ei_printf("AKD1000 raw output:\n%s\n", ret_str.c_str());
    }

    // convert to vector of floats to make further processing much easier
    std::vector<float> potentials_v;// = potentials.cast<std::vector<float>>();

    // TODO: output conversion depending on output shape?
    if (impulse->object_detection == false) {
        potentials_v = potentials.squeeze().cast<std::vector<float>>();
    }
    else {
        // TODO: output from AkidaNet/MobileNet is always N x M x P (3 dimensions)?
        auto q = potentials.unchecked<>();
        for (py::ssize_t x = 0; x < q.shape(0); x++) {
            for (py::ssize_t y = 0; y < q.shape(1); y++) {
                for(py::ssize_t z = 0; z < q.shape(2); z++) {
                    potentials_v.push_back(q(x, y, z));
                }
            }
        }
    }

    // apply softmax, becuase Akida is not supporting this operation
    tflite::reference_ops::Softmax(dummy_params, softmax_shape, potentials_v.data(), softmax_shape, potentials_v.data());

    if(debug == true) {
        ei_printf("After softmax:\n");
        debug_print(potentials_v);
    }

    float active_power = 0;
#if (defined(EI_CLASSIFIER_USE_AKIDA_HARDWARE))
    // power measurement post-processing
    float floor_power = device.attr("soc").attr("power_meter").attr("floor").cast<float>();
    py::array pwr_events = device.attr("soc").attr("power_meter").attr("events")();
    auto events = pwr_events.mutable_unchecked<py::object>();
    for (py::ssize_t i = 0; i < events.shape(0); i++) {
        active_power += events(i).attr("power").cast<float>();
    }
    active_power = (active_power/pwr_events.size()) - floor_power;
#endif

    result->timing.classification_us = ctx_end_us - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);

    // clear info
    engine_info.str("");
    engine_info << "Power consumption: " << std::fixed << std::setprecision(2) << active_power << " mW\n";
    engine_info << "Inferences per second: " << (1000000 / result->timing.classification_us);

    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
            case EI_CLASSIFIER_LAST_LAYER_FOMO: {
                fill_res = fill_result_struct_f32_fomo(
                    impulse,
                    result,
                    potentials_v.data(),
                    impulse->fomo_output_size,
                    impulse->fomo_output_size);
                break;
            }
            case EI_CLASSIFIER_LAST_LAYER_SSD: {
                ei_printf("ERR: MobileNet SSD models are not implemented for Akida (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
            case EI_CLASSIFIER_LAST_LAYER_YOLOV5: {
                ei_printf("ERR: YOLO v5 models are not implemented for Akida (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
            default: {
                ei_printf("ERR: Unsupported object detection last layer (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
        }
    }
    else {
        fill_res = fill_result_struct_f32(impulse, result, potentials_v.data(), debug);
    }

    return fill_res;
}

#endif // EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_AKIDA

#endif /* EI_CLASSIFIER_INFERENCING_ENGINE_AKIDA_H */
