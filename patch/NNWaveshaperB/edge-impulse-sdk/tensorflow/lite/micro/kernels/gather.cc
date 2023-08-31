/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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
#ifndef TF_LITE_STATIC_MEMORY

#include <stdint.h>

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/micro_utils.h"

namespace tflite {
namespace ops {
namespace micro {
namespace gather {

template <typename T, typename CoordsT = int32>
inline void Gather(const tflite::GatherParams& op_params,
                   const RuntimeShape& input_shape, const T* input_data,
                   const RuntimeShape& coords_shape, const CoordsT* coords_data,
                   const RuntimeShape& output_shape, T* output_data) {
  int axis = op_params.axis;
  if (axis < 0) {
    axis += input_shape.DimensionsCount();
  }
  TFLITE_DCHECK_GE(axis, 0);
  TFLITE_DCHECK_LT(axis, input_shape.DimensionsCount());
  const int axis_size = input_shape.Dims(axis);
  const int coords_count = coords_shape.FlatSize();

  int outer_size = 1;
  for (int i = 0; i < axis; ++i) {
    outer_size *= input_shape.Dims(i);
  }

  int inner_size = 1;
  for (int i = axis + 1; i < input_shape.DimensionsCount(); ++i) {
    inner_size *= input_shape.Dims(i);
  }

  for (int outer = 0; outer < outer_size; ++outer) {
    for (int i = 0; i < coords_count; ++i) {
      TFLITE_DCHECK_GE(coords_data[i], 0);
      TFLITE_DCHECK_LT(coords_data[i], axis_size);
      // TODO(rsun): replace memcpy with a for loop
      std::memcpy(
          output_data + (outer * coords_count + i) * inner_size,
          input_data + (outer * axis_size + coords_data[i]) * inner_size,
          sizeof(T) * inner_size);
    }
  }
}

constexpr int kInputTensor = 0;
constexpr int kInputPositions = 1;
constexpr int kOutputTensor = 0;

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const auto* params =
      reinterpret_cast<const TfLiteGatherParams*>(node->builtin_data);

  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* positions;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputPositions, &positions));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  switch (positions->type) {
    case kTfLiteInt64:
    case kTfLiteInt32:
      break;
    default:
      context->ReportError(
          context, "Positions of type '%s' are not supported by gather.",
          TfLiteTypeGetName(positions->type));
      return kTfLiteError;
  }

  // Assign to output the input type.
  output->type = input->type;

  // Check conditions for different types.
  switch (input->type) {
    case kTfLiteFloat32:
    case kTfLiteUInt8:
    case kTfLiteInt8:
    case kTfLiteInt16:
    case kTfLiteInt64:
    case kTfLiteInt32:
    case kTfLiteBool:
      break;
    default:
      context->ReportError(context, "Type '%s' is not supported by gather.",
                           TfLiteTypeGetName(input->type));
      return kTfLiteError;
  }

  int axis = params->axis;
  if (axis < 0) {
    axis += NumDimensions(input);
  }
  TF_LITE_ENSURE(context, 0 <= axis && axis < NumDimensions(input));

  const int num_dimensions =
      NumDimensions(input) + NumDimensions(positions) - 1;
  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(num_dimensions);
  int output_index = 0;
  for (int i = 0; i < axis; ++i) {
    output_shape->data[output_index++] = input->dims->data[i];
  }
  for (int i = 0; i < positions->dims->size; ++i) {
    output_shape->data[output_index++] = positions->dims->data[i];
  }
  for (int i = axis + 1; i < input->dims->size; ++i) {
    output_shape->data[output_index++] = input->dims->data[i];
  }

  return kTfLiteOk;
}

template <typename InputT, typename PositionsT>
TfLiteStatus Gather(const TfLiteGatherParams& params, const TfLiteTensor* input,
                    const TfLiteTensor* positions, TfLiteTensor* output) {
  tflite::GatherParams op_params;
  op_params.axis = params.axis;
  Gather(op_params, GetTensorShape(input),
                    GetTensorData<InputT>(input), GetTensorShape(positions),
                    GetTensorData<PositionsT>(positions),
                    GetTensorShape(output), GetTensorData<InputT>(output));
  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const auto* params =
      reinterpret_cast<const TfLiteGatherParams*>(node->builtin_data);
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* positions;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputPositions, &positions));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  if (positions->type == kTfLiteInt32) {
    switch (input->type) {
      case kTfLiteFloat32:
        return Gather<float, int32_t>(*params, input, positions, output);
      case kTfLiteUInt8:
        return Gather<uint8_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt8:
        return Gather<int8_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt16:
        return Gather<int16_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt32:
        return Gather<int32_t, int32_t>(*params, input, positions, output);
      case kTfLiteInt64:
        return Gather<int64_t, int32_t>(*params, input, positions, output);
      case kTfLiteBool:
        return Gather<bool, int32_t>(*params, input, positions, output);
      default:
        context->ReportError(context, "Type '%s' is not supported by gather.",
                             TfLiteTypeGetName(input->type));
        return kTfLiteError;
    }
  }
  if (positions->type == kTfLiteInt64) {
    switch (input->type) {
      case kTfLiteFloat32:
        return Gather<float, int64_t>(*params, input, positions, output);
      case kTfLiteUInt8:
        return Gather<uint8_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt8:
        return Gather<int8_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt16:
        return Gather<int16_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt32:
        return Gather<int32_t, int64_t>(*params, input, positions, output);
      case kTfLiteInt64:
        return Gather<int64_t, int64_t>(*params, input, positions, output);
      case kTfLiteBool:
        return Gather<bool, int64_t>(*params, input, positions, output);
      default:
        context->ReportError(context, "Type '%s' is not supported by gather.",
                             TfLiteTypeGetName(input->type));
        return kTfLiteError;
    }
  }
  context->ReportError(context,
                       "Positions of type '%s' are not supported by gather.",
                       TfLiteTypeGetName(positions->type));
  return kTfLiteError;
}

}  // namespace gather
}  // namespace micro
}  // namespace ops

TfLiteRegistration Register_GATHER() {
  return {/*init=*/nullptr,
          /*free=*/nullptr,
          /*prepare=*/ops::micro::gather::Prepare,
          /*invoke=*/ops::micro::gather::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#endif // TF_LITE_STATIC_MEMORY
