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
#include "edge-impulse-sdk/dsp/kissfft/kiss_fftr.h"

namespace tflite {
namespace ops {
namespace micro {
namespace rfft2d {

using std::complex;

constexpr int kInputTensor = 0;
constexpr int kFftLengthTensor = 1;
constexpr int kOutputTensor = 0;

struct OpData {
  int     kiss_fft_output_buffer_index;
};

bool IsPowerOfTwo(uint32_t v) { return v && !(v & (v - 1)); }

static int software_rfft(float *fft_input, TfLiteComplex64 *output, size_t n_fft, size_t n_fft_out_features, kiss_fft_cpx *fft_output) {
  size_t kiss_fftr_mem_length;

  // create fftr context (this should move to a scratch buffer...)
  kiss_fftr_cfg cfg = kiss_fftr_alloc(n_fft, 0, NULL, NULL, &kiss_fftr_mem_length);
  if (!cfg) {
      ei_free(fft_output);
      return -1;
  }

  // execute the rfft operation
  kiss_fftr(cfg, fft_input, fft_output);

  // and write back to the output
  for (size_t ix = 0; ix < n_fft_out_features; ix++) {
      output[ix].re = fft_output[ix].r;
      output[ix].im = fft_output[ix].i;
  }

  ei_free(cfg);

  return 0;
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  (void)buffer;
  (void)length;
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  // Check type and shape of the input tensor
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  TF_LITE_ENSURE(context, NumDimensions(input) >= 2);
  if (input->type != kTfLiteFloat32) {
    context->ReportError(context,
                         "Type '%s' for input is not supported by rfft2d.",
                         TfLiteTypeGetName(input->type));
    return kTfLiteError;
  }

  // Check type and shape of the fft_length tensor
  const TfLiteTensor* fft_length;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kFftLengthTensor, &fft_length));
  const RuntimeShape fft_length_shape = GetTensorShape(fft_length);

  TF_LITE_ENSURE_EQ(context, NumDimensions(fft_length), 1);
  TF_LITE_ENSURE_EQ(context, fft_length_shape.Dims(0), 2);
  if (fft_length->type != kTfLiteInt32) {
    context->ReportError(context,
                         "Type '%s' for fft_length is not supported by rfft2d.",
                         TfLiteTypeGetName(fft_length->type));
    return kTfLiteError;
  }

  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  OpData* data = static_cast<OpData*>(node->user_data);

  size_t output_els = output->bytes / sizeof(TfLiteComplex64);

  TF_LITE_ENSURE_STATUS(
    context->RequestScratchBufferInArena(
              context, output_els * sizeof(kiss_fft_cpx), &data->kiss_fft_output_buffer_index));


  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* fft_length;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kFftLengthTensor, &fft_length));
  const int32_t* fft_length_data = GetTensorData<int32_t>(fft_length);
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  if (output->type != kTfLiteComplex64) {
    context->ReportError(context,
                         "Type '%s' for output is not supported by rfft2d.",
                         TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  TF_LITE_ENSURE(context, IsPowerOfTwo(fft_length_data[0]));
  TF_LITE_ENSURE(context, IsPowerOfTwo(fft_length_data[1]));

  int fft_height, fft_width;
  fft_height = fft_length_data[0];
  fft_width = fft_length_data[1];

  OpData* data = static_cast<OpData*>(node->user_data);

  if (fft_height != 1) {
    context->ReportError(context,
                      "Only supports fft_height 1",
                      TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  kiss_fft_cpx* shift_buffer = (kiss_fft_cpx*)context->GetScratchBuffer(context, data->kiss_fft_output_buffer_index);

  size_t in_row_els = 1;
  for (size_t ix = 1; ix < input->dims->size; ix++) {
    in_row_els *= input->dims->data[ix];
  }
  size_t out_row_els = 1;
  for (size_t ix = 1; ix < output->dims->size; ix++) {
    out_row_els *= output->dims->data[ix];
  }

  for (size_t row = 0; row < input->dims->data[0]; row++) {
    float *in_ptr = &input->data.f[row * in_row_els];
    auto out_ptr = &output->data.c64[row * out_row_els];

    int x = software_rfft(in_ptr, out_ptr, fft_width, in_row_els, shift_buffer);
    if (x != 0) {
      context->ReportError(context,
                    "software_rfft failed (%d)",
                    x);
      return kTfLiteError;
    }
  }

  return kTfLiteOk;
}

}  // namespace rfft2d
}  // namespace micro
}  // namespace ops

TfLiteRegistration Register_RFFT2D() {
  return {/*init=*/ops::micro::rfft2d::Init,
          /*free=*/nullptr,
          /*prepare=*/ops::micro::rfft2d::Prepare,
          /*invoke=*/ops::micro::rfft2d::Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite
