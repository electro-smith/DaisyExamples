// Patched by Edge Impulse to include reference and hardware-accelerated kernels
#include "../../../../classifier/ei_classifier_config.h"
#if 0 == 1
/* noop */
#elif EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN == 1
/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/mul.h"

#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nnfunctions.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/mul.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/process_broadcast_shapes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/memory_helpers.h"

namespace tflite {
namespace ops {
namespace micro {
namespace mul {

constexpr int kInput1Tensor = 0;
constexpr int kInput2Tensor = 1;
constexpr int kOutputTensor = 0;

struct OpData {
  int32_t output_activation_min;
  int32_t output_activation_max;

  int32_t output_multiplier;
  int output_shift;

  // Cached tensor zero point values for quantized operations.
  int32_t input1_zero_point;
  int32_t input2_zero_point;
  int32_t output_zero_point;

  float output_activation_min_f32;
  float output_activation_max_f32;
};

TfLiteStatus CalculateOpData(TfLiteContext* context, TfLiteNode* node,
                             TfLiteMulParams* params, OpData* data) {
  const TfLiteTensor* input1 = GetInput(context, node, kInput1Tensor);
  TF_LITE_ENSURE(context, input1 != nullptr);
  const TfLiteTensor* input2 = GetInput(context, node, kInput2Tensor);
  TF_LITE_ENSURE(context, input2 != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8) {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));

    double real_multiplier = static_cast<double>(input1->params.scale) *
                             static_cast<double>(input2->params.scale) /
                             static_cast<double>(output->params.scale);
    QuantizeMultiplier(real_multiplier, &data->output_multiplier,
                       &data->output_shift);

    data->input1_zero_point = input1->params.zero_point;
    data->input2_zero_point = input2->params.zero_point;
    data->output_zero_point = output->params.zero_point;
  } else {
    CalculateActivationRange(params->activation,
                             &data->output_activation_min_f32,
                             &data->output_activation_max_f32);
  }

  return kTfLiteOk;
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  OpData* data = static_cast<OpData*>(node->user_data);

  return CalculateOpData(context, node, params, data);
}

void EvalQuantized(TfLiteContext* context, TfLiteNode* node, const OpData& data,
                   const TfLiteEvalTensor* input1,
                   const TfLiteEvalTensor* input2, TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params;
  op_params.quantized_activation_min = data.output_activation_min;
  op_params.quantized_activation_max = data.output_activation_max;
  op_params.input1_offset = -data.input1_zero_point;
  op_params.input2_offset = -data.input2_zero_point;
  op_params.output_offset = data.output_zero_point;
  op_params.output_multiplier = data.output_multiplier;
  op_params.output_shift = data.output_shift;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

#define TF_LITE_MUL(type, opname, dtype)                         \
  type::opname(op_params, tflite::micro::GetTensorShape(input1), \
               tflite::micro::GetTensorData<dtype>(input1),      \
               tflite::micro::GetTensorShape(input2),            \
               tflite::micro::GetTensorData<dtype>(input2),      \
               tflite::micro::GetTensorShape(output),            \
               tflite::micro::GetTensorData<dtype>(output));

  if (output->type == kTfLiteInt8) {
    if (need_broadcast) {
      TF_LITE_MUL(reference_integer_ops, BroadcastMul4DSlow, int8_t);
    } else {
      arm_elementwise_mul_s8(
          tflite::micro::GetTensorData<int8_t>(input1),
          tflite::micro::GetTensorData<int8_t>(input2), op_params.input1_offset,
          op_params.input2_offset, tflite::micro::GetTensorData<int8_t>(output),
          op_params.output_offset, op_params.output_multiplier,
          op_params.output_shift, op_params.quantized_activation_min,
          op_params.quantized_activation_max,
          MatchingElementsSize(tflite::micro::GetTensorShape(input1),
                               tflite::micro::GetTensorShape(input2),
                               tflite::micro::GetTensorShape(output)));
    }
  } else if (output->type == kTfLiteUInt8) {
    if (need_broadcast) {
      TF_LITE_MUL(reference_integer_ops, BroadcastMul4DSlow, uint8_t);
    } else {
      TF_LITE_MUL(reference_integer_ops, Mul, uint8_t);
    }
  }
#undef TF_LITE_MUL
}

void EvalFloat(TfLiteContext* context, TfLiteNode* node,
               TfLiteMulParams* params, const OpData& data,
               const TfLiteEvalTensor* input1, const TfLiteEvalTensor* input2,
               TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params;
  op_params.float_activation_min = data.output_activation_min_f32;
  op_params.float_activation_max = data.output_activation_max_f32;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);
#define TF_LITE_MUL(opname)                                               \
  reference_ops::opname(op_params, tflite::micro::GetTensorShape(input1), \
                        tflite::micro::GetTensorData<float>(input1),      \
                        tflite::micro::GetTensorShape(input2),            \
                        tflite::micro::GetTensorData<float>(input2),      \
                        tflite::micro::GetTensorShape(output),            \
                        tflite::micro::GetTensorData<float>(output));

  if (need_broadcast) {
    TF_LITE_MUL(BroadcastMul4DSlow);
  } else {
    TF_LITE_MUL(Mul);
  }
#undef TF_LITE_MUL
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  const TfLiteEvalTensor* input1 =
      tflite::micro::GetEvalInput(context, node, kInput1Tensor);
  const TfLiteEvalTensor* input2 =
      tflite::micro::GetEvalInput(context, node, kInput2Tensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  switch (input1->type) {
    case kTfLiteUInt8:
    case kTfLiteInt8:
      EvalQuantized(context, node, data, input1, input2, output);
      break;
    case kTfLiteFloat32:
      EvalFloat(context, node, params, data, input1, input2, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input1->type), input1->type);
      return kTfLiteError;
  }

  return kTfLiteOk;
}
}  // namespace mul

TfLiteRegistration Register_MUL() {
  return {/* Init=*/mul::Init,
          /* Free=*/nullptr,
          /* Prepare=*/mul::Prepare,
          /*invoke=*/mul::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
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

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/mul.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/mul.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/process_broadcast_shapes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/memory_helpers.h"

#include "edge-impulse-sdk/porting/espressif/ESP-NN/include/esp_nn.h"
#include <esp_timer.h>

long long mul_total_time = 0;

namespace tflite {
namespace ops {
namespace micro {
namespace mul {

namespace {

constexpr int kInput1Tensor = 0;
constexpr int kInput2Tensor = 1;
constexpr int kOutputTensor = 0;

struct OpData {
  int32_t input1_zero_point;
  int32_t input2_zero_point;

  int32_t output_activation_min;
  int32_t output_activation_max;
  int32_t output_zero_point;
  int32_t output_multiplier;
  int output_shift;

  float output_activation_min_f32;
  float output_activation_max_f32;
};

TfLiteStatus CalculateOpData(TfLiteContext* context, TfLiteNode* node,
                             TfLiteMulParams* params, OpData* data) {
  const TfLiteTensor* input1 = GetInput(context, node, kInput1Tensor);
  TF_LITE_ENSURE(context, input1 != nullptr);
  const TfLiteTensor* input2 = GetInput(context, node, kInput2Tensor);
  TF_LITE_ENSURE(context, input2 != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8) {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));

    double real_multiplier = static_cast<double>(input1->params.scale) *
                             static_cast<double>(input2->params.scale) /
                             static_cast<double>(output->params.scale);
    QuantizeMultiplier(real_multiplier, &data->output_multiplier,
                       &data->output_shift);

    data->input1_zero_point = input1->params.zero_point;
    data->input2_zero_point = input2->params.zero_point;
    data->output_zero_point = output->params.zero_point;
  } else {
    CalculateActivationRange(params->activation,
                             &data->output_activation_min_f32,
                             &data->output_activation_max_f32);
  }

  return kTfLiteOk;
}

}  // namespace

void EvalQuantized(TfLiteContext* context, TfLiteNode* node,
                      const OpData* data, const TfLiteEvalTensor* input1,
                      const TfLiteEvalTensor* input2,
                      TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params = {};
  op_params.quantized_activation_min = data->output_activation_min;
  op_params.quantized_activation_max = data->output_activation_max;
  op_params.float_activation_max = data->output_activation_max_f32;
  op_params.input1_offset = -data->input1_zero_point;
  op_params.input2_offset = -data->input2_zero_point;
  op_params.output_offset = data->output_zero_point;
  op_params.output_multiplier = data->output_multiplier;
  op_params.output_shift = data->output_shift;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

  if (need_broadcast) {
    reference_integer_ops::BroadcastMul4DSlow(
        op_params, tflite::micro::GetTensorShape(input1),
        tflite::micro::GetTensorData<int8_t>(input1),
        tflite::micro::GetTensorShape(input2),
        tflite::micro::GetTensorData<int8_t>(input2),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int8_t>(output));
  } else {
    const int8_t *input1_data = tflite::micro::GetTensorData<int8_t>(input1);
    const int8_t *input2_data = tflite::micro::GetTensorData<int8_t>(input2);
    int8_t *out_data = tflite::micro::GetTensorData<int8_t>(output);

    esp_nn_mul_elementwise_s8(input1_data, input2_data, op_params.input1_offset,
                              op_params.input2_offset, out_data, op_params.output_offset,
                              op_params.output_multiplier, op_params.output_shift,
                              op_params.quantized_activation_min, op_params.quantized_activation_max,
                              MatchingElementsSize(tflite::micro::GetTensorShape(input1),
                                                    tflite::micro::GetTensorShape(input2),
                                                    tflite::micro::GetTensorShape(output)));
  }
}

void EvalQuantizedReference(TfLiteContext* context, TfLiteNode* node, const OpData* data,
                   const TfLiteEvalTensor* input1,
                   const TfLiteEvalTensor* input2, TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params = {};
  op_params.quantized_activation_min = data->output_activation_min;
  op_params.quantized_activation_max = data->output_activation_max;
  op_params.float_activation_max = data->output_activation_max_f32;
  op_params.input1_offset = -data->input1_zero_point;
  op_params.input2_offset = -data->input2_zero_point;
  op_params.output_offset = data->output_zero_point;
  op_params.output_multiplier = data->output_multiplier;
  op_params.output_shift = data->output_shift;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

  if (output->type == kTfLiteInt8) {
    if (need_broadcast) {
      reference_integer_ops::BroadcastMul4DSlow(
          op_params, tflite::micro::GetTensorShape(input1),
          tflite::micro::GetTensorData<int8_t>(input1),
          tflite::micro::GetTensorShape(input2),
          tflite::micro::GetTensorData<int8_t>(input2),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int8_t>(output));
    } else {
      reference_integer_ops::Mul(op_params,
                                 tflite::micro::GetTensorShape(input1),
                                 tflite::micro::GetTensorData<int8_t>(input1),
                                 tflite::micro::GetTensorShape(input2),
                                 tflite::micro::GetTensorData<int8_t>(input2),
                                 tflite::micro::GetTensorShape(output),
                                 tflite::micro::GetTensorData<int8_t>(output));
    }
  } else if (output->type == kTfLiteUInt8) {
    if (need_broadcast) {
      reference_integer_ops::BroadcastMul4DSlow(
          op_params, tflite::micro::GetTensorShape(input1),
          tflite::micro::GetTensorData<uint8_t>(input1),
          tflite::micro::GetTensorShape(input2),
          tflite::micro::GetTensorData<uint8_t>(input2),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<uint8_t>(output));
    } else {
      reference_integer_ops::Mul(op_params,
                                 tflite::micro::GetTensorShape(input1),
                                 tflite::micro::GetTensorData<uint8_t>(input1),
                                 tflite::micro::GetTensorShape(input2),
                                 tflite::micro::GetTensorData<uint8_t>(input2),
                                 tflite::micro::GetTensorShape(output),
                                 tflite::micro::GetTensorData<uint8_t>(output));
    }
  }
}

void EvalFloat(TfLiteContext* context, TfLiteNode* node,
               TfLiteMulParams* params, const OpData* data,
               const TfLiteEvalTensor* input1, const TfLiteEvalTensor* input2,
               TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params = {};
  op_params.float_activation_min = data->output_activation_min_f32;
  op_params.float_activation_max = data->output_activation_max_f32;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

  if (need_broadcast) {
    reference_ops::BroadcastMul4DSlow(
        op_params, tflite::micro::GetTensorShape(input1),
        tflite::micro::GetTensorData<float>(input1),
        tflite::micro::GetTensorShape(input2),
        tflite::micro::GetTensorData<float>(input2),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<float>(output));
  } else {
    reference_ops::Mul(op_params, tflite::micro::GetTensorShape(input1),
                       tflite::micro::GetTensorData<float>(input1),
                       tflite::micro::GetTensorShape(input2),
                       tflite::micro::GetTensorData<float>(input2),
                       tflite::micro::GetTensorShape(output),
                       tflite::micro::GetTensorData<float>(output));
  }
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  OpData* data = static_cast<OpData*>(node->user_data);

  return CalculateOpData(context, node, params, data);
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData* data = static_cast<const OpData*>(node->user_data);

  const TfLiteEvalTensor* input1 =
      tflite::micro::GetEvalInput(context, node, kInput1Tensor);
  const TfLiteEvalTensor* input2 =
      tflite::micro::GetEvalInput(context, node, kInput2Tensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  long long start_time = esp_timer_get_time();
  switch (input1->type) {
    case kTfLiteInt8:
      EvalQuantized(context, node, data, input1, input2, output);
      break;
    case kTfLiteInt32:
      EvalQuantizedReference(context, node, data, input1, input2, output);
      break;
    case kTfLiteFloat32:
      EvalFloat(context, node, params, data, input1, input2,
                            output);
      break;
    default:

      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input1->type), input1->type);

      return kTfLiteError;
  }
  mul_total_time += esp_timer_get_time() - start_time;
  return kTfLiteOk;
}

}  // namespace mul

TfLiteRegistration Register_MUL() {
  return {/*init=*/mul::Init,
          /*free=*/nullptr,
          /*prepare=*/mul::Prepare,
          /*invoke=*/mul::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
}  // namespace tflite
#else
/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/mul.h"

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/mul.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/process_broadcast_shapes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/memory_helpers.h"

namespace tflite {
namespace ops {
namespace micro {
namespace mul {
namespace {

constexpr int kInput1Tensor = 0;
constexpr int kInput2Tensor = 1;
constexpr int kOutputTensor = 0;

struct OpData {
  int32_t input1_zero_point;
  int32_t input2_zero_point;

  int32_t output_activation_min;
  int32_t output_activation_max;
  int32_t output_zero_point;
  int32_t output_multiplier;
  int output_shift;

  float output_activation_min_f32;
  float output_activation_max_f32;
};

TfLiteStatus CalculateOpData(TfLiteContext* context, TfLiteNode* node,
                             TfLiteMulParams* params, OpData* data) {
  const TfLiteTensor* input1 = GetInput(context, node, kInput1Tensor);
  TF_LITE_ENSURE(context, input1 != nullptr);
  const TfLiteTensor* input2 = GetInput(context, node, kInput2Tensor);
  TF_LITE_ENSURE(context, input2 != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);

  if (output->type == kTfLiteUInt8 || output->type == kTfLiteInt8) {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));

    double real_multiplier = static_cast<double>(input1->params.scale) *
                             static_cast<double>(input2->params.scale) /
                             static_cast<double>(output->params.scale);
    QuantizeMultiplier(real_multiplier, &data->output_multiplier,
                       &data->output_shift);

    data->input1_zero_point = input1->params.zero_point;
    data->input2_zero_point = input2->params.zero_point;
    data->output_zero_point = output->params.zero_point;
  } else {
    CalculateActivationRange(params->activation,
                             &data->output_activation_min_f32,
                             &data->output_activation_max_f32);
  }

  return kTfLiteOk;
}

}  // namespace

void EvalQuantized(TfLiteContext* context, TfLiteNode* node, const OpData* data,
                   const TfLiteEvalTensor* input1,
                   const TfLiteEvalTensor* input2, TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params = {};
  op_params.quantized_activation_min = data->output_activation_min;
  op_params.quantized_activation_max = data->output_activation_max;
  op_params.float_activation_max = data->output_activation_max_f32;
  op_params.input1_offset = -data->input1_zero_point;
  op_params.input2_offset = -data->input2_zero_point;
  op_params.output_offset = data->output_zero_point;
  op_params.output_multiplier = data->output_multiplier;
  op_params.output_shift = data->output_shift;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

  if (output->type == kTfLiteInt8) {
    if (need_broadcast) {
      reference_integer_ops::BroadcastMul4DSlow(
          op_params, tflite::micro::GetTensorShape(input1),
          tflite::micro::GetTensorData<int8_t>(input1),
          tflite::micro::GetTensorShape(input2),
          tflite::micro::GetTensorData<int8_t>(input2),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int8_t>(output));
    } else {
      reference_integer_ops::Mul(op_params,
                                 tflite::micro::GetTensorShape(input1),
                                 tflite::micro::GetTensorData<int8_t>(input1),
                                 tflite::micro::GetTensorShape(input2),
                                 tflite::micro::GetTensorData<int8_t>(input2),
                                 tflite::micro::GetTensorShape(output),
                                 tflite::micro::GetTensorData<int8_t>(output));
    }
  } else if (output->type == kTfLiteUInt8) {
    if (need_broadcast) {
      reference_integer_ops::BroadcastMul4DSlow(
          op_params, tflite::micro::GetTensorShape(input1),
          tflite::micro::GetTensorData<uint8_t>(input1),
          tflite::micro::GetTensorShape(input2),
          tflite::micro::GetTensorData<uint8_t>(input2),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<uint8_t>(output));
    } else {
      reference_integer_ops::Mul(op_params,
                                 tflite::micro::GetTensorShape(input1),
                                 tflite::micro::GetTensorData<uint8_t>(input1),
                                 tflite::micro::GetTensorShape(input2),
                                 tflite::micro::GetTensorData<uint8_t>(input2),
                                 tflite::micro::GetTensorShape(output),
                                 tflite::micro::GetTensorData<uint8_t>(output));
    }
  }
}

void EvalFloat(TfLiteContext* context, TfLiteNode* node,
               TfLiteMulParams* params, const OpData* data,
               const TfLiteEvalTensor* input1, const TfLiteEvalTensor* input2,
               TfLiteEvalTensor* output) {
  tflite::ArithmeticParams op_params = {};
  op_params.float_activation_min = data->output_activation_min_f32;
  op_params.float_activation_max = data->output_activation_max_f32;

  bool need_broadcast = reference_ops::ProcessBroadcastShapes(
      tflite::micro::GetTensorShape(input1),
      tflite::micro::GetTensorShape(input2), &op_params);

  if (need_broadcast) {
    reference_ops::BroadcastMul4DSlow(
        op_params, tflite::micro::GetTensorShape(input1),
        tflite::micro::GetTensorData<float>(input1),
        tflite::micro::GetTensorShape(input2),
        tflite::micro::GetTensorData<float>(input2),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<float>(output));
  } else {
    reference_ops::Mul(op_params, tflite::micro::GetTensorShape(input1),
                       tflite::micro::GetTensorData<float>(input1),
                       tflite::micro::GetTensorShape(input2),
                       tflite::micro::GetTensorData<float>(input2),
                       tflite::micro::GetTensorShape(output),
                       tflite::micro::GetTensorData<float>(output));
  }
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  OpData* data = static_cast<OpData*>(node->user_data);

  return CalculateOpData(context, node, params, data);
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  auto* params = reinterpret_cast<TfLiteMulParams*>(node->builtin_data);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData* data = static_cast<const OpData*>(node->user_data);

  const TfLiteEvalTensor* input1 =
      tflite::micro::GetEvalInput(context, node, kInput1Tensor);
  const TfLiteEvalTensor* input2 =
      tflite::micro::GetEvalInput(context, node, kInput2Tensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  switch (input1->type) {
    case kTfLiteUInt8:
    case kTfLiteInt8:
      EvalQuantized(context, node, data, input1, input2, output);
      break;
    case kTfLiteFloat32:
      EvalFloat(context, node, params, data, input1, input2, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input1->type), input1->type);
      return kTfLiteError;
  }

  return kTfLiteOk;
}
}  // namespace mul

TfLiteRegistration Register_MUL() {
  return {/*init=*/mul::Init,
          /*free=*/nullptr,
          /*prepare=*/mul::Prepare,
          /*invoke=*/mul::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace micro
}  // namespace ops
}  // namespace tflite

#endif
