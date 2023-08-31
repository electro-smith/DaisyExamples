/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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
#include <stdint.h>

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/compatibility.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/transpose.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace ops {
namespace builtin {
namespace transpose {

constexpr int kInputTensor = 0;
constexpr int kPermTensor = 1;
constexpr int kOutputTensor = 0;

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* perm = GetInput(context, node, kPermTensor);
  TF_LITE_ENSURE(context, perm != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  // Ensure validity of input tensor.
  TF_LITE_ENSURE_MSG(context, NumDimensions(input) <= 5,
                     "Transpose op only supports 1D-5D input arrays.");
  TF_LITE_ENSURE_TYPES_EQ(context, input->type,
                          output->type);

  int dims = NumDimensions(input);
  const int32_t* perm_data = perm->data.i32;

  // Ensure validity of the permutations tensor as a 1D tensor.
  TF_LITE_ENSURE_EQ(context, NumDimensions(perm), 1);
  TF_LITE_ENSURE_EQ(context, perm->dims->data[0], dims);
  for (int idx = 0; idx < dims; ++idx) {
    TF_LITE_ENSURE_MSG(context, (perm_data[idx] >= 0 && perm_data[idx] < dims),
                       "Transpose op permutations array is out of bounds.");
  }

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* perm_tensor =
      tflite::micro::GetEvalInput(context, node, kPermTensor);
  const int32_t* perm_data = perm_tensor->data.i32;
  const int size = perm_tensor->dims->data[0];
  TransposeParams params;
  params.perm_count = size;
  for (int i = 0; i < size; ++i) {
    params.perm[i] = perm_data[i];
  }

  // Transpose kernel only does rearranging values not numeric evaluations
  // on each cell. It's safe to implement per size of scalar type and this
  // trick keeps the total code size in a reasonable range.
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);
  switch (input->type) {
    case kTfLiteFloat32:
      reference_ops::Transpose(params, tflite::micro::GetTensorShape(input),
                               input->data.f,
                               tflite::micro::GetTensorShape(output),
                               output->data.f);
      break;
    case kTfLiteInt8:
      reference_ops::Transpose(params, tflite::micro::GetTensorShape(input),
                               input->data.int8,
                               tflite::micro::GetTensorShape(output),
                               output->data.int8);
      break;
    case kTfLiteInt32:
      reference_ops::Transpose(params, tflite::micro::GetTensorShape(input),
                               input->data.i32,
                               tflite::micro::GetTensorShape(output),
                               output->data.i32);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }

  return kTfLiteOk;
}

}  // namespace transpose
}  // namespace builtin
}  // namespace ops

TfLiteRegistration Register_TRANSPOSE() {
  return {/*init=*/nullptr,
          /*free=*/nullptr,
          /*prepare=*/ops::builtin::transpose::Prepare,
          /*invoke=*/ops::builtin::transpose::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
