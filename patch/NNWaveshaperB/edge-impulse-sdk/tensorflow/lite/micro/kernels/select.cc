/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include <stddef.h>
#include <stdint.h>

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/portable_tensor.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/micro_utils.h"

namespace tflite {
namespace ops {
namespace micro {
namespace select {

constexpr int kInputTensorCondition = 0;
constexpr int kInputTensorX = 1;
constexpr int kInputTensorY = 2;
constexpr int kOutputTensor = 0;

enum KernelType {
  kVersionOne,
  kVersionTwo,
};

struct OpData {
  bool requires_broadcast;
  // True if input condition is scalar or input condition has rank one and
  // matches the first dimension of other inputs.
  bool has_low_rank_input_condition;
};

template <typename D, typename T>
void Select(const RuntimeShape& input_condition_shape,
            const D* input_condition_data, const RuntimeShape& input_x_shape,
            const T* input_x_data, const RuntimeShape& input_y_shape,
            const T* input_y_data, const RuntimeShape& output_shape,
            T* output_data) {
  const int64_t flatsize = MatchingFlatSize(
      input_condition_shape, input_x_shape, input_y_shape, output_shape);
  for (int64_t i = 0; i < flatsize; ++i) {
    output_data[i] =
        input_condition_data[i] ? input_x_data[i] : input_y_data[i];
  }
}

template <typename D, typename T>
void RankOneSelect(const RuntimeShape& input_condition_shape,
                   const D* input_condition_data,
                   const RuntimeShape& input_x_shape, const T* input_x_data,
                   const RuntimeShape& input_y_shape, const T* input_y_data,
                   const RuntimeShape& output_shape, T* output_data) {
  const int64_t outer_size = input_condition_shape.FlatSize();
  int64_t inner_size;
  if (input_condition_shape.DimensionsCount() == 0) {
    inner_size = MatchingFlatSize(input_x_shape, input_y_shape, output_shape);
  } else {
    TFLITE_DCHECK_EQ(
        MatchingDim(input_x_shape, 0, input_y_shape, 0, output_shape, 0),
        outer_size);
    inner_size =
        MatchingFlatSizeSkipDim(input_x_shape, 0, input_y_shape, output_shape);
  }

  int64_t offset = 0;
  for (int64_t i = 0; i < outer_size; i++) {
    const T* input_data = input_condition_data[i] ? input_x_data : input_y_data;
    memcpy(output_data + offset, input_data + offset, inner_size * sizeof(T));
    offset += inner_size;
  }
}

template <typename D, typename T>
void BroadcastSelect4DSlow(const RuntimeShape& input_condition_shape,
                           const D* input_condition_data,
                           const RuntimeShape& input_x_shape,
                           const T* input_x_data,
                           const RuntimeShape& input_y_shape,
                           const T* input_y_data,
                           const RuntimeShape& output_shape, T* output_data) {
  TFLITE_DCHECK_LE(input_condition_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_LE(input_x_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_LE(input_y_shape.DimensionsCount(), 4);
  TFLITE_DCHECK_LE(output_shape.DimensionsCount(), 4);

  const RuntimeShape extended_output_shape =
      RuntimeShape::ExtendedShape(4, output_shape);

  NdArrayDesc<4> desc_condition;
  NdArrayDesc<4> desc_x;
  NdArrayDesc<4> desc_y;
  NdArrayDescsForElementwiseBroadcast(input_condition_shape, input_x_shape,
                                      input_y_shape, &desc_condition, &desc_x,
                                      &desc_y);

  // In Tensorflow, the dimensions are canonically named (batch_number, row,
  // col, channel), with extents (batches, height, width, depth), with the
  // trailing dimension changing most rapidly (channels has the smallest
  // stride, typically 1 element).
  //
  // In generated C code, we store arrays with the dimensions reversed. The
  // first dimension has smallest stride.
  //
  // We name our variables by their Tensorflow convention, but generate C code
  // nesting loops such that the innermost loop has the smallest stride for
  // the best cache behavior.
  for (int b = 0; b < extended_output_shape.Dims(0); ++b) {
    for (int y = 0; y < extended_output_shape.Dims(1); ++y) {
      for (int x = 0; x < extended_output_shape.Dims(2); ++x) {
        for (int c = 0; c < extended_output_shape.Dims(3); ++c) {
          const int condition_index =
              SubscriptToIndex(desc_condition, b, y, x, c);
          const int x_index = SubscriptToIndex(desc_x, b, y, x, c);
          const int y_index = SubscriptToIndex(desc_y, b, y, x, c);
          output_data[Offset(extended_output_shape, b, y, x, c)] =
              input_condition_data[condition_index] ? input_x_data[x_index]
                                                    : input_y_data[y_index];
        }
      }
    }
  }
}

void* SelectInit(TfLiteContext* context, const char* buffer, size_t length) {
  auto* data = new OpData;
  data->requires_broadcast = false;
  data->has_low_rank_input_condition = false;
  return data;
}

void SelectFree(TfLiteContext* context, void* buffer) {
  delete reinterpret_cast<OpData*>(buffer);
}

template <KernelType kernel_type>
TfLiteStatus SelectPrepare(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input_condition;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensorCondition,
                                          &input_condition));
  const TfLiteTensor* input_x;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensorX, &input_x));
  const TfLiteTensor* input_y;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensorY, &input_y));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  // Input must be bool.
  TF_LITE_ENSURE_TYPES_EQ(context, input_condition->type, kTfLiteBool);
  TF_LITE_ENSURE_TYPES_EQ(context, input_x->type, input_y->type);
  output->type = input_x->type;

  bool same_shape = HaveSameShapes(input_condition, input_x) &&
                    HaveSameShapes(input_x, input_y);
  TfLiteIntArray* output_size;
  if (!same_shape) {
    switch (kernel_type) {
      case kVersionOne: {
        bool is_input_condition_scalar = NumDimensions(input_condition) == 0;
        bool has_rank_one_input_condition =
            NumDimensions(input_condition) == 1 &&
            SizeOfDimension(input_condition, 0) == SizeOfDimension(input_x, 0);
        data->has_low_rank_input_condition =
            is_input_condition_scalar || has_rank_one_input_condition;
        TF_LITE_ENSURE(context, data->has_low_rank_input_condition);

        output_size = TfLiteIntArrayCopy(input_x->dims);

        // Input tensors must have the same type and size
        TF_LITE_ENSURE(context, HaveSameShapes(input_x, input_y));
        break;
      }
      case kVersionTwo: {
        TF_LITE_ENSURE_OK(context, CalculateShapeForBroadcast(
                                       context, input_condition, input_x,
                                       input_y, &output_size));
        data->requires_broadcast = true;
        break;
      }
      default:
        return kTfLiteError;
    }
  } else {
    output_size = TfLiteIntArrayCopy(input_x->dims);
  }

  return kTfLiteOk;
}

TfLiteStatus SelectEval(TfLiteContext* context, TfLiteNode* node) {
  OpData* data = reinterpret_cast<OpData*>(node->user_data);
  const TfLiteTensor* input_condition;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensorCondition,
                                          &input_condition));
  const TfLiteTensor* input_x;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensorX, &input_x));
  const TfLiteTensor* input_y;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensorY, &input_y));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

#define TF_LITE_SELECT(type, op)                                           \
  op(GetTensorShape(input_condition),                       \
     GetTensorData<bool>(input_condition),                  \
     GetTensorShape(input_x), GetTensorData<type>(input_x), \
     GetTensorShape(input_y), GetTensorData<type>(input_y), \
     GetTensorShape(output), GetTensorData<type>(output));

#define TF_LITE_SWITCH(type, op)                                               \
  switch (type) {                                                              \
    break;                                                                     \
    case kTfLiteBool:                                                          \
      TF_LITE_SELECT(bool, op);                                                \
      break;                                                                   \
    case kTfLiteFloat32:                                                       \
      TF_LITE_SELECT(float, op);                                               \
      break;                                                                   \
    case kTfLiteUInt8:                                                         \
      TF_LITE_SELECT(uint8_t, op);                                             \
      break;                                                                   \
    case kTfLiteInt8:                                                          \
      TF_LITE_SELECT(int8_t, op);                                              \
      break;                                                                   \
    case kTfLiteInt16:                                                         \
      TF_LITE_SELECT(int16_t, op);                                             \
      break;                                                                   \
    case kTfLiteInt32:                                                         \
      TF_LITE_SELECT(int32_t, op);                                             \
      break;                                                                   \
    case kTfLiteInt64:                                                         \
      TF_LITE_SELECT(int64_t, op);                                             \
      break;                                                                   \
    default:                                                                   \
      context->ReportError(context,                                            \
                           "Does not support type other than bool|float|int, " \
                           "got %d",                                           \
                           type);                                              \
      return kTfLiteError;                                                     \
  }

  if (data->has_low_rank_input_condition) {
    TF_LITE_SWITCH(input_x->type, RankOneSelect);
  } else if (data->requires_broadcast) {
    TF_LITE_SWITCH(input_x->type, BroadcastSelect4DSlow);
  } else {
    TF_LITE_SWITCH(input_x->type, Select);
  }

#undef TF_LITE_SELECT
#undef TF_LITE_SWITCH
  return kTfLiteOk;
}

}  // namespace select
}  // namespace micro
}  // namespace ops

TfLiteRegistration Register_SELECT() {
  return {/*init=*/ops::micro::select::SelectInit,
          /*free=*/ops::micro::select::SelectFree,
          /*prepare=*/ops::micro::select::SelectPrepare<ops::micro::select::kVersionOne>,
          /*invoke=*/ops::micro::select::SelectEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

TfLiteRegistration Register_SELECT_V2() {
  return {/*init=*/ops::micro::select::SelectInit,
          /*free=*/ops::micro::select::SelectFree,
          /*prepare=*/ops::micro::select::SelectPrepare<ops::micro::select::kVersionTwo>,
          /*invoke=*/ops::micro::select::SelectEval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#endif // TF_LITE_STATIC_MEMORY
