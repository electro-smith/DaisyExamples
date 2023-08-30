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

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/conv.h"

#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nn_types.h"
#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nnfunctions.h"
#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

struct OpData {
  OpDataConv reference_op_data;

  // Index to buffer for optimizations if applicable.
  int buffer_idx;
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  int32_t buf_size = 0;
  const auto& params =
      *(static_cast<const TfLiteConvParams*>(node->builtin_data));
  OpData* data = static_cast<OpData*>(node->user_data);

  const TfLiteTensor* input = GetInput(context, node, kConvInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter = GetInput(context, node, kConvWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);
  const TfLiteTensor* output = GetOutput(context, node, kConvOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  RuntimeShape input_shape = GetTensorShape(input);
  RuntimeShape output_shape = GetTensorShape(output);

  // Initialize cmsis_nn input dimensions
  cmsis_nn_dims input_dims;
  input_dims.n = MatchingDim(input_shape, 0, output_shape, 0);
  input_dims.h = input->dims->data[1];
  input_dims.w = input->dims->data[2];
  input_dims.c = input_shape.Dims(3);

  // Initialize cmsis_nn filter dimensions
  cmsis_nn_dims filter_dims;
  filter_dims.n = output_shape.Dims(3);
  filter_dims.h = filter->dims->data[1];
  filter_dims.w = filter->dims->data[2];
  filter_dims.c = input_dims.c;

  // Initialize cmsis_nn output dimensions
  cmsis_nn_dims output_dims;
  output_dims.n = input_dims.n;
  output_dims.h = output->dims->data[1];
  output_dims.w = output->dims->data[2];
  output_dims.c = output_shape.Dims(3);

  // Dynamically allocate per-channel quantization parameters.
  // TODO(#42883): This allocation is done even for non-int8 cases to get around
  // a bug in kernel_util.cc which incorrectly uses per_channel_output_shift in
  // non-int8 cases. Protect this section with a if (input->type == kTfLiteInt8)
  // when the issue is fixed.
  const int num_channels = filter->dims->data[kConvQuantizedDimension];
  data->reference_op_data.per_channel_output_multiplier =
      static_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));
  data->reference_op_data.per_channel_output_shift =
      static_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));

  TF_LITE_ENSURE_STATUS(CalculateOpDataConv(
      context, node, params, input_dims.w, input_dims.h, filter_dims.w,
      filter_dims.h, output_dims.w, output_dims.h, input->type,
      &data->reference_op_data));

  if (input->type == kTfLiteInt8) {
    // Initialize cmsis_nn convolution parameters
    cmsis_nn_conv_params conv_params;
    conv_params.input_offset = -input->params.zero_point;
    conv_params.output_offset = output->params.zero_point;
    conv_params.stride.h = params.stride_height;
    conv_params.stride.w = params.stride_width;
    conv_params.dilation.h = params.dilation_height_factor;
    conv_params.dilation.w = params.dilation_width_factor;
    conv_params.padding.h = data->reference_op_data.padding.height;
    conv_params.padding.w = data->reference_op_data.padding.width;
    conv_params.activation.min = data->reference_op_data.output_activation_min;
    conv_params.activation.max = data->reference_op_data.output_activation_max;

    buf_size = arm_convolve_wrapper_s8_get_buffer_size(
        &conv_params, &input_dims, &filter_dims, &output_dims);
  }

  if (buf_size > 0) {
    TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
        context, buf_size, &data->buffer_idx));
  } else {
    data->buffer_idx = -1;
  }
  return kTfLiteOk;
}

TfLiteStatus EvalQuantizedPerChannel(
    TfLiteContext* context, TfLiteNode* node, const TfLiteConvParams& params,
    const OpData& data, const TfLiteEvalTensor* input,
    const TfLiteEvalTensor* filter, const TfLiteEvalTensor* bias,
    TfLiteEvalTensor* output, TfLiteEvalTensor* im2col) {
  cmsis_nn_conv_params conv_params;
  conv_params.dilation.h = params.dilation_height_factor;
  conv_params.dilation.w = params.dilation_width_factor;
  // TODO(#43557) Remove checks for dilation and call to reference
  // implementation when dilation is supported in the optimized implementation
  // by CMSIS-NN.
  if (conv_params.dilation.h == 1 && conv_params.dilation.w == 1) {
    // Initialize cmsis_nn convolution parameters
    conv_params.input_offset = -data.reference_op_data.input_zero_point;
    conv_params.output_offset = data.reference_op_data.output_zero_point;
    conv_params.stride.h = params.stride_height;
    conv_params.stride.w = params.stride_width;
    conv_params.padding.h = data.reference_op_data.padding.height;
    conv_params.padding.w = data.reference_op_data.padding.width;
    conv_params.activation.min = data.reference_op_data.output_activation_min;
    conv_params.activation.max = data.reference_op_data.output_activation_max;

    // Initialize cmsis_nn per channel quantization parameters
    cmsis_nn_per_channel_quant_params quant_params;
    quant_params.multiplier = const_cast<int32_t*>(
        data.reference_op_data.per_channel_output_multiplier);
    quant_params.shift =
        const_cast<int32_t*>(data.reference_op_data.per_channel_output_shift);

    RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
    RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
    RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
    RuntimeShape bias_shape = tflite::micro::GetTensorShape(bias);

    // Consistency check.
    TFLITE_DCHECK_LE(conv_params.activation.min, conv_params.activation.max);
    TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
    TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);
    const int batch_size = MatchingDim(input_shape, 0, output_shape, 0);
    const int input_depth = MatchingDim(input_shape, 3, filter_shape, 3);
    const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);
    if (tflite::micro::GetTensorData<int8_t>(bias)) {
      TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
    }

    // Initialize cmsis_nn dimensions
    // Input
    cmsis_nn_dims input_dims;
    input_dims.n = batch_size;
    input_dims.h = input_shape.Dims(1);
    input_dims.w = input_shape.Dims(2);
    input_dims.c = input_depth;

    // Filter
    cmsis_nn_dims filter_dims;
    filter_dims.n = output_depth;
    filter_dims.h = filter_shape.Dims(1);
    filter_dims.w = filter_shape.Dims(2);
    filter_dims.c = input_depth;

    // Bias
    cmsis_nn_dims bias_dims;
    bias_dims.n = 1;
    bias_dims.h = 1;
    bias_dims.w = 1;
    bias_dims.c = output_depth;

    // Output
    cmsis_nn_dims output_dims;
    output_dims.n = batch_size;
    output_dims.h = output_shape.Dims(1);
    output_dims.w = output_shape.Dims(2);
    output_dims.c = output_depth;

    // Initialize cmsis_nn context
    cmsis_nn_context ctx;
    ctx.buf = nullptr;
    ctx.size = 0;

    if (data.buffer_idx > -1) {
      ctx.buf = context->GetScratchBuffer(context, data.buffer_idx);
      // Note: ctx.size is currently not used in cmsis_nn.
      // The buffer should be allocated in the Prepare function through
      // arm_convolve_wrapper_s8_get_buffer_size
    }

    // arm_convolve_wrapper_s8 dispatches the optimized kernel accordingly with
    // the parameters passed
    TFLITE_DCHECK_EQ(
        arm_convolve_wrapper_s8(
            &ctx, &conv_params, &quant_params, &input_dims,
            tflite::micro::GetTensorData<int8_t>(input), &filter_dims,
            tflite::micro::GetTensorData<int8_t>(filter), &bias_dims,
            tflite::micro::GetTensorData<int32_t>(bias), &output_dims,
            tflite::micro::GetTensorData<int8_t>(output)),
        ARM_MATH_SUCCESS);
  } else {
    reference_integer_ops::ConvPerChannel(
        ConvParamsQuantized(params, data.reference_op_data),
        data.reference_op_data.per_channel_output_multiplier,
        data.reference_op_data.per_channel_output_shift,
        tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int8_t>(input),
        tflite::micro::GetTensorShape(filter),
        tflite::micro::GetTensorData<int8_t>(filter),
        tflite::micro::GetTensorShape(bias),
        tflite::micro::GetTensorData<int32_t>(bias),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int8_t>(output));
  }
  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kConvInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
  const TfLiteEvalTensor* bias =
      (NumInputs(node) == 3)
          ? tflite::micro::GetEvalInput(context, node, kConvBiasTensor)
          : nullptr;
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kConvOutputTensor);

  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto& params =
      *(reinterpret_cast<TfLiteConvParams*>(node->builtin_data));
  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  TF_LITE_ENSURE_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_CONV_2D_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::Conv(
          ConvParamsFloat(params, data.reference_op_data),
          tflite::micro::GetTensorShape(input),
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
    case kTfLiteInt8:
      #if EI_TFLITE_DISABLE_CONV_2D_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      return EvalQuantizedPerChannel(context, node, params, data, input, filter,
                                     bias, output, nullptr);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_CONV_2D() {
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

#elif EI_CLASSIFIER_TFLITE_ENABLE_ARC == 1
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

#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/conv.h"

#include "mli_api.h"  // NOLINT
#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/mli_slicers.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/mli_tf_utils.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/scratch_buf_mgr.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/scratch_buffers.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

constexpr int kInputTensor = 0;
constexpr int kFilterTensor = 1;
constexpr int kBiasTensor = 2;
constexpr int kOutputTensor = 0;

// Conv is quantized along dimension 0:
// https://www.tensorflow.org/lite/performance/quantization_spec
constexpr int kConvQuantizedDimension = 0;

// This file has 2 implementation of Conv.

struct OpData {
  TfLitePaddingValues padding;

  // Cached tensor zero point values for quantized operations.
  int32_t input_zero_point;
  int32_t filter_zero_point;
  int32_t output_zero_point;

  // The scaling factor from input to output (aka the 'real multiplier') can
  // be represented as a fixed point multiplier plus a left shift.
  int32_t output_multiplier;
  int output_shift;

  // Per channel output multiplier and shift.
  int32_t* per_channel_output_multiplier;
  int32_t* per_channel_output_shift;

  // The range of the fused activation layer. For example for kNone and
  // uint8_t these would be 0 and 255.
  int32_t output_activation_min;
  int32_t output_activation_max;

  // The result of checking if MLI optimized version of tensors can be used.
  bool is_mli_applicable;

  // Tensors in MLI format.
  mli_tensor* mli_in;
  mli_tensor* mli_weights;
  mli_tensor* mli_bias;
  mli_tensor* mli_out;
  mli_conv2d_cfg* cfg;
};

#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
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
#endif

bool IsMliApplicable(TfLiteContext* context, const TfLiteTensor* input,
                     const TfLiteTensor* filter, const TfLiteTensor* bias,
                     const TfLiteConvParams* params) {
  const auto* affine_quantization =
      reinterpret_cast<TfLiteAffineQuantization*>(filter->quantization.params);
  // MLI optimized version only supports int8_t datatype, dilation factor of 1
  // and per-axis quantization of weights (no broadcasting/per-tensor)
  bool ret_val = (filter->type == kTfLiteInt8) &&
                 (input->type == kTfLiteInt8) && (bias->type == kTfLiteInt32) &&
                 (params->dilation_width_factor == 1) &&
                 (params->dilation_height_factor == 1) &&
                 (affine_quantization->scale->size ==
                  filter->dims->data[kConvQuantizedDimension]);
  return ret_val;
}

TfLiteStatus CalculateOpData(TfLiteContext* context, TfLiteNode* node,
                             const TfLiteConvParams* params, int width,
                             int height, int filter_width, int filter_height,
                             int out_width, int out_height,
                             const TfLiteType data_type, OpData* data) {
  bool has_bias = node->inputs->size == 3;
  // Check number of inputs/outputs
  TF_LITE_ENSURE(context, has_bias || node->inputs->size == 2);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, 1);

  // Matching GetWindowedOutputSize in TensorFlow.
  auto padding = params->padding;
  data->padding = ComputePaddingHeightWidth(
      params->stride_height, params->stride_width,
      params->dilation_height_factor, params->dilation_width_factor, height,
      width, filter_height, filter_width, padding, &out_height, &out_width);
  // Note that quantized inference requires that all tensors have their
  // parameters set. This is usually done during quantized training.
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* filter = GetInput(context, node, kFilterTensor);
  const TfLiteTensor* bias = GetOptionalInputTensor(context, node, kBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  if (data_type != kTfLiteFloat32 && !data->is_mli_applicable) {
    int output_channels = filter->dims->data[kConvQuantizedDimension];

    TF_LITE_ENSURE_STATUS(tflite::PopulateConvolutionQuantizationParams(
        context, input, filter, bias, output, params->activation,
        &data->output_multiplier, &data->output_shift,
        &data->output_activation_min, &data->output_activation_max,
        data->per_channel_output_multiplier,
        reinterpret_cast<int*>(data->per_channel_output_shift),
        output_channels));
  }
#endif
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
  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* filter = GetInput(context, node, kFilterTensor);
  const TfLiteTensor* bias = GetOptionalInputTensor(context, node, kBiasTensor);

  int input_width = input->dims->data[2];
  int input_height = input->dims->data[1];
  int filter_width = filter->dims->data[2];
  int filter_height = filter->dims->data[1];
  int output_width = output->dims->data[2];
  int output_height = output->dims->data[1];

  // Dynamically allocate per-channel quantization parameters.
  const int num_channels = filter->dims->data[kConvQuantizedDimension];
  data->per_channel_output_multiplier =
      reinterpret_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));
  data->per_channel_output_shift =
      reinterpret_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));

  data->is_mli_applicable =
      IsMliApplicable(context, input, filter, bias, params);

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

  data->input_zero_point = input->params.zero_point;
  data->filter_zero_point = filter->params.zero_point;
  data->output_zero_point = output->params.zero_point;

  if (data->is_mli_applicable) {
    data->mli_in = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->mli_weights = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->mli_bias = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->mli_out = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->cfg = static_cast<mli_conv2d_cfg*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_conv2d_cfg)));

    // reuse space allocated for OpData parameters
    data->mli_weights->el_params.asym.scale.pi32 =
        static_cast<int32_t*>(data->per_channel_output_multiplier);
    data->mli_bias->el_params.asym.scale.pi32 =
        static_cast<int32_t*>(data->per_channel_output_shift);

    data->mli_weights->el_params.asym.zero_point.pi16 =
        reinterpret_cast<int16_t*>(&data->filter_zero_point);
    data->mli_bias->el_params.asym.zero_point.pi16 =
        reinterpret_cast<int16_t*>(&data->filter_zero_point) + sizeof(int16_t);

    ops::micro::ConvertToMliTensor(input, data->mli_in);
    ops::micro::ConvertToMliTensorPerChannel(filter, data->mli_weights);
    ops::micro::ConvertToMliTensorPerChannel(bias, data->mli_bias);
    ops::micro::ConvertToMliTensor(output, data->mli_out);

    if (params->activation == kTfLiteActRelu) {
      data->cfg->relu.type = MLI_RELU_GEN;
    } else if (params->activation == kTfLiteActRelu6) {
      data->cfg->relu.type = MLI_RELU_6;
    } else if (params->activation == kTfLiteActReluN1To1) {
      data->cfg->relu.type = MLI_RELU_1;
    } else {
      data->cfg->relu.type = MLI_RELU_NONE;
    }
    data->cfg->stride_width = params->stride_width;
    data->cfg->stride_height = params->stride_height;
    if (params->padding == kTfLitePaddingValid) {
      data->cfg->padding_left = 0;
      data->cfg->padding_right = 0;
      data->cfg->padding_top = 0;
      data->cfg->padding_bottom = 0;
    } else {
      data->cfg->padding_left = data->padding.width;
      data->cfg->padding_right =
          data->padding.width + data->padding.width_offset;
      data->cfg->padding_top = data->padding.height;
      data->cfg->padding_bottom =
          data->padding.height + data->padding.height_offset;
    }
  }
  return kTfLiteOk;
}

void EvalQuantized(TfLiteContext* context, TfLiteNode* node,
                   TfLiteConvParams* params, const OpData& data,
                   const TfLiteEvalTensor* input,
                   const TfLiteEvalTensor* filter, const TfLiteEvalTensor* bias,
                   TfLiteEvalTensor* im2col, TfLiteEvalTensor* hwcn_weights,
                   TfLiteEvalTensor* output) {
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  const int32_t input_offset = -data.input_zero_point;
  const int32_t filter_offset = -data.filter_zero_point;
  const int32_t output_offset = data.output_zero_point;

  ConvParams op_params;
  op_params.padding_type = RuntimePaddingType(params->padding);
  op_params.padding_values.width = data.padding.width;
  op_params.padding_values.height = data.padding.height;
  op_params.stride_width = params->stride_width;
  op_params.stride_height = params->stride_height;
  op_params.dilation_width_factor = params->dilation_width_factor;
  op_params.dilation_height_factor = params->dilation_height_factor;
  op_params.input_offset = input_offset;
  op_params.weights_offset = filter_offset;
  op_params.output_offset = output_offset;
  op_params.output_multiplier = data.output_multiplier;
  op_params.output_shift = -data.output_shift;
  op_params.quantized_activation_min = data.output_activation_min;
  op_params.quantized_activation_max = data.output_activation_max;
  reference_ops::Conv(op_params, tflite::micro::GetTensorShape(input),
                      tflite::micro::GetTensorData<uint8_t>(input),
                      tflite::micro::GetTensorShape(filter),
                      tflite::micro::GetTensorData<uint8_t>(filter),
                      tflite::micro::GetTensorShape(bias),
                      tflite::micro::GetTensorData<int32_t>(bias),
                      tflite::micro::GetTensorShape(output),
                      tflite::micro::GetTensorData<uint8_t>(output),
                      tflite::micro::GetTensorShape(im2col),
                      tflite::micro::GetTensorData<uint8_t>(im2col), nullptr);
#else
  TF_LITE_KERNEL_LOG(context,
                     "Type %s (%d) is not supported by ARC MLI Library.",
                     TfLiteTypeGetName(input->type), input->type);
#endif
}

TfLiteStatus EvalMliQuantizedPerChannel(
    TfLiteContext* context, TfLiteNode* node, TfLiteConvParams* params,
    const OpData& data, const TfLiteEvalTensor* input,
    const TfLiteEvalTensor* filter, const TfLiteEvalTensor* bias,
    TfLiteEvalTensor* output) {
  // Run Conv MLI kernel
  // MLI optimized version only supports int8_t dataype and dilation factor of 1
  if (data.is_mli_applicable) {
    // Copy configuration data from external to local memory
    mli_conv2d_cfg cfg_local = *data.cfg;

    ops::micro::MliTensorAttachBuffer<int8_t>(input, data.mli_in);
    ops::micro::MliTensorAttachBuffer<int8_t>(filter, data.mli_weights);
    ops::micro::MliTensorAttachBuffer<int32_t>(bias, data.mli_bias);
    ops::micro::MliTensorAttachBuffer<int8_t>(output, data.mli_out);

    // for height slicing
    const int height_dimension = 1;
    int in_slice_height = 0;
    int out_slice_height = 0;
    const int kernel_height =
        static_cast<int>(data.mli_weights->shape[KRNL_H_DIM_HWC]);
    const int overlap = kernel_height - cfg_local.stride_height;

    // for weight slicing (on output channels)
    // NHWC layout for weights, output channel dimension is the first dimension.
    const int weight_out_ch_dimension = 0;
    int slice_channels =
        static_cast<int>(data.mli_weights->shape[weight_out_ch_dimension]);
    // Batch-Height-Width-Channel layout means last dimension is output
    // channels.
    const int out_tensor_ch_dimension = 3;

    // Tensors for data in fast (local) memory and config to copy data from
    // external to local memory
    mli_tensor weights_local = *data.mli_weights;
    mli_tensor bias_local = *data.mli_bias;
    mli_tensor in_local = *data.mli_in;
    mli_tensor out_local = *data.mli_out;
    mli_mov_cfg_t copy_config;
    mli_mov_cfg_for_copy(&copy_config);
    TF_LITE_ENSURE_STATUS(ops::micro::get_arc_scratch_buffer_for_conv_tensors(
        context, &in_local, &weights_local, &bias_local, &out_local));
    TF_LITE_ENSURE_STATUS(ops::micro::arc_scratch_buffer_calc_slice_size_io(
        &in_local, &out_local, kernel_height, cfg_local.stride_height,
        cfg_local.padding_top, cfg_local.padding_bottom, &in_slice_height,
        &out_slice_height));
    TF_LITE_ENSURE_STATUS(
        ops::micro::arc_scratch_buffer_calc_slice_size_weights(
            &weights_local, &bias_local, weight_out_ch_dimension,
            &slice_channels));

    /* is_local indicates that the tensor is already in local memory,
       so in that case the original tensor can be used,
       and there is no need to copy it to the local tensor*/
    const bool in_is_local = in_local.data == data.mli_in->data;
    const bool out_is_local = out_local.data == data.mli_out->data;
    const bool w_is_local = weights_local.data == data.mli_weights->data;
    const bool b_is_local = bias_local.data == data.mli_bias->data;

    ops::micro::TensorSlicer w_slice(data.mli_weights, weight_out_ch_dimension,
                                     slice_channels);
    ops::micro::TensorSlicer b_slice(data.mli_bias, weight_out_ch_dimension,
                                     slice_channels);
    ops::micro::TensorSlicer out_ch_slice(data.mli_out, out_tensor_ch_dimension,
                                          slice_channels, 0, 0, 0, true);

    mli_tensor* w_ptr = w_is_local ? w_slice.Sub() : &weights_local;
    mli_tensor* b_ptr = b_is_local ? b_slice.Sub() : &bias_local;

    void* input_buffer_ptr = NULL;
    uint32_t input_buffer_size = 0;

    while (!w_slice.Done()) {
      mli_mov_tensor_sync(w_slice.Sub(), &copy_config, w_ptr);
      mli_mov_tensor_sync(b_slice.Sub(), &copy_config, b_ptr);

      /* mli_in tensor contains batches of HWC tensors. so it is a 4 dimensional
      tensor. because the mli kernel will process one HWC tensor at a time, the
      4 dimensional tensor needs to be sliced into nBatch 3 dimensional tensors.
      on top of that there could be a need to also slice in the Height
      dimension. for that the sliceHeight has been calculated. The tensor slicer
      is configured that it will completely slice the nBatch dimension (0) and
      slice the height dimension (1) in chunks of 'sliceHeight' */
      ops::micro::TensorSlicer in_slice(data.mli_in, height_dimension,
                                        in_slice_height, cfg_local.padding_top,
                                        cfg_local.padding_bottom, overlap);

      /* output tensor is already sliced in the output channel dimension.
      out_ch_slice.Sub() is the tensor for the amount of output channels of this
      iteration of the weight slice loop. This tensor needs to be further
      sliced over the batch and height dimension. */
      ops::micro::TensorSlicer out_slice(out_ch_slice.Sub(), height_dimension,
                                         out_slice_height);

      /* setup the pointers to the local or remote tensor to make the code
       * inside the loop easier. */
      mli_tensor* in_ptr = in_is_local ? in_slice.Sub() : &in_local;
      mli_tensor* out_ptr = out_is_local ? out_slice.Sub() : &out_local;

      while (!out_slice.Done()) {
        TF_LITE_ENSURE(context, !in_slice.Done());
        cfg_local.padding_top = in_slice.GetPaddingPre();
        cfg_local.padding_bottom = in_slice.GetPaddingPost();

        // if same input copy as previous iteration, skip the copy of input
        if ((in_slice.Sub()->data != input_buffer_ptr) ||
            (mli_hlp_count_elem_num(in_slice.Sub(), 0) != input_buffer_size)) {
          mli_mov_tensor_sync(in_slice.Sub(), &copy_config, in_ptr);
          input_buffer_ptr = in_slice.Sub()->data;
          input_buffer_size = mli_hlp_count_elem_num(in_slice.Sub(), 0);
        }
        mli_krn_conv2d_nhwc_sa8_sa8_sa32(in_ptr, w_ptr, b_ptr, &cfg_local,
                                         out_ptr);
        mli_mov_tensor_sync(out_ptr, &copy_config, out_slice.Sub());

        in_slice.Next();
        out_slice.Next();
      }
      w_slice.Next();
      b_slice.Next();
      out_ch_slice.Next();
      TF_LITE_ENSURE(context, in_slice.Done());
    }
  }
  return kTfLiteOk;
}

void EvalQuantizedPerChannel(TfLiteContext* context, TfLiteNode* node,
                             TfLiteConvParams* params, const OpData& data,
                             const TfLiteEvalTensor* input,
                             const TfLiteEvalTensor* filter,
                             const TfLiteEvalTensor* bias,
                             TfLiteEvalTensor* output,
                             TfLiteEvalTensor* im2col) {
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  ConvParams op_params;
  op_params.input_offset = -data.input_zero_point;
  op_params.output_offset = data.output_zero_point;
  op_params.stride_height = params->stride_height;
  op_params.stride_width = params->stride_width;
  op_params.dilation_height_factor = params->dilation_height_factor;
  op_params.dilation_width_factor = params->dilation_width_factor;
  op_params.padding_values.height = data.padding.height;
  op_params.padding_values.width = data.padding.width;
  op_params.quantized_activation_min = data.output_activation_min;
  op_params.quantized_activation_max = data.output_activation_max;

  reference_integer_ops::ConvPerChannel(
      op_params, data.per_channel_output_multiplier,
      data.per_channel_output_shift, tflite::micro::GetTensorShape(input),
      tflite::micro::GetTensorData<int8_t>(input),
      tflite::micro::GetTensorShape(filter),
      tflite::micro::GetTensorData<int8_t>(filter),
      tflite::micro::GetTensorShape(bias),
      tflite::micro::GetTensorData<int32_t>(bias),
      tflite::micro::GetTensorShape(output),
      tflite::micro::GetTensorData<int8_t>(output));
#else
  TF_LITE_KERNEL_LOG(context,
                     "Node configuration is not supported by ARC MLI Library.");
#endif
}

void EvalFloat(TfLiteContext* context, TfLiteNode* node,
               TfLiteConvParams* params, const OpData& data,
               const TfLiteEvalTensor* input, const TfLiteEvalTensor* filter,
               const TfLiteEvalTensor* bias, TfLiteEvalTensor* im2col,
               TfLiteEvalTensor* hwcn_weights, TfLiteEvalTensor* output) {
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  float output_activation_min, output_activation_max;
  CalculateActivationRange(params->activation, &output_activation_min,
                           &output_activation_max);
  ConvParams op_params;
  op_params.padding_type = RuntimePaddingType(params->padding);
  op_params.padding_values.width = data.padding.width;
  op_params.padding_values.height = data.padding.height;
  op_params.stride_width = params->stride_width;
  op_params.stride_height = params->stride_height;
  op_params.dilation_width_factor = params->dilation_width_factor;
  op_params.dilation_height_factor = params->dilation_height_factor;
  op_params.float_activation_min = output_activation_min;
  op_params.float_activation_max = output_activation_max;

  reference_ops::Conv(op_params, tflite::micro::GetTensorShape(input),
                      tflite::micro::GetTensorData<float>(input),
                      tflite::micro::GetTensorShape(filter),
                      tflite::micro::GetTensorData<float>(filter),
                      tflite::micro::GetTensorShape(bias),
                      tflite::micro::GetTensorData<float>(bias),
                      tflite::micro::GetTensorShape(output),
                      tflite::micro::GetTensorData<float>(output),
                      tflite::micro::GetTensorShape(im2col),
                      tflite::micro::GetTensorData<float>(im2col));
#else
  TF_LITE_KERNEL_LOG(context,
                     "Type %s (%d) is not supported by ARC MLI Library.",
                     TfLiteTypeGetName(input->type), input->type);
#endif
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteConvParams*>(node->builtin_data);

  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFilterTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kBiasTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32:
      #if EI_TFLITE_DISABLE_CONV_2D_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      EvalFloat(context, node, params, data, input, filter, bias, nullptr,
                nullptr, output);
      break;
    case kTfLiteInt8:
      #if EI_TFLITE_DISABLE_CONV_2D_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      if (data.is_mli_applicable) {
        EvalMliQuantizedPerChannel(context, node, params, data, input, filter,
                                   bias, output);
      } else {
        EvalQuantizedPerChannel(context, node, params, data, input, filter,
                                bias, output, nullptr);
      }
      break;
    case kTfLiteUInt8:
      #if EI_TFLITE_DISABLE_CONV_2D_IN_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      EvalQuantized(context, node, params, data, input, filter, bias, nullptr,
                    nullptr, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_CONV_2D() {
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

#elif EI_CLASSIFIER_TFLITE_ENABLE_SILABS_MVP == 1

#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nnfunctions.h"

#include "sl_mvp_ml_conv2d.h"

namespace tflite {
namespace sl {
namespace conv2d {

constexpr int kInputTensor  = 0;
constexpr int kFilterTensor = 1;
constexpr int kBiasTensor   = 2;
constexpr int kOutputTensor = 0;

// Conv is quantized along dimension 0 of filter tensor.
// https://www.tensorflow.org/lite/performance/quantization_spec
constexpr int kConvQuantizedDimension = 0;

enum op_support { kMvp, kCmsisNN, kTFLMrefF32, kTFLMrefI8 };

struct OpData {
  op_support  supported;
  float       activation_min_f32;
  float       activation_max_f32;
  int         scratch_buffer_index;
  sli_mvp_ml_conv2d_s8_params_t op_params;

  // CMSIS-NN per channel output multiplier and shift.
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
    // quantization dimension (channels_out).
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
  const auto params = static_cast<const TfLiteConvParams*>(node->builtin_data);

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
  data->op_params.dilation_height = params->dilation_height_factor;
  data->op_params.dilation_width  = params->dilation_width_factor;
  data->op_params.padding         = params->padding == kTfLitePaddingSame;

  int dummy_height, dummy_width;
  const auto padding = ComputePaddingHeightWidth(
                         params->stride_height, params->stride_width,
                         params->dilation_height_factor, params->dilation_width_factor,
                         data->op_params.input_height, data->op_params.input_width,
                         data->op_params.filter_height, data->op_params.filter_width,
                         params->padding,
                         &dummy_height, &dummy_width);

  data->op_params.pad_height = padding.height;
  data->op_params.pad_width  = padding.width;

  const int num_channels = data->op_params.out_channels;

  if (input->type == kTfLiteInt8) {
    if (sli_mvp_ml_conv2d_s8_is_supported(&data->op_params)) {
      data->supported = kMvp;

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
        context, input, filter, output, params->activation,
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_min),
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_max),
        scaler_data, num_channels, SLI_MVP_ACCUMULATOR_MULTIPLIER));

    } else {
      data->per_channel_output_multiplier = static_cast<int32_t*>(context->AllocatePersistentBuffer(
                                            context, num_channels * sizeof(int32_t)));
      data->per_channel_output_shift = static_cast<int32_t*>(context->AllocatePersistentBuffer(
                                       context, num_channels * sizeof(int32_t)));

      int32_t dummy_output_multiplier;
      int dummy_output_shift;
      TF_LITE_ENSURE_STATUS(tflite::PopulateConvolutionQuantizationParams(
        context, input, filter, bias, output, params->activation,
        &dummy_output_multiplier, &dummy_output_shift,
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_min),
        reinterpret_cast<int32_t*>(&data->op_params.output_activation_max),
        data->per_channel_output_multiplier,
        reinterpret_cast<int*>(data->per_channel_output_shift),
        num_channels));

      if (data->op_params.dilation_height == 1 && data->op_params.dilation_width == 1) {
        data->supported = kCmsisNN;
        cmsis_nn_conv_params       conv_params;
        conv_params.input_offset   = data->op_params.input_offset;
        conv_params.output_offset  = data->op_params.output_offset;
        conv_params.stride.h       = data->op_params.stride_height;
        conv_params.stride.w       = data->op_params.stride_width;
        conv_params.dilation.h     = 1;
        conv_params.dilation.w     = 1;
        conv_params.padding.h      = data->op_params.pad_height;
        conv_params.padding.w      = data->op_params.pad_width;
        conv_params.activation.min = data->op_params.output_activation_min;
        conv_params.activation.max = data->op_params.output_activation_max;

        cmsis_nn_dims input_dims;
        input_dims.n = data->op_params.batches;
        input_dims.h = data->op_params.input_height;
        input_dims.w = data->op_params.input_width;
        input_dims.c = data->op_params.in_channels;

        cmsis_nn_dims filter_dims;
        filter_dims.h = data->op_params.filter_height;
        filter_dims.w = data->op_params.filter_width;

        cmsis_nn_dims output_dims;
        output_dims.h = data->op_params.output_height;
        output_dims.w = data->op_params.output_width;
        output_dims.c = data->op_params.out_channels;

        scratch_buffer_size = arm_convolve_wrapper_s8_get_buffer_size(
                              &conv_params, &input_dims, &filter_dims, &output_dims);
      } else {
        data->supported = kTFLMrefI8;
      }
    }

  } else if (input->type == kTfLiteFloat32) {
    data->supported = kTFLMrefF32;
    CalculateActivationRange(params->activation,
                             &data->activation_min_f32,
                             &data->activation_max_f32);

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
  data->op_params.input  = tflite::micro::GetTensorData<int8_t>(input);
  data->op_params.output = tflite::micro::GetTensorData<int8_t>(output);
  data->op_params.filter = tflite::micro::GetTensorData<int8_t>(filter);

  TF_LITE_ENSURE_EQ(context, SL_STATUS_OK, sli_mvp_ml_conv2d_s8(&data->op_params));

  return kTfLiteOk;
}

TfLiteStatus eval_cmsis_int8(TfLiteContext* context,
                             OpData* data,
                             const TfLiteEvalTensor* input,
                             const TfLiteEvalTensor* filter,
                             const TfLiteEvalTensor* bias,
                             TfLiteEvalTensor* output)
{
  cmsis_nn_dims input_dims;
  input_dims.n = data->op_params.batches;
  input_dims.h = data->op_params.input_height;
  input_dims.w = data->op_params.input_width;
  input_dims.c = data->op_params.in_channels;

  cmsis_nn_dims filter_dims;
  filter_dims.n = data->op_params.out_channels;
  filter_dims.h = data->op_params.filter_height;
  filter_dims.w = data->op_params.filter_width;
  filter_dims.c = data->op_params.in_channels;

  cmsis_nn_dims bias_dims;
  bias_dims.n = 1;
  bias_dims.h = 1;
  bias_dims.w = 1;
  bias_dims.c = data->op_params.out_channels;

  cmsis_nn_dims output_dims;
  output_dims.n = data->op_params.batches;
  output_dims.h = data->op_params.output_height;
  output_dims.w = data->op_params.output_width;
  output_dims.c = data->op_params.out_channels;

  cmsis_nn_per_channel_quant_params quant_params;
  quant_params.multiplier = data->per_channel_output_multiplier;
  quant_params.shift = data->per_channel_output_shift;

  cmsis_nn_conv_params       conv_params;
  conv_params.input_offset   = data->op_params.input_offset;
  conv_params.output_offset  = data->op_params.output_offset;
  conv_params.stride.h       = data->op_params.stride_height;
  conv_params.stride.w       = data->op_params.stride_width;
  conv_params.dilation.h     = 1;
  conv_params.dilation.w     = 1;
  conv_params.padding.h      = data->op_params.pad_height;
  conv_params.padding.w      = data->op_params.pad_width;
  conv_params.activation.min = data->op_params.output_activation_min;
  conv_params.activation.max = data->op_params.output_activation_max;

  cmsis_nn_context ctx;
  ctx.buf = nullptr;
  ctx.size = 0;

  if (data->scratch_buffer_index > -1) {
    ctx.buf = context->GetScratchBuffer(context, data->scratch_buffer_index);
  }
  TFLITE_DCHECK_EQ(ARM_MATH_SUCCESS,
                   arm_convolve_wrapper_s8(
                     &ctx, &conv_params, &quant_params,
                     &input_dims,  tflite::micro::GetTensorData<int8_t>(input),
                     &filter_dims, tflite::micro::GetTensorData<int8_t>(filter),
                     &bias_dims,   bias == nullptr ? NULL : tflite::micro::GetTensorData<int32_t>(bias),
                     &output_dims, tflite::micro::GetTensorData<int8_t>(output)));

  return kTfLiteOk;
}

TfLiteStatus eval_tflm_int8(OpData* data,
                            const TfLiteEvalTensor* input,
                            const TfLiteEvalTensor* filter,
                            const TfLiteEvalTensor* bias,
                            TfLiteEvalTensor* output)
{
  ConvParams op_params;

  op_params.input_offset             = data->op_params.input_offset;
  op_params.output_offset            = data->op_params.output_offset;
  op_params.stride_height            = data->op_params.stride_height;
  op_params.stride_width             = data->op_params.stride_width;
  op_params.dilation_height_factor   = data->op_params.dilation_height;
  op_params.dilation_width_factor    = data->op_params.dilation_width;
  op_params.padding_values.height    = data->op_params.pad_height;
  op_params.padding_values.width     = data->op_params.pad_width;
  op_params.quantized_activation_min = data->op_params.output_activation_min;
  op_params.quantized_activation_max = data->op_params.output_activation_max;

  reference_integer_ops::ConvPerChannel(
    op_params,
    data->per_channel_output_multiplier,
    data->per_channel_output_shift,
    tflite::micro::GetTensorShape(input),
    tflite::micro::GetTensorData<int8_t>(input),
    tflite::micro::GetTensorShape(filter),
    tflite::micro::GetTensorData<int8_t>(filter),
    tflite::micro::GetTensorShape(bias),
    bias == nullptr ? nullptr : tflite::micro::GetTensorData<int32_t>(bias),
    tflite::micro::GetTensorShape(output),
    tflite::micro::GetTensorData<int8_t>(output));

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
  op_params.dilation_width_factor  = data->op_params.dilation_width;
  op_params.dilation_height_factor = data->op_params.dilation_height;
  op_params.float_activation_min   = data->activation_min_f32;
  op_params.float_activation_max   = data->activation_max_f32;

  reference_ops::Conv(op_params,
                      tflite::micro::GetTensorShape(input),
                      tflite::micro::GetTensorData<float>(input),
                      tflite::micro::GetTensorShape(filter),
                      tflite::micro::GetTensorData<float>(filter),
                      tflite::micro::GetTensorShape(bias),
                      bias == nullptr ? nullptr : tflite::micro::GetTensorData<float>(bias),
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
  const auto bias   = NumInputs(node) == 3
                      ? tflite::micro::GetEvalInput(context, node, kBiasTensor)
                      : nullptr;
  auto output       = tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  if (data->supported == kMvp) {
    status = eval_mvp_int8(context, data, input, filter, output);

  } else if (data->supported == kCmsisNN) {
    status = eval_cmsis_int8(context, data, input, filter, bias, output);

  } else if (data->supported == kTFLMrefI8) {
    status = eval_tflm_int8(data, input, filter, bias, output);

  } else if (data->supported == kTFLMrefF32) {
    #if EI_TFLITE_DISABLE_CONV_2D_IN_F32
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(input->type), input->type);
    return kTfLiteError;
    #endif

    status = eval_float(params, data, input, filter, bias, output);
  }

  return status;
}

}  // namespace conv2d
}  // namespace sl

TfLiteRegistration Register_CONV_2D() {
  return {/*init=*/sl::conv2d::Init,
          /*free=*/nullptr,
          /*prepare=*/sl::conv2d::Prepare,
          /*invoke=*/sl::conv2d::Invoke,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#elif EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN == 1
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

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

#include "freertos/FreeRTOS.h"
#include <esp_timer.h>
#include "edge-impulse-sdk/porting/espressif/ESP-NN/include/esp_nn.h"

long long conv_total_time = 0;

namespace tflite {
namespace {

struct NodeData {
  OpDataConv op_data;
  int buffer_idx;
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(NodeData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  NodeData* data = static_cast<NodeData*>(node->user_data);
  const auto& params =
      *(static_cast<const TfLiteConvParams*>(node->builtin_data));

  TfLiteTensor* output = GetOutput(context, node, kConvOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);
  const TfLiteTensor* input = GetInput(context, node, kConvInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter = GetInput(context, node, kConvWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);

  const int input_width = input->dims->data[2];
  const int input_height = input->dims->data[1];
  const int filter_width = filter->dims->data[2];
  const int filter_height = filter->dims->data[1];
  const int output_width = output->dims->data[2];
  const int output_height = output->dims->data[1];

  // Dynamically allocate per-channel quantization parameters.
  const int num_channels = filter->dims->data[kConvQuantizedDimension];
  data->op_data.per_channel_output_multiplier =
      static_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));
  data->op_data.per_channel_output_shift =
      static_cast<int32_t*>(context->AllocatePersistentBuffer(
          context, num_channels * sizeof(int32_t)));

  // All per-channel quantized tensors need valid zero point and scale arrays.
  if (input->type == kTfLiteInt8) {
    TF_LITE_ENSURE_EQ(context, filter->quantization.type,
                      kTfLiteAffineQuantization);

    const auto* affine_quantization =
        static_cast<TfLiteAffineQuantization*>(filter->quantization.params);
    TFLITE_DCHECK(affine_quantization != nullptr);
    TFLITE_DCHECK(affine_quantization->scale != nullptr);
    TFLITE_DCHECK(affine_quantization->zero_point != nullptr);

    TF_LITE_ENSURE(context,
                   affine_quantization->scale->size == 1 ||
                       affine_quantization->scale->size ==
                           filter->dims->data[kConvQuantizedDimension]);
    TF_LITE_ENSURE_EQ(context, affine_quantization->scale->size,
                      affine_quantization->zero_point->size);
  }

  TF_LITE_ENSURE_STATUS(CalculateOpDataConv(
      context, node, params, input_width, input_height, filter_width,
      filter_height, output_width, output_height, input->type, &data->op_data));

  if (input->type == kTfLiteInt8) {

    data_dims_t input_dims =  {
                                .width = input_width, .height = input_height,
                                .channels = input->dims->data[3], 1
                              };
    data_dims_t output_dims = {
                                .width = output_width, .height = output_height,
                                .channels = output->dims->data[3], 1
                              };
    data_dims_t filter_dims = {.width = filter_width, .height = filter_height, 0, 0};
    conv_params_t conv_params = {
                                  .in_offset = 0, .out_offset = 0,
                                  .stride = {params.stride_width, params.stride_height},
                                  .padding = {data->op_data.padding.width, data->op_data.padding.height},
                                  .dilation = {0, 0}, .activation = {-128, 127}
                                };

    int scratch_buf_size = esp_nn_get_conv_scratch_size(
        &input_dims, &filter_dims, &output_dims, &conv_params);
    if (scratch_buf_size > 0) {
      TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
        context, scratch_buf_size, &data->buffer_idx));
    } else {
      data->buffer_idx = -1;
    }
  }

  //micro_context->DeallocateTempTfLiteTensor(output);
  //micro_context->DeallocateTempTfLiteTensor(input);
  //micro_context->DeallocateTempTfLiteTensor(filter);

  return kTfLiteOk;
}

// Fixed-point per-channel-quantization convolution Int8 function wrapper.
inline void EvalQuantizedPerChannel(
    TfLiteContext* context, TfLiteNode* node, const TfLiteConvParams& params,
    const NodeData& data, const TfLiteEvalTensor* input,
    const TfLiteEvalTensor* filter, const TfLiteEvalTensor* bias,
    TfLiteEvalTensor* output) {
  const int dilation_width_factor = params.dilation_width_factor;
  const int dilation_height_factor = params.dilation_height_factor;

  if (dilation_width_factor == 1 && dilation_height_factor == 1) {
    // Get parameters.
    RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
    RuntimeShape input_shape = tflite::micro::GetTensorShape(input);
    RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
    RuntimeShape bias_shape = tflite::micro::GetTensorShape(bias);

    const int8_t *input_data = tflite::micro::GetTensorData<int8_t>(input);
    int8_t *output_data = tflite::micro::GetTensorData<int8_t>(output);

    const int32_t input_offset = -data.op_data.input_zero_point;
    const int32_t output_offset = data.op_data.output_zero_point;
    const int stride_width = params.stride_width;
    const int stride_height = params.stride_height;
    const int pad_width = data.op_data.padding.width;
    const int pad_height = data.op_data.padding.height;

    const int input_height = input_shape.Dims(1);
    const int input_width = input_shape.Dims(2);
    const int filter_height = filter_shape.Dims(1);
    const int filter_width = filter_shape.Dims(2);
    const int output_height = output_shape.Dims(1);
    const int output_width = output_shape.Dims(2);

    // Set min and max value of the output.
    const int32_t activation_min = data.op_data.output_activation_min;
    const int32_t activation_max = data.op_data.output_activation_max;

    // Consistency check.
    TFLITE_DCHECK_LE(activation_min, activation_max);
    TFLITE_DCHECK_EQ(input_shape.DimensionsCount(), 4);
    TFLITE_DCHECK_EQ(filter_shape.DimensionsCount(), 4);
    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 4);
    const int batch_size = MatchingDim(input_shape, 0, output_shape, 0);
    const int input_depth = MatchingDim(input_shape, 3, filter_shape, 3);
    const int output_depth = MatchingDim(filter_shape, 0, output_shape, 3);

    if (tflite::micro::GetTensorData<int8_t>(bias)) {
      TFLITE_DCHECK_EQ(bias_shape.FlatSize(), output_depth);
    }

    void *scratch_buf = NULL;
    if (data.buffer_idx > -1) {
      scratch_buf = context->GetScratchBuffer(context, data.buffer_idx);
    }
    esp_nn_set_conv_scratch_buf(scratch_buf);

    const int input_size = input_width * input_height * input_depth;
    const int output_size = output_width * output_height * output_depth;

    data_dims_t input_dims =  {
                                .width = input_width, .height = input_height,
                                .channels = input_depth, 1
                              };
    data_dims_t output_dims = {
                                .width = output_width, .height = output_height,
                                .channels = output_depth, 1
                              };
    data_dims_t filter_dims = {.width = filter_width, .height = filter_height, 0, 0};
    conv_params_t conv_params = {
                                  .in_offset = input_offset, .out_offset = output_offset,
                                  .stride = {stride_width, stride_height},
                                  .padding = {pad_width, pad_height},
                                  .dilation = {0, 0},
                                  .activation = {activation_min, activation_max}
                                };
    quant_data_t quant_data = {
                                .shift = data.op_data.per_channel_output_shift,
                                .mult = data.op_data.per_channel_output_multiplier
                              };

    for (int i_batch = 0; i_batch < batch_size; i_batch++) {
      esp_nn_conv_s8(&input_dims, input_data + i_batch * input_size,
                     &filter_dims, tflite::micro::GetTensorData<int8_t>(filter),
                     tflite::micro::GetTensorData<int32_t>(bias),
                     &output_dims, output_data + i_batch * output_size,
                     &conv_params, &quant_data);
    }
  } else {
    reference_integer_ops::ConvPerChannel(
        ConvParamsQuantized(params, data.op_data),
        data.op_data.per_channel_output_multiplier,
        data.op_data.per_channel_output_shift,
        tflite::micro::GetTensorShape(input),
        tflite::micro::GetTensorData<int8_t>(input),
        tflite::micro::GetTensorShape(filter),
        tflite::micro::GetTensorData<int8_t>(filter),
        tflite::micro::GetTensorShape(bias),
        tflite::micro::GetTensorData<int32_t>(bias),
        tflite::micro::GetTensorShape(output),
        tflite::micro::GetTensorData<int8_t>(output));
  }
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kConvInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
  const TfLiteEvalTensor* bias =
      (NumInputs(node) == 3)
          ? tflite::micro::GetEvalInput(context, node, kConvBiasTensor)
          : nullptr;
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kConvOutputTensor);

  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto& params =
      *(reinterpret_cast<TfLiteConvParams*>(node->builtin_data));
  TFLITE_DCHECK(node->user_data != nullptr);
  const auto& data = *(static_cast<const NodeData*>(node->user_data));

  TF_LITE_ENSURE_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  long long start_time = esp_timer_get_time();
  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_CONV_2D_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      tflite::reference_ops::Conv(
          ConvParamsFloat(params, data.op_data),
          tflite::micro::GetTensorShape(input),
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
      #if EI_TFLITE_DISABLE_CONV_2D_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      EvalQuantizedPerChannel(context, node, params, data, input, filter,
                              bias, output);
      break;
    }
    case kTfLiteUInt8: {
      #if EI_TFLITE_DISABLE_CONV_2D_IN_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      //EvalQuantized
      reference_ops::Conv(ConvParamsQuantized(params, data.op_data),
                          tflite::micro::GetTensorShape(input),
                          tflite::micro::GetTensorData<uint8_t>(input),
                          tflite::micro::GetTensorShape(filter),
                          tflite::micro::GetTensorData<uint8_t>(filter),
                          tflite::micro::GetTensorShape(bias),
                          tflite::micro::GetTensorData<int32_t>(bias),
                          tflite::micro::GetTensorShape(output),
                          tflite::micro::GetTensorData<uint8_t>(output),
                          tflite::micro::GetTensorShape(nullptr), nullptr,
                          nullptr);
      break;
    }

    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  
  long long time_this_instance = esp_timer_get_time() - start_time;
  conv_total_time += time_this_instance;
  //printf("time this instance: %llu\n", time_this_instance / 1000);

  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_CONV_2D() {
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

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/conv.h"

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/conv.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/padding.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpDataConv));
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kConvInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kConvWeightsTensor);
  const TfLiteEvalTensor* bias =
      (NumInputs(node) == 3)
          ? tflite::micro::GetEvalInput(context, node, kConvBiasTensor)
          : nullptr;
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kConvOutputTensor);

  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto& params =
      *(reinterpret_cast<TfLiteConvParams*>(node->builtin_data));
  TFLITE_DCHECK(node->user_data != nullptr);
  const auto& data = *(static_cast<const OpDataConv*>(node->user_data));

  TF_LITE_ENSURE_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  switch (input->type) {  // Already know in/out types are same.
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_CONV_2D_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::Conv(
          ConvParamsFloat(params, data), tflite::micro::GetTensorShape(input),
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
      #if EI_TFLITE_DISABLE_CONV_2D_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      reference_integer_ops::ConvPerChannel(
          ConvParamsQuantized(params, data), data.per_channel_output_multiplier,
          data.per_channel_output_shift, tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<int8_t>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<int32_t>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int8_t>(output));
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

TfLiteRegistration Register_CONV_2D() {
  return {/*init=*/Init,
          /*free=*/nullptr,
          /*prepare=*/ConvPrepare,
          /*invoke=*/Eval,
          /*profiling_string=*/nullptr,
          /*builtin_code=*/0,
          /*custom_name=*/nullptr,
          /*version=*/0};
}

}  // namespace tflite

#endif
