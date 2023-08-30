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

#include <array>
#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/portable_tensor.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/micro_utils.h"

namespace tflite {
namespace ops {
namespace micro {
namespace slice {

constexpr int kInputTensor = 0;
constexpr int kBeginTensor = 1;
constexpr int kSizeTensor = 2;
constexpr int kOutputTensor = 0;

// This Op only supports 1-5D cases and since we use the optimized ops 5D
// implementation, the 1-4D tensors are mapped to 5D.
const int kMaxDim = 5;


template <typename T>
TfLiteStatus CalculateOutputShapeVector(TfLiteContext* context,
                                        const TfLiteTensor* input,
                                        const TfLiteTensor* begin,
                                        const TfLiteTensor* size,
                                        std::vector<int>* output_shape_vector) {
  for (int idx = 0; idx < NumDimensions(input); ++idx) {
    T size_value = GetTensorData<T>(size)[idx];
    if (size_value < 0) {
      if (size_value != -1) {
        context->ReportError(context, "Invalid size.");
        return kTfLiteError;
      }
      size_value = SizeOfDimension(input, idx) - GetTensorData<T>(begin)[idx];
    } else {
      if (SizeOfDimension(input, idx) <
          GetTensorData<T>(begin)[idx] + size_value) {
        context->ReportError(context, "Invalid begin and size.");
        return kTfLiteError;
      }
    }
    output_shape_vector->push_back(static_cast<int>(size_value));
  }
  return kTfLiteOk;
}

template <typename T>
void GetBeginAndSizeVectors(int dimensions, const TfLiteTensor* begin,
                            const TfLiteTensor* size, std::vector<int>* begins,
                            std::vector<int>* sizes) {
  for (int idx = 0; idx < dimensions; ++idx) {
    begins->push_back(GetTensorData<T>(begin)[idx]);
    sizes->push_back(GetTensorData<T>(size)[idx]);
  }
}

template <typename T>
inline void Slice(const tflite::SliceParams& op_params,
                  const RuntimeShape& input_shape,
                  const RuntimeShape& output_shape,
                  SequentialTensorWriter<T>* writer) {
  const RuntimeShape ext_shape = RuntimeShape::ExtendedShape(5, input_shape);
  TFLITE_DCHECK_LE(op_params.begin_count, 5);
  TFLITE_DCHECK_LE(op_params.size_count, 5);
  const int begin_count = op_params.begin_count;
  const int size_count = op_params.size_count;
  // We front-pad the begin and size vectors.
  std::array<int, 5> start;
  std::array<int, 5> stop;
  for (int i = 0; i < 5; ++i) {
    int padded_i = 5 - i;
    start[i] =
        begin_count < padded_i ? 0 : op_params.begin[begin_count - padded_i];
    stop[i] =
        (size_count < padded_i || op_params.size[size_count - padded_i] == -1)
            ? ext_shape.Dims(i)
            : start[i] + op_params.size[size_count - padded_i];
  }

  for (int i0 = start[0]; i0 < stop[0]; ++i0) {
    for (int i1 = start[1]; i1 < stop[1]; ++i1) {
      for (int i2 = start[2]; i2 < stop[2]; ++i2) {
        for (int i3 = start[3]; i3 < stop[3]; ++i3) {
          for (int i4 = start[4]; i4 < stop[4]; ++i4) {
            writer->Write(Offset(ext_shape, i0, i1, i2, i3, i4));
          }
        }
      }
    }
  }
}

template <typename T>
inline void Slice(const tflite::SliceParams& op_params,
                  const RuntimeShape& input_shape, const T* input_data,
                  const RuntimeShape& output_shape, T* output_data) {
  SequentialTensorWriter<T> writer(input_data, output_data);
  return Slice(op_params, input_shape, output_shape, &writer);
}

template <typename T>
inline void Slice(const tflite::SliceParams& op_params,
                  const RuntimeShape& input_shape, const TfLiteTensor* input,
                  const RuntimeShape& output_shape, TfLiteTensor* output) {
  SequentialTensorWriter<T> writer(input, output);
  return Slice(op_params, input_shape, output_shape, &writer);
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* begin;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kBeginTensor, &begin));
  const TfLiteTensor* size;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kSizeTensor, &size));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  // Ensure validity of input tensor and its dimension.
  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  TF_LITE_ENSURE(context,
                 begin->type == kTfLiteInt32 || begin->type == kTfLiteInt64);
  TF_LITE_ENSURE(context,
                 size->type == kTfLiteInt32 || size->type == kTfLiteInt64);
  TF_LITE_ENSURE_EQ(context, NumDimensions(begin), 1);
  TF_LITE_ENSURE_EQ(context, NumDimensions(size), 1);
  TF_LITE_ENSURE_EQ(context, NumElements(begin), NumElements(size));
  TF_LITE_ENSURE_MSG(context, NumDimensions(input) <= kMaxDim,
                     "Slice op only supports 1D-5D input arrays.");

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* begin;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kBeginTensor, &begin));
  const TfLiteTensor* size;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kSizeTensor, &size));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  std::vector<int> begins;
  begins.reserve(kMaxDim);
  std::vector<int> sizes;
  sizes.reserve(kMaxDim);

  for (int i = NumDimensions(input); i < kMaxDim; ++i) {
    begins.push_back(0);
    sizes.push_back(1);
  }

  if (begin->type == kTfLiteInt32) {
    GetBeginAndSizeVectors<int32_t>(NumDimensions(input), begin, size, &begins,
                                    &sizes);
  } else if (begin->type == kTfLiteInt64) {
    GetBeginAndSizeVectors<int64_t>(NumDimensions(input), begin, size, &begins,
                                    &sizes);
  } else {
    context->ReportError(
        context, "Type %d is currently not supported by Slice.", begin->type);
    return kTfLiteError;
  }

  // The Slice op implementation only accepts 5-D sizes. That constraint is, for
  // the present, maintained here.
  //
  // The dimensions in the kernel used to be in reverse-order, and TFLite
  // arranged the begins and sizes vectors accordingly. This macro incorporates
  // the needed reversing.
#define TF_LITE_SLICE(data_type)                                               \
  {                                                                            \
    TF_LITE_ENSURE_EQ(context, begins.size(), kMaxDim);                        \
    TF_LITE_ENSURE_EQ(context, sizes.size(), kMaxDim);                         \
    tflite::SliceParams op_params;                                             \
    op_params.begin_count = kMaxDim;                                           \
    op_params.size_count = kMaxDim;                                            \
    for (int i = 0; i < kMaxDim; ++i) {                                        \
      op_params.begin[i] = begins[i];                                          \
      op_params.size[i] = sizes[i];                                            \
    }                                                                          \
                                                                               \
    Slice<data_type>(op_params, GetTensorShape(input), input, \
                     GetTensorShape(output), output);         \
  }

  switch (input->type) {
    case kTfLiteFloat32:
      TF_LITE_SLICE(float);
      break;
    case kTfLiteInt32:
      TF_LITE_SLICE(int32_t);
      break;
    case kTfLiteInt64:
      TF_LITE_SLICE(int64_t);
      break;
    case kTfLiteInt8:
      TF_LITE_SLICE(int8_t);
      break;
    case kTfLiteInt16:
      TF_LITE_SLICE(int16_t);
      break;
    case kTfLiteUInt8:
      TF_LITE_SLICE(uint8_t);
      break;
    case kTfLiteBool:
      TF_LITE_SLICE(bool);
      break;
    default:
      context->ReportError(
          context, "Type %d is currently not supported by Slice.", input->type);
      return kTfLiteError;
  }
#undef TF_LITE_SLICE
  return kTfLiteOk;
}

}  // namespace slice
}  // namespace micro
}  // namespace ops

TfLiteRegistration Register_SLICE() {
  return {/*init=*/nullptr,
          /*free=*/nullptr,
          /*prepare=*/ops::micro::slice::Prepare,
          /*invoke=*/ops::micro::slice::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
