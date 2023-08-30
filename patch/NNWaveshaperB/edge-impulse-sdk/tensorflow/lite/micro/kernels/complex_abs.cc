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

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <complex>

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/micro_utils.h"

namespace tflite {
namespace ops {
namespace micro {
namespace complex_abs {

using std::complex;

constexpr int kInputTensor = 0;
constexpr int kOutputTensor = 0;

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  if (input->type != kTfLiteComplex64 || output->type != kTfLiteFloat32) {
      TF_LITE_KERNEL_LOG(context, "Types input %s (%d), output %s (%d) not supported.",
                          TfLiteTypeGetName(input->type), input->type,
                          TfLiteTypeGetName(output->type), output->type);
      return kTfLiteError;
  }

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  size_t total_input_els = 1;
  for (size_t dim_ix = 0; dim_ix < input->dims->size; dim_ix++) {
      total_input_els *= input->dims->data[dim_ix];
  }

  for (size_t ix = 0; ix < total_input_els; ix++) {
    output->data.f[ix] = sqrt(pow(input->data.c64[ix].re, 2) + pow(input->data.c64[ix].im, 2));
  }

  return kTfLiteOk;
}

}  // namespace complex_abs
}  // namespace micro
}  // namespace ops

TfLiteRegistration Register_COMPLEX_ABS() {
  return {/*init=*/nullptr,
          /*free=*/nullptr,
          /*prepare=*/ops::micro::complex_abs::Prepare,
          /*invoke=*/ops::micro::complex_abs::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
