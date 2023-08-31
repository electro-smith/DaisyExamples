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

#ifndef _EI_CLASSIFIER_INFERENCING_ENGINE_DRPAI_H_
#define _EI_CLASSIFIER_INFERENCING_ENGINE_DRPAI_H_

#if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_DRPAI)

/*****************************************
 * includes
 ******************************************/
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <fstream>
#include <iomanip>
#include <map>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <model-parameters/model_metadata.h>

#if ((EI_CLASSIFIER_OBJECT_DETECTION == 1) && (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI))
// For a YOLOV5_V5_DRPAI model we ran the unsupported layers with TF
#include <thread>
#include "tensorflow-lite/tensorflow/lite/c/common.h"
#include "tensorflow-lite/tensorflow/lite/interpreter.h"
#include "tensorflow-lite/tensorflow/lite/kernels/register.h"
#include "tensorflow-lite/tensorflow/lite/model.h"
#include "tensorflow-lite/tensorflow/lite/optional_debug_tools.h"
#endif
#include "edge-impulse-sdk/tensorflow/lite/kernels/tree_ensemble_classifier.h"
#include "edge-impulse-sdk/classifier/ei_fill_result_struct.h"
#include "edge-impulse-sdk/classifier/ei_model_types.h"
#include "edge-impulse-sdk/classifier/ei_run_dsp.h"
#include "edge-impulse-sdk/porting/ei_logging.h"

#include <linux/drpai.h>
#include <tflite-model/drpai_model.h>



/*****************************************
 * Macro
 ******************************************/
/*Maximum DRP-AI Timeout threshold*/
#define DRPAI_TIMEOUT (5)

/*Buffer size for writing data to memory via DRP-AI Driver.*/
#define BUF_SIZE (1024)

/*Index to access drpai_file_path[]*/
#define INDEX_D (0)
#define INDEX_C (1)
#define INDEX_P (2)
#define INDEX_A (3)
#define INDEX_W (4)

/*****************************************
 * Public global vars
 ******************************************/
// input and output buffer pointers for memory mapped regions used by DRP-AI
uint8_t *drpai_input_buf = (uint8_t *)NULL;
float *drpai_output_buf = (float *)NULL;

/*****************************************
 * Typedef
 ******************************************/
/* For DRP-AI Address List */
typedef struct {
  unsigned long desc_aimac_addr;
  unsigned long desc_aimac_size;
  unsigned long desc_drp_addr;
  unsigned long desc_drp_size;
  unsigned long drp_param_addr;
  unsigned long drp_param_size;
  unsigned long data_in_addr;
  unsigned long data_in_size;
  unsigned long data_addr;
  unsigned long data_size;
  unsigned long work_addr;
  unsigned long work_size;
  unsigned long data_out_addr;
  unsigned long data_out_size;
  unsigned long drp_config_addr;
  unsigned long drp_config_size;
  unsigned long weight_addr;
  unsigned long weight_size;
} st_addr_t;

/*****************************************
 * static vars
 ******************************************/
static st_addr_t drpai_address;
static uint64_t udmabuf_address = 0;

static int drpai_fd = -1;

drpai_data_t proc[DRPAI_INDEX_NUM];

void get_udmabuf_memory_start_addr()
{   /* Obtain udmabuf memory area starting address */

    int8_t fd = 0;
    char addr[1024];
    int32_t read_ret = 0;
    errno = 0;

    fd = open("/sys/class/u-dma-buf/udmabuf0/phys_addr", O_RDONLY);
    if (0 > fd)
    {
        fprintf(stderr, "[ERROR] Failed to open udmabuf0/phys_addr : errno=%d\n", errno);
    }

    read_ret = read(fd, addr, 1024);
    if (0 > read_ret)
    {
        fprintf(stderr, "[ERROR] Failed to read udmabuf0/phys_addr : errno=%d\n", errno);
        close(fd);
    }

    sscanf(addr, "%lx", &udmabuf_address);
    close(fd);

    /* Filter the bit higher than 32 bit */
    udmabuf_address &=0xFFFFFFFF;
}

uint8_t drpai_init_mem(uint32_t input_frame_size) {
  int32_t i = 0;

  int udmabuf_fd0 = open("/dev/udmabuf0", O_RDWR);
  if (udmabuf_fd0 < 0) {
    return -1;
  }

  // input_frame_size === data_in_size
  uint8_t *addr =
      (uint8_t *)mmap(NULL, input_frame_size,
                      PROT_READ | PROT_WRITE, MAP_SHARED, udmabuf_fd0, 0);

  drpai_input_buf = addr;

  /* Write once to allocate physical memory to u-dma-buf virtual space.
   * Note: Do not use memset() for this.
   *       Because it does not work as expected. */
  for (i = 0; i < input_frame_size; i++) {
    drpai_input_buf[i] = 0;
  }


  get_udmabuf_memory_start_addr();
  if (0 == udmabuf_address) {
    return EI_IMPULSE_DRPAI_INIT_FAILED;
  }

  return 0;
}

/*****************************************
 * Function Name : read_addrmap_txt
 * Description	: Loads address and size of DRP-AI Object files into struct
 *addr. Arguments		: addr_file = filename of addressmap file (from
 *DRP-AI Object files) Return value	: 0 if succeeded not 0 otherwise
 ******************************************/
static int8_t read_addrmap_txt() {
  // create a stream from the DRP-AI model data without copying
  std::istringstream ifs;
  ifs.rdbuf()->pubsetbuf((char *)ei_ei_addrmap_intm_txt, ei_ei_addrmap_intm_txt_len);

  std::string str;
  unsigned long l_addr;
  unsigned long l_size;
  std::string element, a, s;

  if (ifs.fail()) {
    return -1;
  }

  while (getline(ifs, str)) {
    std::istringstream iss(str);
    iss >> element >> a >> s;
    l_addr = strtol(a.c_str(), NULL, 16);
    l_size = strtol(s.c_str(), NULL, 16);

    if (element == "drp_config") {
      drpai_address.drp_config_addr = l_addr;
      drpai_address.drp_config_size = l_size;
    } else if (element == "desc_aimac") {
      drpai_address.desc_aimac_addr = l_addr;
      drpai_address.desc_aimac_size = l_size;
    } else if (element == "desc_drp") {
      drpai_address.desc_drp_addr = l_addr;
      drpai_address.desc_drp_size = l_size;
    } else if (element == "drp_param") {
      drpai_address.drp_param_addr = l_addr;
      drpai_address.drp_param_size = l_size;
    } else if (element == "weight") {
      drpai_address.weight_addr = l_addr;
      drpai_address.weight_size = l_size;
    } else if (element == "data_in") {
      drpai_address.data_in_addr = l_addr;
      drpai_address.data_in_size = l_size;
    } else if (element == "data") {
      drpai_address.data_addr = l_addr;
      drpai_address.data_size = l_size;
    } else if (element == "data_out") {
      drpai_address.data_out_addr = l_addr;
      drpai_address.data_out_size = l_size;
    } else if (element == "work") {
      drpai_address.work_addr = l_addr;
      drpai_address.work_size = l_size;
    }
  }

  return 0;
}

/*****************************************
 * Function Name : load_data_to_mem
 * Description	: Loads a binary blob DRP-AI Driver Memory
 * Arguments		: data_ptr = pointer to the bytes to write
 *				  drpai_fd = file descriptor of DRP-AI Driver
 *				  from = memory start address where the data is
 *written size = data size to be written Return value	: 0 if succeeded not 0
 *otherwise
 ******************************************/
static int8_t load_data_to_mem(unsigned char *data_ptr, int drpai_fd,
                               unsigned long from, unsigned long size) {
  drpai_data_t drpai_data;

  drpai_data.address = from;
  drpai_data.size = size;

  errno = 0;
  if (-1 == ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data)) {
    return -1;
  }

  errno = 0;
  if (-1 == write(drpai_fd, data_ptr, size)) {
    return -1;
  }

  return 0;
}

/*****************************************
 * Function Name :  load_drpai_data
 * Description	: Loads DRP-AI Object files to memory via DRP-AI Driver.
 * Arguments		: drpai_fd = file descriptor of DRP-AI Driver
 * Return value	: 0 if succeeded
 *				: not 0 otherwise
 ******************************************/
static int load_drpai_data(int drpai_fd) {
  unsigned long addr, size;
  unsigned char *data_ptr;
  for (int i = 0; i < 5; i++) {
    switch (i) {
    case (INDEX_W):
      addr = drpai_address.weight_addr;
      size = drpai_address.weight_size;
      data_ptr = ei_ei_weight_dat;
      break;
    case (INDEX_C):
      addr = drpai_address.drp_config_addr;
      size = drpai_address.drp_config_size;
      data_ptr = ei_ei_drpcfg_mem;
      break;
    case (INDEX_P):
      addr = drpai_address.drp_param_addr;
      size = drpai_address.drp_param_size;
      data_ptr = ei_drp_param_bin;
      break;
    case (INDEX_A):
      addr = drpai_address.desc_aimac_addr;
      size = drpai_address.desc_aimac_size;
      data_ptr = ei_aimac_desc_bin;
      break;
    case (INDEX_D):
      addr = drpai_address.desc_drp_addr;
      size = drpai_address.desc_drp_size;
      data_ptr = ei_drp_desc_bin;
      break;
    default:
      return -1;
      break;
    }
    if (0 != load_data_to_mem(data_ptr, drpai_fd, addr, size)) {
      return -1;
    }
  }
  return 0;
}

EI_IMPULSE_ERROR drpai_init_classifier() {
  // retval for drpai status
  int ret_drpai;

  // Read DRP-AI Object files address and size
  if (0 != read_addrmap_txt()) {
    ei_printf("ERR: read_addrmap_txt failed : %d\n", errno);
    return EI_IMPULSE_DRPAI_INIT_FAILED;
  }

  // DRP-AI Driver Open
  drpai_fd = open("/dev/drpai0", O_RDWR);
  if (drpai_fd < 0) {
    ei_printf("ERR: Failed to Open DRP-AI Driver: errno=%d\n", errno);
    return EI_IMPULSE_DRPAI_INIT_FAILED;
  }

  // Load DRP-AI Data from Filesystem to Memory via DRP-AI Driver
  ret_drpai = load_drpai_data(drpai_fd);
  if (ret_drpai != 0) {
    ei_printf("ERR: Failed to load DRPAI Data\n");
    if (0 != close(drpai_fd)) {
      ei_printf("ERR: Failed to Close DRPAI Driver: errno=%d\n", errno);
    }
    return EI_IMPULSE_DRPAI_INIT_FAILED;
  }

  // statically store DRP object file addresses and sizes
  proc[DRPAI_INDEX_INPUT].address = (uint32_t)udmabuf_address;
  proc[DRPAI_INDEX_INPUT].size = drpai_address.data_in_size;
  proc[DRPAI_INDEX_DRP_CFG].address = drpai_address.drp_config_addr;
  proc[DRPAI_INDEX_DRP_CFG].size = drpai_address.drp_config_size;
  proc[DRPAI_INDEX_DRP_PARAM].address = drpai_address.drp_param_addr;
  proc[DRPAI_INDEX_DRP_PARAM].size = drpai_address.drp_param_size;
  proc[DRPAI_INDEX_AIMAC_DESC].address = drpai_address.desc_aimac_addr;
  proc[DRPAI_INDEX_AIMAC_DESC].size = drpai_address.desc_aimac_size;
  proc[DRPAI_INDEX_DRP_DESC].address = drpai_address.desc_drp_addr;
  proc[DRPAI_INDEX_DRP_DESC].size = drpai_address.desc_drp_size;
  proc[DRPAI_INDEX_WEIGHT].address = drpai_address.weight_addr;
  proc[DRPAI_INDEX_WEIGHT].size = drpai_address.weight_size;
  proc[DRPAI_INDEX_OUTPUT].address = drpai_address.data_out_addr;
  proc[DRPAI_INDEX_OUTPUT].size = drpai_address.data_out_size;

  EI_LOGD("proc[DRPAI_INDEX_INPUT] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_INPUT].address, proc[DRPAI_INDEX_INPUT].size);
  EI_LOGD("proc[DRPAI_INDEX_DRP_CFG] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_DRP_CFG].address, proc[DRPAI_INDEX_DRP_CFG].size);
  EI_LOGD("proc[DRPAI_INDEX_DRP_PARAM] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_DRP_PARAM].address, proc[DRPAI_INDEX_DRP_PARAM].size);
  EI_LOGD("proc[DRPAI_INDEX_AIMAC_DESC] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_AIMAC_DESC].address, proc[DRPAI_INDEX_AIMAC_DESC].size);
  EI_LOGD("proc[DRPAI_INDEX_DRP_DESC] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_DRP_DESC].address, proc[DRPAI_INDEX_DRP_DESC].size);
  EI_LOGD("proc[DRPAI_INDEX_WEIGHT] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_WEIGHT].address, proc[DRPAI_INDEX_WEIGHT].size);
  EI_LOGD("proc[DRPAI_INDEX_OUTPUT] addr: %p, size: %p\r\n", proc[DRPAI_INDEX_OUTPUT].address, proc[DRPAI_INDEX_OUTPUT].size);

  drpai_output_buf = (float *)ei_malloc(drpai_address.data_out_size);

  return EI_IMPULSE_OK;
}

EI_IMPULSE_ERROR drpai_run_classifier_image_quantized() {
#if EI_CLASSIFIER_COMPILED == 1
#error "DRP-AI is not compatible with EON Compiler"
#endif
  // output data from DRPAI model
  drpai_data_t drpai_data;
  // status used to query if any internal errors occured during inferencing
  drpai_status_t drpai_status;
  // descriptor used for checking if DRPAI is done inferencing
  fd_set rfds;
  // struct used to define DRPAI timeout
  struct timespec tv;
  // retval for drpai status
  int ret_drpai;
  // retval when querying drpai status
  int inf_status = 0;

  // DRP-AI Output Memory Preparation
  drpai_data.address = drpai_address.data_out_addr;
  drpai_data.size = drpai_address.data_out_size;

  // Start DRP-AI driver
  EI_LOGD("Start DRPAI inference\r\n");
  int ioret = ioctl(drpai_fd, DRPAI_START, &proc[0]);
  if (0 != ioret) {
    EI_LOGE("Failed to Start DRPAI Inference: %d\n", errno);
    return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
  }

  // Settings For pselect - this is how DRPAI signals inferencing complete
  FD_ZERO(&rfds);
  FD_SET(drpai_fd, &rfds);
  // Define a timeout for DRP-AI to complete
  tv.tv_sec = DRPAI_TIMEOUT;
  tv.tv_nsec = 0;

  // Wait until DRP-AI ends
  EI_LOGD("Waiting on DRPAI inference results\r\n");
  ret_drpai = pselect(drpai_fd + 1, &rfds, NULL, NULL, &tv, NULL);
  if (ret_drpai == 0) {
      EI_LOGE("DRPAI Inference pselect() Timeout: %d\n", errno);
    return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
  } else if (ret_drpai < 0) {
      EI_LOGE("DRPAI Inference pselect() Error: %d\n", errno);
    return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
  }

  // Checks for DRPAI inference status errors
  EI_LOGD("Getting DRPAI Status\r\n");
  inf_status = ioctl(drpai_fd, DRPAI_GET_STATUS, &drpai_status);
  if (inf_status != 0) {
      EI_LOGE("DRPAI Internal Error: %d\n", errno);
    return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
  }

  EI_LOGD("Getting inference results\r\n");
  if (ioctl(drpai_fd, DRPAI_ASSIGN, &drpai_data) != 0) {
      EI_LOGE("Failed to Assign DRPAI data: %d\n", errno);
    return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
  }

  if (read(drpai_fd, drpai_output_buf, drpai_data.size) < 0) {
      EI_LOGE("Failed to read DRPAI output data: %d\n", errno);
    return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
  }
  return EI_IMPULSE_OK;
}

// close the driver (reset file handles)
EI_IMPULSE_ERROR drpai_close(uint32_t input_frame_size) {
  munmap(drpai_input_buf, input_frame_size);
  free(drpai_output_buf);
  if (drpai_fd > 0) {
    if (0 != close(drpai_fd)) {
        EI_LOGE("Failed to Close DRP-AI Driver: errno=%d\n", errno);
      return EI_IMPULSE_DRPAI_RUNTIME_FAILED;
    }
    drpai_fd = -1;
  }
  return EI_IMPULSE_OK;
}

#if ((EI_CLASSIFIER_OBJECT_DETECTION == 1) && (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI))
EI_IMPULSE_ERROR drpai_run_yolov5_postprocessing(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    bool debug = false)
{
    static std::unique_ptr<tflite::FlatBufferModel> model = nullptr;
    static std::unique_ptr<tflite::Interpreter> interpreter = nullptr;

    if (!model) {
        model = tflite::FlatBufferModel::BuildFromBuffer((const char*)yolov5_part2, yolov5_part2_len);
        if (!model) {
            ei_printf("Failed to build TFLite model from buffer\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }

        tflite::ops::builtin::BuiltinOpResolver resolver;
        tflite::InterpreterBuilder builder(*model, resolver);
        builder(&interpreter);

        if (!interpreter) {
            ei_printf("Failed to construct interpreter\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }

        if (interpreter->AllocateTensors() != kTfLiteOk) {
            ei_printf("AllocateTensors failed\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }

        int hw_thread_count = (int)std::thread::hardware_concurrency();
        hw_thread_count -= 1; // leave one thread free for the other application
        if (hw_thread_count < 1) {
            hw_thread_count = 1;
        }

        if (interpreter->SetNumThreads(hw_thread_count) != kTfLiteOk) {
            ei_printf("SetNumThreads failed\n");
            return EI_IMPULSE_TFLITE_ERROR;
        }
    }

    const size_t drpai_buff_size = drpai_address.data_out_size / sizeof(float);
    const size_t drpai_features = drpai_buff_size;

    const size_t els_per_grid = drpai_features / ((NUM_GRID_1 * NUM_GRID_1) + (NUM_GRID_2 * NUM_GRID_2) + (NUM_GRID_3 * NUM_GRID_3));

    const size_t grid_1_offset = 0;
    const size_t grid_1_size = (NUM_GRID_1 * NUM_GRID_1) * els_per_grid;

    const size_t grid_2_offset = grid_1_offset + grid_1_size;
    const size_t grid_2_size = (NUM_GRID_2 * NUM_GRID_2) * els_per_grid;

    const size_t grid_3_offset = grid_2_offset + grid_2_size;
    const size_t grid_3_size = (NUM_GRID_3 * NUM_GRID_3) * els_per_grid;

    // Now we don't know the exact tensor order for some reason
    // so let's do that dynamically
    for (size_t ix = 0; ix < 3; ix++) {
        TfLiteTensor * tensor = interpreter->input_tensor(ix);
        size_t tensor_size = 1;
        for (size_t ix = 0; ix < tensor->dims->size; ix++) {
            tensor_size *= tensor->dims->data[ix];
        }

        EI_LOGD("input tensor %d, tensor_size=%d\n", (int)ix, (int)tensor_size);

        float *input = interpreter->typed_input_tensor<float>(ix);

        if (tensor_size == grid_1_size) {
            memcpy(input, drpai_output_buf + grid_1_offset, grid_1_size * sizeof(float));
        }
        else if (tensor_size == grid_2_size) {
            memcpy(input, drpai_output_buf + grid_2_offset, grid_2_size * sizeof(float));
        }
        else if (tensor_size == grid_3_size) {
            memcpy(input, drpai_output_buf + grid_3_offset, grid_3_size * sizeof(float));
        }
        else {
            ei_printf("ERR: Cannot determine which grid to use for input tensor %d with %d tensor size\n",
                (int)ix, (int)tensor_size);
            return EI_IMPULSE_TFLITE_ERROR;
        }
    }

    uint64_t ctx_start_us = ei_read_timer_us();

    interpreter->Invoke();

    uint64_t ctx_end_us = ei_read_timer_us();

    EI_LOGD("Invoke took %d ms.\n", (int)((ctx_end_us - ctx_start_us) / 1000));

    float* out_data = interpreter->typed_output_tensor<float>(0);

    const size_t out_size = impulse->tflite_output_features_count;

    if (debug) {
      printf("First 20 bytes: ");
      for (size_t ix = 0; ix < 20; ix++) {
          ei_printf("%f ", out_data[ix]);
      }
      ei_printf("\n");
    }

    // printf("Last 5 bytes: ");
    // for (size_t ix = out_size - 5; ix < out_size; ix++) {
    //     printf("%f ", out_data[ix]);
    // }
    // printf("\n");

    return fill_result_struct_f32_yolov5(impulse, result, 5, out_data, out_size);
}
#endif

/**
 * @brief      Do neural network inferencing over the processed feature matrix
 *
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
    // dummy, not used for DRPAI
}

/**
 * Special function to run the classifier on images, only works on TFLite models (either interpreter or EON or for tensaiflow)
 * that allocates a lot less memory by quantizing in place. This only works if 'can_run_classifier_image_quantized'
 * returns EI_IMPULSE_OK.
 */
EI_IMPULSE_ERROR run_nn_inference_image_quantized(
    const ei_impulse_t *impulse,
    signal_t *signal,
    ei_impulse_result_t *result,
    void *config_ptr,
    bool debug = false)
{
    // this needs to be changed for multi-model, multi-impulse
    static bool first_run = true;
    uint64_t ctx_start_us;
    uint64_t dsp_start_us = ei_read_timer_us();

    if (first_run) {
        // map memory regions to the DRP-AI UDMA. This is required for passing data
        // to and from DRP-AI
        int t = drpai_init_mem(impulse->nn_input_frame_size);
        if (t != 0) {
            return EI_IMPULSE_DRPAI_INIT_FAILED;
        }

        EI_IMPULSE_ERROR ret = drpai_init_classifier();
        if (ret != EI_IMPULSE_OK) {
            drpai_close(impulse->nn_input_frame_size);
            return EI_IMPULSE_DRPAI_INIT_FAILED;
        }

        EI_LOGI("Initialized input and output buffers:\r\n");
        EI_LOGI("input buf (addr: %p, size: 0x%x)\r\n", drpai_input_buf, drpai_address.data_in_size);
        EI_LOGI("output buf (addr: %p, size: 0x%x)\r\n", drpai_output_buf, drpai_address.data_out_size);
        EI_LOGI("udmabuf_addr: %p\n", udmabuf_address);
    }

    EI_LOGD("Starting DSP...\n");
    int ret;

    EI_LOGD("fmatrix size == Bpp * signal.total_length ( %p == %p * %p = %p )\r\n", proc[DRPAI_INDEX_INPUT].size, 3, signal->total_length, 3 * signal->total_length);
    // Creates a features matrix mapped to the DRP-AI UDMA input region
    ei::matrix_u8_t features_matrix(1, proc[DRPAI_INDEX_INPUT].size, drpai_input_buf);

    // Grabs the raw image buffer from the signal, DRP-AI will automatically
    // extract features
    ret = extract_drpai_features_quantized(
        signal,
        &features_matrix,
        impulse->dsp_blocks[0].config,
        impulse->frequency);
    if (ret != EIDSP_OK) {
        ei_printf("ERR: Failed to run DSP process (%d)\n", ret);
        return EI_IMPULSE_DSP_ERROR;
    }

    if (ei_run_impulse_check_canceled() == EI_IMPULSE_CANCELED) {
        return EI_IMPULSE_CANCELED;
    }

    result->timing.dsp_us = ei_read_timer_us() - dsp_start_us;
    result->timing.dsp = (int)(result->timing.dsp_us / 1000);
    if (debug) {
      ei_printf("Features (%d ms.): ", result->timing.dsp);
      for (size_t ix = 0; ix < EI_CLASSIFIER_NN_INPUT_FRAME_SIZE; ix++) {
          ei_printf("0x%hhx, ", drpai_input_buf[ix]);
      }
      ei_printf("\n");
    }

    ctx_start_us = ei_read_timer_us();

    // Run DRP-AI inference, a static buffer is used to store the raw output
    // results
    ret = drpai_run_classifier_image_quantized();

    // close driver to reset memory, file pointer
    if (ret != EI_IMPULSE_OK) {
        drpai_close(impulse->nn_input_frame_size);
        first_run = true;
    }
    else {
        // drpai_reset();
        first_run = false;
    }

    EI_IMPULSE_ERROR fill_res = EI_IMPULSE_OK;

    if (impulse->object_detection) {
        switch (impulse->object_detection_last_layer) {
            case EI_CLASSIFIER_LAST_LAYER_FOMO: {
                if (debug) {
                    ei_printf("DEBUG: raw drpai output");
                    ei_printf("\n[");
                    for (uint32_t i = 0; i < impulse->tflite_output_features_count; i++) {
                        ei_printf_float(drpai_output_buf[i]);
                        ei_printf(" ");
                    }
                    ei_printf("]\n");
                }

                fill_res = fill_result_struct_f32_fomo(
                    impulse,
                    result,
                    drpai_output_buf,
                    impulse->fomo_output_size,
                    impulse->fomo_output_size);
                break;
            }
            case EI_CLASSIFIER_LAST_LAYER_SSD: {
                ei_printf("ERR: MobileNet SSD models are not implemented for DRP-AI (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
            case EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI: {
                #if EI_CLASSIFIER_TFLITE_OUTPUT_QUANTIZED == 1
                    ei_printf("ERR: YOLOv5 does not support quantized inference\n");
                    return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
                #else
                  if (debug) {
                      ei_printf("DEBUG: raw drpai output");
                      ei_printf("\n[");
                      // impulse->tflite_output_features_count can't be used here as this is not the final output
                      // so print only the first 10 values.
                      for (uint32_t i = 0; i < 10; i++) {
                          ei_printf_float(drpai_output_buf[i]);
                          ei_printf(" ");
                      }
                      ei_printf("]\n");
                  }

#if ((EI_CLASSIFIER_OBJECT_DETECTION == 1) && (EI_CLASSIFIER_OBJECT_DETECTION_LAST_LAYER == EI_CLASSIFIER_LAST_LAYER_YOLOV5_V5_DRPAI))
                  // do post processing
                  fill_res = drpai_run_yolov5_postprocessing(impulse, signal, result, debug);
#endif

                #endif

                break;
            }
            default: {
                ei_printf("ERR: Unsupported object detection last layer (%d)\n",
                    impulse->object_detection_last_layer);
                return EI_IMPULSE_UNSUPPORTED_INFERENCING_ENGINE;
            }
        }
    }
    else {
        fill_res = fill_result_struct_f32(impulse, result, drpai_output_buf, debug);
    }

    if (fill_res != EI_IMPULSE_OK) {
        return fill_res;
    }

    result->timing.classification_us = ei_read_timer_us() - ctx_start_us;
    result->timing.classification = (int)(result->timing.classification_us / 1000);
    return EI_IMPULSE_OK;
}

#endif // #if (EI_CLASSIFIER_INFERENCING_ENGINE == EI_CLASSIFIER_DRPAI)
#endif // _EI_CLASSIFIER_INFERENCING_ENGINE_DRPAI_H_
