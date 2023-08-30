// Patched by Edge Impulse to include reference and hardware-accelerated kernels
#include "../../../../classifier/ei_classifier_config.h"
#if 0 == 1
/* noop */
#elif EI_CLASSIFIER_TFLITE_ENABLE_SILABS_MVP == 1

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/transpose_conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/transpose_conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

#include "sl_mvp_ml_transpose_conv2d.h"

namespace tflite {
namespace sl {
namespace transpose_conv2d {

constexpr int kFilterTensor = 1;
constexpr int kInputTensor  = 2;
constexpr int kBiasTensor   = 3;
constexpr int kOutputTensor = 0;

// TransposeConv is quantized along dimension 0 of filter tensor.
// https://www.tensorflow.org/lite/performance/quantization_spec
constexpr int kConvQuantizedDimension = 0;

enum op_support { kMvp, kTFLMrefF32, kTFLMrefI8 };

struct OpData {
  op_support  supported;
  int         scratch_buffer_index;
  sli_mvp_ml_transpose_conv2d_s8_params_t op_params;

  // Per channel output multiplier and shift.
  int32_t     *per_channel_output_multiplier;
  int32_t     *per_channel_output_shift;
};

inline float16_t normalize_fp16(float f)
{
  return (float16_t)std::min(std::max(f, SLI_MVP_FP16_MIN), SLI_MVP_FP16_MAX);
}

inline PaddingType RuntimePaddingType(TfLitePadding padding)
{
  switch (padding) {
    case TfLitePadding::kTfLitePaddingSame:
      return PaddingType::kSame;
    case TfLitePadding::kTfLitePaddingValid:
      return PaddingType::kValid;
    case TfLitePadding::kTfLitePaddingUnknown:
    default:
      return PaddingType::kNone;
  }
}

TfLiteStatus PopulateConvolutionQuantizationParams(
    TfLiteContext* context,
    const TfLiteTensor* input,
    const TfLiteTensor* filter,
    TfLiteTensor* output,
    const TfLiteFusedActivation& activation,
    int32_t* output_activation_min, int32_t* output_activation_max,
    float16_t* per_channel_scalers, int num_channels, float accumulator_multipler)
{
  auto affine_quantization =
        reinterpret_cast<const TfLiteAffineQuantization*>(filter->quantization.params);

  // Populate multiplier and shift using affine quantization.
  const float input_scale = input->params.scale;
  const float output_scale = output->params.scale;
  const float* filter_scales = affine_quantization->scale->data;

  for (int i = 0; i < num_channels; ++i) {
    // If per-tensor quantization parameter is specified, broadcast it along the
    // quantization dimension.
    const float filter_scale = filter_scales[i];
    const float effective_output_scale = (input_scale * filter_scale) / output_scale;
    const float acc_output_scale = effective_output_scale * accumulator_multipler;
    per_channel_scalers[i] = normalize_fp16(acc_output_scale);
  }

  TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
          context, activation, output, output_activation_min,
          output_activation_max));

  return kTfLiteOk;
}

void *Init(TfLiteContext* context, const char* buffer, size_t length)
{
  (void)buffer;
  (void)length;
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node)
{
  int scratch_buffer_size = 0;

  TFLITE_DCHECK(node->user_data    != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  const auto params = static_cast<const TfLiteTransposeConvParams*>(node->builtin_data);

  TfLiteTensor* output       = GetOutput(context, node, kOutputTensor);
  const TfLiteTensor* bias   = GetOptionalInputTensor(context, node, kBiasTensor);
  const TfLiteTensor* input  = GetInput(context, node, kInputTensor);
  const TfLiteTensor* filter = GetInput(context, node, kFilterTensor);
  TF_LITE_ENSURE(context, input  != nullptr);
  TF_LITE_ENSURE(context, output != nullptr);
  TF_LITE_ENSURE(context, filter != nullptr);

  data->op_params.batches         = input->dims->data[0];
  data->op_params.in_channels     = input->dims->data[3];
  data->op_params.input_height    = input->dims->data[1];
  data->op_params.input_width     = input->dims->data[2];
  data->op_params.out_channels    = filter->dims->data[kConvQuantizedDimension];
  data->op_params.output_height   = output->dims->data[1];
  data->op_params.output_width    = output->dims->data[2];
  data->op_params.filter_height   = filter->dims->data[1];
  data->op_params.filter_width    = filter->dims->data[2];
  data->op_params.input_offset    = -input->params.zero_point;
  data->op_params.output_offset   = output->params.zero_point;
  data->op_params.stride_height   = params->stride_height;
  data->op_params.stride_width    = params->stride_width;
  data->op_params.padding         = params->padding == kTfLitePaddingSame;

  int dummy_height, dummy_width;
  const auto padding = ComputePaddingHeightWidth(
                         params->stride_height, params->stride_width,
                         1, 1, //dilation_rate_height and dilation_rate_width
                         data->op_params.input_height, data->op_params.input_width,
                         data->op_params.filter_height, data->op_params.filter_width,
                         params->padding,
                         &dummy_height, &dummy_width);

  data->op_params.pad_height = padding.height;
  data->op_params.pad_width  = padding.width;

  const int num_channels = data->op_params.out_channels;

  if (input->type == kTfLiteInt8) {
    if (sli_mvp_ml_transpose_conv2d_s8_is_supported(&data->op_params)) {
      data->supported = kMvp;
      scratch_buffer_size = GetTensorShape(output).FlatSize() * sizeof(float16_t);

      float16_t *bias_data = static_cast<float16_t*>(context->AllocatePersistentBuffer(
                             context, num_channels * sizeof(float16_t)));
      if(bias != nullptr) {
        data->op_params.bias = bias_data;
        int32_t i32_bias;
        for(int i = 0; i < num_channels; i++) {
          i32_bias = bias->data.i32[i];
          bias_data[i] = float16_t(i32_bias * SLI_MVP_ACCUMULATOR_SCALER);
        }
      } else {
        data->op_params.bias = nullptr;
      }

      float16_t *scaler_data = static_cast<float16_t*>(context->AllocatePersistentBuffer(
                               context, num_channels * sizeof(float16_t)));
      data->op_params.output_scaler = scaler_data;
      TF_LITE_ENSURE_STATUS(PopulateConvolutionQuantizationParams(
        context, input, filter, output, kTfLiteActNone,
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_min),
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_max),
        scaler_data, num_channels, SLI_MVP_ACCUMULATOR_MULTIPLIER));

    } else {
      data->supported = kTFLMrefI8;
      scratch_buffer_size = GetTensorShape(output).FlatSize() * sizeof(int32_t);
      data->per_channel_output_multiplier = static_cast<int32_t*>(context->AllocatePersistentBuffer(
                                            context, num_channels * sizeof(int32_t)));
      data->per_channel_output_shift = static_cast<int32_t*>(context->AllocatePersistentBuffer(
                                       context, num_channels * sizeof(int32_t)));

      int32_t dummy_output_multiplier;
      int dummy_output_shift;
      TF_LITE_ENSURE_STATUS(tflite::PopulateConvolutionQuantizationParams(
        context, input, filter, bias, output, kTfLiteActNone,
        &dummy_output_multiplier, &dummy_output_shift,
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_min),
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_max),
        data->per_channel_output_multiplier,
        reinterpret_cast<int*>(data->per_channel_output_shift),
        num_channels));
    }

  } else if (input->type == kTfLiteFloat32) {
    data->supported = kTFLMrefF32;
  } else {
    TF_LITE_KERNEL_LOG(context, "Type %s not currently supported.",
                       TfLiteTypeGetName(input->type));
    return kTfLiteError;
  }

  if(scratch_buffer_size > 0) {
    TF_LITE_ENSURE_STATUS(
      context->RequestScratchBufferInArena(
                 context, scratch_buffer_size, &data->scratch_buffer_index));
  } else {
    data->scratch_buffer_index = -1;
  }

  return kTfLiteOk;
}

TfLiteStatus eval_mvp_int8(TfLiteContext* context,
                           OpData* data,
                           const TfLiteEvalTensor* input,
                           const TfLiteEvalTensor* filter,
                           TfLiteEvalTensor* output)
{
  float16_t *scratch;
  if (data->scratch_buffer_index > -1) {
    scratch = reinterpret_cast<float16_t*>(context->GetScratchBuffer(context, data->scratch_buffer_index));
  } else {
    return kTfLiteError;
  }

  data->op_params.scratch_buffer = scratch;
  data->op_params.input          = tflite::micro::GetTensorData<int8_t>(input);
  data->op_params.output         = tflite::micro::GetTensorData<int8_t>(output);
  data->op_params.filter         = tflite::micro::GetTensorData<int8_t>(filter);

  TF_LITE_ENSURE_EQ(context, SL_STATUS_OK, sli_mvp_ml_transpose_conv2d_s8(&data->op_params));

  return kTfLiteOk;
}

TfLiteStatus eval_tflm_int8(TfLiteContext* context,
                            OpData* data,
                            const TfLiteEvalTensor* input,
                            const TfLiteEvalTensor* filter,
                            const TfLiteEvalTensor* bias,
                            TfLiteEvalTensor* output)
{
  int32_t *scratch;
  ConvParams op_params;

  if (data->scratch_buffer_index > -1) {
    scratch = reinterpret_cast<int32_t*>(context->GetScratchBuffer(context, data->scratch_buffer_index));
  } else {
    return kTfLiteError;
  }

  op_params.input_offset             = data->op_params.input_offset;
  op_params.output_offset            = data->op_params.output_offset;
  op_params.stride_height            = data->op_params.stride_height;
  op_params.stride_width             = data->op_params.stride_width;
  op_params.padding_values.height    = data->op_params.pad_height;
  op_params.padding_values.width     = data->op_params.pad_width;

  reference_integer_ops::TransposeConv(op_params,
                                       data->per_channel_output_multiplier,
                                       data->per_channel_output_shift,
                                       tflite::micro::GetTensorShape(input),
                                       tflite::micro::GetTensorData<int8_t>(input),
                                       tflite::micro::GetTensorShape(filter),
                                       tflite::micro::GetTensorData<int8_t>(filter),
                                       tflite::micro::GetTensorShape(bias),
                                       tflite::micro::GetTensorData<int32_t>(const_cast<TfLiteEvalTensor*>(bias)),
                                       tflite::micro::GetTensorShape(output),
                                       tflite::micro::GetTensorData<int8_t>(output),
                                       RuntimeShape(),
                                       nullptr,
                                       scratch);
  return kTfLiteOk;
}

TfLiteStatus eval_float(TfLiteConvParams* params,
                        const OpData* data,
                        const TfLiteEvalTensor* input,
                        const TfLiteEvalTensor* filter,
                        const TfLiteEvalTensor* bias,
                        TfLiteEvalTensor* output)
{
  ConvParams op_params;

  op_params.padding_type           = RuntimePaddingType(params->padding);
  op_params.padding_values.width   = data->op_params.pad_width;
  op_params.padding_values.height  = data->op_params.pad_height;
  op_params.stride_width           = data->op_params.stride_width;
  op_params.stride_height          = data->op_params.stride_height;

  reference_ops::TransposeConv(op_params,
                               tflite::micro::GetTensorShape(input),
                               tflite::micro::GetTensorData<float>(input),
                               tflite::micro::GetTensorShape(filter),
                               tflite::micro::GetTensorData<float>(filter),
                               tflite::micro::GetTensorShape(bias),
                               tflite::micro::GetTensorData<float>(const_cast<TfLiteEvalTensor*>(bias)),
                               tflite::micro::GetTensorShape(output),
                               tflite::micro::GetTensorData<float>(output),
                               RuntimeShape(),
                               nullptr);
  return kTfLiteOk;
}

TfLiteStatus Invoke(TfLiteContext* context, TfLiteNode* node)
{
  TfLiteStatus status = kTfLiteError;

  TFLITE_DCHECK(node->user_data    != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  auto* params = reinterpret_cast<TfLiteConvParams*>(node->builtin_data);
  OpData* data = static_cast<OpData*>(node->user_data);

  const auto input  = tflite::micro::GetEvalInput(context, node, kInputTensor);
  const auto filter = tflite::micro::GetEvalInput(context, node, kFilterTensor);
  const auto bias   = NumInputs(node) == 4
                      ? tflite::micro::GetEvalInput(context, node, kBiasTensor)
                      : nullptr;
  auto output       = tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  if (data->supported == kMvp) {
    status = eval_mvp_int8(context, data, input, filter, output);

  } else if (data->supported == kTFLMrefI8) {
    status = eval_tflm_int8(context, data, input, filter, bias, output);

  } else if (data->supported == kTFLMrefF32) {
    status = eval_float(params, data, input, filter, bias, output);
  }

  return status;
}

}  // namespace transpose_conv2d
}  // namespace sl

TfLiteRegistration Register_TRANSPOSE_CONV() {
  return {/*init=*/sl::transpose_conv2d::Init,
          /*free=*/nullptr,
          /*prepare=*/sl::transpose_conv2d::Prepare,
          /*invoke=*/sl::transpose_conv2d::Invoke,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#else
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

#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/transpose_conv.h"

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/transpose_conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

// For the TfLite transpose_conv implementation, input tensor 0 corresponds to
// the OutputShapeTensor. However, since TFLM does not support dynamic tensors,
// the TFLM implementation ignores input tensor 0 and the only inputs we care
// about are kFilterTensor, kInputTensor and kBiasTensor.
constexpr int kFilterTensor = 1;
constexpr int kInputTensor = 2;
constexpr int kBiasTensor = 3;
constexpr int kOutputTensor = 0;

// Conv is quantized along dimension 0:
// https://www.tensorflow.org/lite/performance/quantization_spec
constexpr int kConvQuantizedDimension = 0;

struct OpData {
  ConvParams params;

  // A scratch buffer is required for quantized implementations.
  int scratch_buffer_index;

  // Multiplier and shift arrays are required for the int8 implementation.
  int32_t* per_channel_output_multiplier;
  int32_t* per_channel_output_shift;
};

inline PaddingType RuntimePaddingType(TfLitePadding padding) {
  switch (padding) {
    case TfLitePadding::kTfLitePaddingSame:
      return PaddingType::kSame;
    case TfLitePadding::kTfLitePaddingValid:
      return PaddingType::kValid;
    case TfLitePadding::kTfLitePaddingUnknown:
    default:
      return PaddingType::kNone;
  }
}

TfLiteStatus CalculateOpData(TfLiteContext* context, TfLiteNode* node,
                             const TfLiteConvParams* params, int width,
                             int height, int filter_width, int filter_height,
                             int out_width, int out_height,
                             const TfLiteType data_type, OpData* data) {
  bool has_bias = node->inputs->size == 4;
  // Check number of inputs/outputs
  TF_LITE_ENSURE(context, has_bias || node->inputs->size == 3);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);

  // Matching GetWindowedOutputSize in TensorFlow.
  auto padding = params->padding;
  TfLitePaddingValues padding_values = ComputePaddingHeightWidth(
      params->stride_height, params->stride_width,
      params->dilation_height_factor, params->dilation_width_factor, height,
      width, filter_height, filter_width, padding, &out_height, &out_width);

  data->params.padding_type = RuntimePaddingType(padding);
  data->params.padding_values.width = padding_values.width;
  data->params.padding_values.height = padding_values.height;

  // Note that quantized inference requires that all tensors have their
  // parameters set. This is usually done during quantized training.
  if (data_type != kTfLiteFloat32) {
    const TfLiteTensor* input = GetInput(context, node, kInputTensor);
    TF_LITE_ENSURE(context, input != nullptr);
    const TfLiteTensor* filter = GetInput(context, node, kFilterTensor);
    TF_LITE_ENSURE(context, filter != nullptr);
    const TfLiteTensor* bias =
        GetOptionalInputTensor(context, node, kBiasTensor);
    TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
    TF_LITE_ENSURE(context, output != nullptr);
    int output_channels = filter->dims->data[kConvQuantizedDimension];

    TF_LITE_ENSURE_STATUS(tflite::PopulateConvolutionQuantizationParams(
        context, input, filter, bias, output, params->activation,
        &data->params.output_multiplier, &data->params.output_shift,
        &data->params.quantized_activation_min,
        &data->params.quantized_activation_max,
        data->per_channel_output_multiplier,
        reinterpret_cast<int*>(data->per_channel_output_shift),
        output_channels));
  }
  return kTfLiteOk;
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  const auto params = static_cast<const TfLiteConvParams*>(node->builtin_data);

  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter = GetInput(context, node, kFilterTensor);
  TF_LITE_ENSURE(context, filter != nullptr);

  int input_width = input->dims->data[2];
  int input_height = input->dims->data[1];
  int filter_width = filter->dims->data[2];
  int filter_height = filter->dims->data[1];
  int output_width = output->dims->data[2];
  int output_height = output->dims->data[1];

  // Dynamically allocate per-channel quantization parameters.
  const int num_channels = filter->dims->data[kConvQuantizedDimension];
  data->per_channel_output_multiplier =
      static_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));
  data->per_channel_output_shift =
      static_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));

  // Quantized kernels use an int32 scratch buffer.
  if (input->type == kTfLiteUInt8 || input->type == kTfLiteInt8) {
    TFLITE_DCHECK(context->RequestScratchBufferInArena != nullptr);
    TFLITE_DCHECK(context->RequestScratchBufferInArena(
                      context,
                      GetTensorShape(output).FlatSize() * sizeof(int32_t),
                      &(data->scratch_buffer_index)) == kTfLiteOk);
  }

  // All per-channel quantized tensors need valid zero point and scale arrays.
  if (input->type == kTfLiteInt8) {
    TF_LITE_ENSURE_EQ(context, filter->quantization.type,
                      kTfLiteAffineQuantization);

    const auto* affine_quantization =
        static_cast<TfLiteAffineQuantization*>(filter->quantization.params);
    TF_LITE_ENSURE(context, affine_quantization);
    TF_LITE_ENSURE(context, affine_quantization->scale);
    TF_LITE_ENSURE(context, affine_quantization->zero_point);

    TF_LITE_ENSURE(context,
                   affine_quantization->scale->size == 1 ||
                       affine_quantization->scale->size ==
                           filter->dims->data[kConvQuantizedDimension]);
    TF_LITE_ENSURE_EQ(context, affine_quantization->scale->size,
                      affine_quantization->zero_point->size);
  }

  TF_LITE_ENSURE_STATUS(CalculateOpData(
      context, node, params, input_width, input_height, filter_width,
      filter_height, output_width, output_height, input->type, data));

  // Offsets (zero points)
  data->params.input_offset = -input->params.zero_point;
  data->params.weights_offset = -filter->params.zero_point;
  data->params.output_offset = output->params.zero_point;

  // Stride + dilation
  data->params.stride_width = params->stride_width;
  data->params.stride_height = params->stride_height;
  data->params.dilation_width_factor = params->dilation_width_factor;
  data->params.dilation_height_factor = params->dilation_height_factor;

  float output_activation_min, output_activation_max;
  CalculateActivationRange(params->activation, &output_activation_min,
                           &output_activation_max);
  data->params.float_activation_min = output_activation_min;
  data->params.float_activation_max = output_activation_max;
  return kTfLiteOk;
}  // namespace conv

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFilterTensor);
  const TfLiteEvalTensor* bias =
      (NumInputs(node) == 4)
          ? tflite::micro::GetEvalInput(context, node, kBiasTensor)
          : nullptr;
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  TF_LITE_ENSURE_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32: {
      reference_ops::TransposeConv(
          data.params, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<float>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<float>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output),
          tflite::micro::GetTensorShape(nullptr), nullptr);
      break;
    }
    case kTfLiteInt8: {
      int32_t* scratch_buffer = static_cast<int32_t*>(
          context->GetScratchBuffer(context, data.scratch_buffer_index));
      reference_integer_ops::TransposeConv(
          data.params, data.per_channel_output_multiplier,
          data.per_channel_output_shift, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<int8_t>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<int32_t>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int8_t>(output),
          tflite::micro::GetTensorShape(nullptr), nullptr, scratch_buffer);
      break;
    }
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_TRANSPOSE_CONV() {
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

#endif
