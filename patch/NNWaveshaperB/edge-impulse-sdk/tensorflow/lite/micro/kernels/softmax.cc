// Patched by Edge Impulse to include reference and hardware-accelerated kernels
#include "../../../../classifier/ei_classifier_config.h"
#if 0 == 1
/* noop */
#elif EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN == 1
/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/softmax.h"

#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nnfunctions.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/softmax.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/op_macros.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

void SoftmaxQuantized(const TfLiteEvalTensor* input, TfLiteEvalTensor* output,
                      const SoftmaxParams& op_data) {
  if (input->type == kTfLiteUInt8) {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_U8
    return;
    #endif

    tflite::reference_ops::Softmax(
        op_data, tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<uint8_t>(input),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<uint8_t>(output));
  } else if (input->type == kTfLiteInt8) {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
    return;
    #endif

    if (output->type == kTfLiteInt16) {
      #if EI_TFLITE_DISABLE_SOFTMAX_OUT_I16
      return;
      #endif

      tflite::reference_ops::Softmax(
          op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int16_t>(output));
    } else {
      #if EI_TFLITE_DISABLE_SOFTMAX_OUT_I8
      return;
      #endif

      const auto input_shape = tflite::micro::GetTensorShape(input);
      const auto output_shape = tflite::micro::GetTensorShape(output);
      const int trailing_dim = input_shape.DimensionsCount() - 1;
      const int outer_size =
          MatchingFlatSizeSkipDim(input_shape, trailing_dim, output_shape);
      const int depth =
          MatchingDim(input_shape, trailing_dim, output_shape, trailing_dim);

      arm_softmax_s8(tflite::micro::GetTensorData<int8_t>(input), outer_size,
                     depth, op_data.input_multiplier, op_data.input_left_shift,
                     op_data.diff_min,
                     tflite::micro::GetTensorData<int8_t>(output));
    }
  } else {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
    return;
    #endif

    tflite::reference_ops::SoftmaxInt16(
        op_data, tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int16_t>(input),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int16_t>(output));
  }
}

TfLiteStatus SoftmaxEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, 0);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, 0);

  TFLITE_DCHECK(node->user_data != nullptr);
  const SoftmaxParams data =
      *static_cast<const SoftmaxParams*>(node->user_data);

  switch (input->type) {
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::Softmax(
          data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      return kTfLiteOk;
    }
    case kTfLiteInt8: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(input, output, data);
      return kTfLiteOk;
    }
    case kTfLiteUInt8: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(input, output, data);
      return kTfLiteOk;
    }
    case kTfLiteInt16: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(input, output, data);
      return kTfLiteOk;
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
}

}  // namespace

TfLiteRegistration Register_SOFTMAX() {
  return {/*init=*/SoftmaxInit,
          /*free=*/nullptr,
          /*prepare=*/SoftmaxPrepare,
          /*invoke=*/SoftmaxEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#elif EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN == 1
/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/softmax.h"

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/softmax.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/op_macros.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

#include "freertos/FreeRTOS.h"
#include <esp_timer.h>

#include "edge-impulse-sdk/porting/espressif/ESP-NN/include/esp_nn.h"

long long softmax_total_time = 0;

namespace tflite {
namespace {

struct NodeData {
  SoftmaxParams op_data;
  int buffer_idx;
};

// Softmax parameter data that persists in user_data
const int kInt16LUTArraySize = 513;

TfLiteStatus CalculateSoftmaxParams(TfLiteContext* context,
                                    const TfLiteTensor* input,
                                    TfLiteTensor* output,
                                    const TfLiteSoftmaxParams* params,
                                    SoftmaxParams* op_data) {
  if (input->type == kTfLiteInt8 || input->type == kTfLiteInt16) {
    if (input->type == kTfLiteInt16) {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      TF_LITE_ENSURE_EQ(context, output->params.zero_point, 0);
      TF_LITE_ENSURE_NEAR(context, output->params.scale, 1.f / 32768,
                          (0.001f * 1.f / 32768));
    } else {  // input->type == kTfLiteInt8
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      TF_LITE_ENSURE_TYPES_EQ(context, input->type, kTfLiteInt8);
      if (output->type == kTfLiteInt16) {
        TF_LITE_ENSURE_EQ(context, output->params.zero_point, -32768);
        TF_LITE_ENSURE_NEAR(context, output->params.scale, 1.f / 65536,
                            (0.001f * 1.f / 65536));
      } else {  // output->type == kTfLiteint8
        TF_LITE_ENSURE_TYPES_EQ(context, output->type, kTfLiteInt8);
        TF_LITE_ENSURE_EQ(context, output->params.zero_point, -128);
        TF_LITE_ENSURE(context, output->params.scale == 1.f / 256);
      }
    }

    static const int kScaledDiffIntegerBits = 5;

    // Calculate input_multiplier and input_left_shift
    if (input->type == kTfLiteInt16) {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      int input_left_shift;
      double input_scale_beta_rescale =
          static_cast<double>(input->params.scale) *
          static_cast<double>(params->beta) /
          (10.0 / 65535.0);  // scale the input_diff such that [-65535, 0]
                             // correspond to [-10.0, 0.0]
      QuantizeMultiplier(input_scale_beta_rescale, &op_data->input_multiplier,
                         &input_left_shift);
      op_data->input_left_shift = input_left_shift;
    } else { // kTfLiteInt8
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      int input_left_shift;
      tflite::PreprocessSoftmaxScaling(
          static_cast<double>(params->beta),
          static_cast<double>(input->params.scale), kScaledDiffIntegerBits,
          &op_data->input_multiplier, &input_left_shift);
      op_data->input_left_shift = input_left_shift;
      op_data->diff_min =
          -1.0 * tflite::CalculateInputRadius(kScaledDiffIntegerBits,
                                              op_data->input_left_shift);
    }
  } else {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_F32
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(input->type), input->type);
    return kTfLiteError;
    #endif

    TF_LITE_ENSURE_TYPES_EQ(context, input->type, kTfLiteFloat32);
    TF_LITE_ENSURE_TYPES_EQ(context, output->type, kTfLiteFloat32);
    op_data->beta = static_cast<double>(params->beta);
  }
  return kTfLiteOk;
}

static void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(NodeData));
}

void SoftmaxQuantized(TfLiteContext* context, const TfLiteEvalTensor* input,
                      TfLiteEvalTensor* output, const NodeData* data) {
  if (input->type == kTfLiteInt8) {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(input->type), input->type);
    return;
    #endif

    if (output->type == kTfLiteInt16) {
      #if EI_TFLITE_DISABLE_SOFTMAX_OUT_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(output->type), output->type);
      return;
      #endif

      tflite::reference_ops::Softmax(
          data->op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int16_t>(output));
    } else {
      #if EI_TFLITE_DISABLE_SOFTMAX_OUT_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(output->type), output->type);
      return;
      #endif

      const int32_t input_beta_multiplier = data->op_data.input_multiplier;
      const int32_t input_beta_left_shift = data->op_data.input_left_shift;
      const int diff_min = data->op_data.diff_min;
      const RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
      const RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
      const int trailing_dim = input_shape.DimensionsCount() - 1;
      const int outer_size =
          MatchingFlatSizeSkipDim(input_shape, trailing_dim, output_shape);
      const int depth =
          MatchingDim(input_shape, trailing_dim, output_shape, trailing_dim);
      const int8_t *in_ptr = tflite::micro::GetTensorData<int8_t>(input);
      int8_t *out_ptr = tflite::micro::GetTensorData<int8_t>(output);
      void *scratch_buf = NULL;
      if (data->buffer_idx > -1) {
        scratch_buf = context->GetScratchBuffer(context, data->buffer_idx);
      }
      esp_nn_set_softmax_scratch_buf(scratch_buf);
      esp_nn_softmax_s8(in_ptr, outer_size, depth, input_beta_multiplier,
                        input_beta_left_shift, diff_min, out_ptr);
    }
  } else {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(input->type), input->type);
    return;
    #endif

    tflite::reference_ops::SoftmaxInt16(
        data->op_data, tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int16_t>(input),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int16_t>(output));
  }
}

static TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, 0);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, 0);

  TFLITE_DCHECK(node->user_data != nullptr);
  NodeData data = *static_cast<NodeData*>(node->user_data);

  long long start_time = esp_timer_get_time();
  switch (input->type) {
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      tflite::reference_ops::Softmax(
          data.op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      break;
    }
    case kTfLiteInt8: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(context, input, output, &data);
      break;
    }
    case kTfLiteInt16: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(context, input, output, &data);
      break;
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  softmax_total_time += esp_timer_get_time() - start_time;
  return kTfLiteOk;
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TF_LITE_ENSURE(context, input != nullptr);
  TF_LITE_ENSURE(context, NumDimensions(input) >= 1);
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE(context, node->user_data != nullptr);
  NodeData* data = static_cast<NodeData*>(node->user_data);

  // Only allocate LUTs for KTfLiteInt16 data type
  if (input->type == kTfLiteInt16) {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(input->type), input->type);
    return kTfLiteError;
    #endif

    void* raw_exp_lut = context->AllocatePersistentBuffer(
        context, sizeof(int16_t) * kInt16LUTArraySize);
    TF_LITE_ENSURE(context, raw_exp_lut != nullptr);
    data->op_data.exp_lut = reinterpret_cast<int16_t*>(raw_exp_lut);
    void* one_over_one_plus_x_lut = context->AllocatePersistentBuffer(
        context, sizeof(int16_t) * kInt16LUTArraySize);
    TF_LITE_ENSURE(context, one_over_one_plus_x_lut != nullptr);
    data->op_data.one_over_one_plus_x_lut =
        reinterpret_cast<int16_t*>(one_over_one_plus_x_lut);
  }

  if (output->type == kTfLiteInt16) {
    TF_LITE_ENSURE(context,
                   input->type == kTfLiteInt8 || input->type == kTfLiteInt16);
  } else {
    TF_LITE_ENSURE_EQ(context, input->type, output->type);
  }

  // Populate LUT if required
  if (input->type == kTfLiteInt16) {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(input->type), input->type);
    return kTfLiteError;
    #endif

    TF_LITE_ENSURE_EQ(context, output->params.zero_point, 0);
    // exp LUT only used on negative values
    // we consider exp(-10.0) is insignificant to accumulation
    gen_lut([](float value) { return std::exp(value); }, -10.0f, 0.0f,
            data->op_data.exp_lut, kInt16LUTArraySize);
    gen_lut([](float value) { return 1.0f / (1.0f + value); }, 0.0f, 1.0f,
            data->op_data.one_over_one_plus_x_lut, kInt16LUTArraySize);
    data->op_data.zero_point = output->params.zero_point;
    data->op_data.scale = output->params.scale;
  }

  auto* params = static_cast<TfLiteSoftmaxParams*>(node->builtin_data);
  auto ret_val =
      CalculateSoftmaxParams(context, input, output, params, &data->op_data);

  if (output->type == kTfLiteInt8 && input->type == kTfLiteInt8) {
    const int32_t input_width = input->dims->data[1];
    const int32_t input_height = input->dims->data[2];
    int scratch_buf_size = esp_nn_get_softmax_scratch_size(input_width,
                                                           input_height);
    if (scratch_buf_size > 0) {
      TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
        context, scratch_buf_size, &data->buffer_idx));
    }
  }

  //micro_context->DeallocateTempTfLiteTensor(input);
  //micro_context->DeallocateTempTfLiteTensor(output);
  return ret_val;
}

}  // namespace

TfLiteRegistration Register_SOFTMAX() {
  return {/*init=*/Init,
          /*free=*/nullptr,
          /*prepare=*/Prepare,
          /*invoke=*/Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
#else
/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/softmax.h"

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/softmax.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/op_macros.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

void SoftmaxQuantized(const TfLiteEvalTensor* input, TfLiteEvalTensor* output,
                      const SoftmaxParams& op_data) {
  if (input->type == kTfLiteInt8) {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
      return;
      #endif

    if (output->type == kTfLiteInt16) {
      #if EI_TFLITE_DISABLE_SOFTMAX_OUT_I16
      return;
      #endif

      tflite::reference_ops::Softmax(
          op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int16_t>(output));
    } else { // kTfLiteInt8
      #if EI_TFLITE_DISABLE_SOFTMAX_OUT_I8
      return;
      #endif

      tflite::reference_ops::Softmax(
          op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int8_t>(output));
    }
  } else {
    #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
    return;
    #endif

    tflite::reference_ops::SoftmaxInt16(
        op_data, tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int16_t>(input),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int16_t>(output));
  }
}

TfLiteStatus SoftmaxEval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input = tflite::micro::GetEvalInput(context, node, 0);
  TfLiteEvalTensor* output = tflite::micro::GetEvalOutput(context, node, 0);

  TFLITE_DCHECK(node->user_data != nullptr);
  SoftmaxParams op_data = *static_cast<SoftmaxParams*>(node->user_data);

  switch (input->type) {
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::Softmax(
          op_data, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      return kTfLiteOk;
    }
    case kTfLiteInt8: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(input, output, op_data);
      return kTfLiteOk;
    }
    case kTfLiteInt16: {
      #if EI_TFLITE_DISABLE_SOFTMAX_IN_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      SoftmaxQuantized(input, output, op_data);
      return kTfLiteOk;
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
}
}  // namespace

TfLiteRegistration Register_SOFTMAX() {
  return {/*init=*/SoftmaxInit,
          /*free=*/nullptr,
          /*prepare=*/SoftmaxPrepare,
          /*invoke=*/SoftmaxEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#endif
