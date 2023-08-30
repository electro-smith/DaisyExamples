// Patched by Edge Impulse to include reference and hardware-accelerated kernels
#include "../../../../classifier/ei_classifier_config.h"
#if 0 == 1
/* noop */
#elif EI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN == 1
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

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/fully_connected.h"

#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nnfunctions.h"
#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

struct OpData {
  OpDataFullyConnected reference_op_data;

  // Index to buffer for optimizations if applicable.
  int buffer_idx;
};

// TODO(b/169801227): This global struct is needed for the linker to drop unused
// code (for example, by using Register_FULLY_CONNECTED_INT8 instead of
// Register_FULLY_CONNECTED).
TfLiteRegistration fully_connected_registration;

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  const auto params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteTensor* input =
      GetInput(context, node, kFullyConnectedInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter =
      GetInput(context, node, kFullyConnectedWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);
  const TfLiteTensor* bias =
      GetOptionalInputTensor(context, node, kFullyConnectedBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kFullyConnectedOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  // Set buffer index to a reset value
  data->buffer_idx = -1;
  TF_LITE_ENSURE_STATUS(CalculateOpDataFullyConnected(
      context, params->activation, input->type, input, filter, bias, output,
      &(data->reference_op_data)));

  if (input->type == kTfLiteInt8) {
    #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_I8
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(output->type), output->type);
    return kTfLiteError;
    #endif

    RuntimeShape filter_shape = GetTensorShape(filter);
    RuntimeShape output_shape = GetTensorShape(output);

    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 2);
    const int filter_dim_count = filter_shape.DimensionsCount();
    cmsis_nn_dims filter_dims;
    filter_dims.n = filter_shape.Dims(filter_dim_count - 1);
    filter_dims.h = 1;
    filter_dims.w = 1;
    filter_dims.c = output_shape.Dims(1);

    const int32_t buf_size =
        arm_fully_connected_s8_get_buffer_size(&filter_dims);

    if (buf_size > 0) {
      TF_LITE_ENSURE_STATUS(context->RequestScratchBufferInArena(
          context, buf_size, &data->buffer_idx));
    } else {
      data->buffer_idx = -1;
    }
  }
  return kTfLiteOk;
}

TfLiteStatus EvalQuantizedInt8(TfLiteContext* context, TfLiteNode* node,
                               const OpData& data,
                               const TfLiteEvalTensor* input,
                               const TfLiteEvalTensor* filter,
                               const TfLiteEvalTensor* bias,
                               TfLiteEvalTensor* output) {
  #if EI_TFLITE_DISABLE_FULLY_CONNECTED_OUT_I8
  TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                  TfLiteTypeGetName(output->type), output->type);
  return kTfLiteError;
  #endif

  const RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
  TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 2);
  const int batches = output_shape.Dims(0);
  const int output_depth = output_shape.Dims(1);
  const RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
  const int filter_dim_count = filter_shape.DimensionsCount();
  const int accum_depth = filter_shape.Dims(filter_dim_count - 1);
  const RuntimeShape input_shape = tflite::micro::GetTensorShape(input);

  cmsis_nn_fc_params fc_params;
  fc_params.input_offset = -data.reference_op_data.input_zero_point;
  fc_params.output_offset = data.reference_op_data.output_zero_point;
  fc_params.filter_offset = -data.reference_op_data.filter_zero_point;
  fc_params.activation.min = data.reference_op_data.output_activation_min;
  fc_params.activation.max = data.reference_op_data.output_activation_max;

  cmsis_nn_per_tensor_quant_params quant_params;
  quant_params.multiplier = data.reference_op_data.output_multiplier;
  quant_params.shift = data.reference_op_data.output_shift;

  cmsis_nn_dims input_dims;
  input_dims.n = batches;
  input_dims.h = 1;
  input_dims.w = 1;
  input_dims.c = accum_depth;

  cmsis_nn_dims filter_dims;
  filter_dims.n = accum_depth;
  filter_dims.h = 1;
  filter_dims.w = 1;
  filter_dims.c = output_depth;

  cmsis_nn_dims bias_dims;
  bias_dims.n = 1;
  bias_dims.h = 1;
  bias_dims.w = 1;
  bias_dims.c = output_depth;

  cmsis_nn_dims output_dims;
  output_dims.n = batches;
  output_dims.h = 1;
  output_dims.w = 1;
  output_dims.c = output_depth;

  cmsis_nn_context ctx;
  ctx.buf = nullptr;
  ctx.size = 0;

  if (data.buffer_idx > -1) {
    ctx.buf = context->GetScratchBuffer(context, data.buffer_idx);
  }

  TF_LITE_ENSURE_EQ(
      context,
      arm_fully_connected_s8(
          &ctx, &fc_params, &quant_params, &input_dims,
          tflite::micro::GetTensorData<int8_t>(input), &filter_dims,
          tflite::micro::GetTensorData<int8_t>(filter), &bias_dims,
          tflite::micro::GetTensorData<int32_t>(bias), &output_dims,
          tflite::micro::GetTensorData<int8_t>(output)),
      ARM_MATH_SUCCESS);

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto* params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kFullyConnectedOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  // Checks in Prepare ensure input, output and filter types are all the same.
  switch (input->type) {
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsFloat(params->activation),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<float>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<float>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      break;
    }
    case kTfLiteInt8: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      return EvalQuantizedInt8(context, node, data, input, filter, bias,
                               output);
    }
    case kTfLiteUInt8: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsQuantized(data.reference_op_data),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<uint8_t>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<uint8_t>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<int32_t>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<uint8_t>(output));
      break;
    }
    default: {
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
    }
  }
  return kTfLiteOk;
}

// Note that the current function names are not ideal at all (this EvalInt8
// function internally calls EvalQuantizedInt8, and there is similar name
// aliasing in the Eval function too). We will be attempting to have a more
// descriptive naming convention but holding off on that for now, since the
// renaming might be coupled with reducing code duplication and some additional
// refactoring.
TfLiteStatus EvalInt8(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kFullyConnectedOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  // Checks in Prepare ensure input, output and filter types are all the same.
  if (input->type != kTfLiteInt8) {
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                       TfLiteTypeGetName(input->type), input->type);
    return kTfLiteError;
  }

  return EvalQuantizedInt8(context, node, data, input, filter, bias, output);
}

}  // namespace

TfLiteRegistration Register_FULLY_CONNECTED() {
  fully_connected_registration.init = Init;
  fully_connected_registration.free = nullptr;
  fully_connected_registration.prepare = Prepare;
  fully_connected_registration.invoke = Eval;
  fully_connected_registration.profiling_string = nullptr;
  fully_connected_registration.builtin_code = 0;
  fully_connected_registration.custom_name = nullptr;
  fully_connected_registration.version = 0;
  return fully_connected_registration;
}

TfLiteRegistration Register_FULLY_CONNECTED_INT8() {
  fully_connected_registration.init = Init;
  fully_connected_registration.free = nullptr;
  fully_connected_registration.prepare = Prepare;
  fully_connected_registration.invoke = EvalInt8;
  fully_connected_registration.profiling_string = nullptr;
  fully_connected_registration.builtin_code = 0;
  fully_connected_registration.custom_name = nullptr;
  fully_connected_registration.version = 0;
  return fully_connected_registration;
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

#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/fully_connected.h"

#include "mli_api.h"  // NOLINT
#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/mli_slicers.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/mli_tf_utils.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/scratch_buf_mgr.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/scratch_buffers.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

struct OpData {
  // The scaling factor from input to output (aka the 'real multiplier') can
  // be represented as a fixed point multiplier plus a left shift.
  int32_t output_multiplier;
  int output_shift;
  // The range of the fused activation layer. For example for kNone and
  // uint8_t these would be 0 and 255.
  int32_t output_activation_min;
  int32_t output_activation_max;
  // The index of the temporary tensor where the quantized inputs are cached.
  int input_quantized_index;
  // Cached tensor zero point values for quantized operations.
  int32_t input_zero_point;
  int32_t filter_zero_point;
  int32_t output_zero_point;

  // The result of checking if MLI optimized version of tensors can be used.
  bool is_mli_applicable;

  // Tensors in MLI format.
  mli_tensor* mli_in;
  mli_tensor* mli_weights;
  mli_tensor* mli_bias;
  mli_tensor* mli_out;
};

constexpr int kInputTensor = 0;
constexpr int kWeightsTensor = 1;
constexpr int kBiasTensor = 2;
constexpr int kOutputTensor = 0;

bool IsMliApplicable(TfLiteContext* context, const TfLiteTensor* input,
                     const TfLiteTensor* filter, const TfLiteTensor* bias,
                     const TfLiteFullyConnectedParams* params) {
  // MLI optimized version only supports int8_t datatype and no fused Relu and
  // symmetric per-tensor quantization of weights (not per-axis)
  bool ret_val = (filter->type == kTfLiteInt8) &&
                 (input->type == kTfLiteInt8) && (bias->type == kTfLiteInt32) &&
                 (params->activation == kTfLiteActNone) &&
                 (filter->params.zero_point == 0);
  return ret_val;
}

TfLiteStatus CalculateOpData(TfLiteContext* context,
                             const TfLiteFullyConnectedParams* params,
                             TfLiteType data_type, const TfLiteTensor* input,
                             const TfLiteTensor* filter,
                             const TfLiteTensor* bias, TfLiteTensor* output,
                             OpData* data) {
  TfLiteStatus status = kTfLiteOk;
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  if (data_type != kTfLiteFloat32 && !data->is_mli_applicable) {
    double real_multiplier = 0.0;
    TF_LITE_ENSURE_STATUS(GetQuantizedConvolutionMultipler(
        context, input, filter, bias, output, &real_multiplier));
    int exponent;
    QuantizeMultiplier(real_multiplier, &data->output_multiplier, &exponent);
    data->output_shift = -exponent;
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));
  }
#endif
  return status;
}

}  // namespace

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  const auto params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteTensor* input = GetInput(context, node, kInputTensor);
  const TfLiteTensor* filter = GetInput(context, node, kWeightsTensor);
  const TfLiteTensor* bias = GetOptionalInputTensor(context, node, kBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);

  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  data->input_zero_point = input->params.zero_point;
  data->filter_zero_point = filter->params.zero_point;
  data->output_zero_point = output->params.zero_point;

  data->is_mli_applicable =
      IsMliApplicable(context, input, filter, bias, params);

  if (input->type == kTfLiteInt8 && data->is_mli_applicable) {
    #if EI_TFLITE_DISABLE_FULLY_CONNECTED_OUT_I8
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                    TfLiteTypeGetName(output->type), output->type);
    return kTfLiteError;
    #endif

    data->mli_in = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->mli_weights = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->mli_bias = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));
    data->mli_out = static_cast<mli_tensor*>(
        context->AllocatePersistentBuffer(context, sizeof(mli_tensor)));

    ops::micro::ConvertToMliTensor(input, data->mli_in);
    ops::micro::ConvertToMliTensor(filter, data->mli_weights);
    ops::micro::ConvertToMliTensor(bias, data->mli_bias);
    ops::micro::ConvertToMliTensor(output, data->mli_out);

    /* The input tensor can have more than 2 dimensions. for the compute this
   doesn't make any difference because all the inputs or a batch entry will
   be used anyway. because the MLI kernel doesn't recognize the multiple
   dimensions, the tensor shape is casted to a {batchnum, inputsize} shape. */
    data->mli_in->shape[0] = data->mli_out->shape[0];
    data->mli_in->shape[1] = data->mli_weights->shape[1];
    data->mli_in->shape[2] = 0;
    data->mli_in->shape[3] = 0;
    data->mli_in->rank = 2;
  }

  return (CalculateOpData(context, params, input->type, input, filter, bias,
                          output, data));
}

TfLiteStatus EvalMliQuantizedInt8(TfLiteContext* context, TfLiteNode* node,
                                  const TfLiteFullyConnectedParams* params,
                                  const OpData& data,
                                  const TfLiteEvalTensor* input,
                                  const TfLiteEvalTensor* filter,
                                  const TfLiteEvalTensor* bias,
                                  TfLiteEvalTensor* output) {
  ops::micro::MliTensorAttachBuffer<int8_t>(input, data.mli_in);
  ops::micro::MliTensorAttachBuffer<int8_t>(filter, data.mli_weights);
  ops::micro::MliTensorAttachBuffer<int32_t>(bias, data.mli_bias);
  ops::micro::MliTensorAttachBuffer<int8_t>(output, data.mli_out);

  // Tensors for data in fast (local) memory and config to copy data from
  // external to local memory
  mli_tensor weights_local = *data.mli_weights;
  mli_tensor bias_local = *data.mli_bias;
  mli_tensor in_local = *data.mli_in;
  mli_tensor out_local = *data.mli_out;
  mli_mov_cfg_t copy_config;
  mli_mov_cfg_for_copy(&copy_config);
  const int weight_out_dimension = 0;
  const int out_tensor_dimension = 1;
  const int input_size_dimension = 1;
  int slice_size = data.mli_weights->shape[weight_out_dimension];

  /* allocate the local buffers, and compute the slice size */
  TF_LITE_ENSURE_STATUS(
      ops::micro::get_arc_scratch_buffer_for_fully_connect_tensors(
          context, &in_local, &weights_local, &bias_local, &out_local));
  TF_LITE_ENSURE_STATUS(ops::micro::arc_scratch_buffer_calc_slice_size_weights(
      &weights_local, &bias_local, weight_out_dimension, &slice_size));
  int max_out_slice_size =
      out_local.capacity / mli_hlp_tensor_element_size(&out_local);
  if (slice_size > max_out_slice_size) slice_size = max_out_slice_size;

  /* is_local indicates that the tensor is already in local memory,
     so in that case the original tensor can be used,
     and there is no need to copy it to the local tensor*/
  const bool in_is_local = in_local.data == data.mli_in->data;
  const bool out_is_local = out_local.data == data.mli_out->data;
  const bool w_is_local = weights_local.data == data.mli_weights->data;
  const bool b_is_local = bias_local.data == data.mli_bias->data;

  ops::micro::TensorSlicer w_slice(data.mli_weights, weight_out_dimension,
                                   slice_size);
  ops::micro::TensorSlicer b_slice(data.mli_bias, weight_out_dimension,
                                   slice_size);
  ops::micro::TensorSlicer out_ch_slice(data.mli_out, out_tensor_dimension,
                                        slice_size, 0, 0, 0, true);

  mli_tensor* w_ptr = w_is_local ? w_slice.Sub() : &weights_local;
  mli_tensor* b_ptr = b_is_local ? b_slice.Sub() : &bias_local;

  void* input_buffer_ptr = NULL;

  while (!w_slice.Done()) {
    mli_mov_tensor_sync(w_slice.Sub(), &copy_config, w_ptr);
    mli_mov_tensor_sync(b_slice.Sub(), &copy_config, b_ptr);

    // Slice the input over the batches (one at a time with the size of a
    // complete input)
    ops::micro::TensorSlicer in_slice(data.mli_in, input_size_dimension,
                                      data.mli_in->shape[input_size_dimension]);

    /* output tensor is already sliced in the output size dimension.
    out_ch_slice.Sub() is the tensor for the amount of output size of this
    iteration of the weight slice loop. This tensor needs to be further
    sliced over the batch */
    ops::micro::TensorSlicer out_slice(out_ch_slice.Sub(), out_tensor_dimension,
                                       slice_size);

    /* setup the pointers to the local or remote tensor to make the code
     * inside the loop easier. */
    mli_tensor* in_ptr = in_is_local ? in_slice.Sub() : &in_local;
    mli_tensor* out_ptr = out_is_local ? out_slice.Sub() : &out_local;

    while (!out_slice.Done()) {
      // if same input copy as previous iteration, skip the copy of input
      if (in_slice.Sub()->data != input_buffer_ptr) {
        mli_mov_tensor_sync(in_slice.Sub(), &copy_config, in_ptr);
        input_buffer_ptr = in_slice.Sub()->data;
      }
      mli_krn_fully_connected_sa8_sa8_sa32(in_ptr, w_ptr, b_ptr, out_ptr);
      mli_mov_tensor_sync(out_ptr, &copy_config, out_slice.Sub());

      in_slice.Next();
      out_slice.Next();
    }
    w_slice.Next();
    b_slice.Next();
    out_ch_slice.Next();
  }
  return kTfLiteOk;
}

TfLiteStatus EvalQuantizedInt8(TfLiteContext* context, TfLiteNode* node,
                               const OpData& data,
                               const TfLiteEvalTensor* input,
                               const TfLiteEvalTensor* filter,
                               const TfLiteEvalTensor* bias,
                               TfLiteEvalTensor* output) {
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  tflite::FullyConnectedParams op_params;
  op_params.input_offset = -data.input_zero_point;
  op_params.weights_offset = -data.filter_zero_point;
  op_params.output_offset = data.output_zero_point;
  op_params.output_multiplier = data.output_multiplier;
  op_params.output_shift = -data.output_shift;
  op_params.quantized_activation_min = data.output_activation_min;
  op_params.quantized_activation_max = data.output_activation_max;

  reference_integer_ops::FullyConnected(
      op_params, tflite::micro::GetTensorShape(input),
      tflite::micro::GetTensorData<int8_t>(input),
      tflite::micro::GetTensorShape(filter),
      tflite::micro::GetTensorData<int8_t>(filter),
      tflite::micro::GetTensorShape(bias),
      tflite::micro::GetTensorData<int32_t>(bias),
      tflite::micro::GetTensorShape(output),
      tflite::micro::GetTensorData<int8_t>(output));
  return kTfLiteOk;
#else
  TF_LITE_KERNEL_LOG(context,
                     "Node configuration is not supported by ARC MLI Library.");
  return kTfLiteError;
#endif
}

TfLiteStatus EvalQuantized(TfLiteContext* context, TfLiteNode* node,
                           const OpData& data, const TfLiteEvalTensor* input,
                           const TfLiteEvalTensor* filter,
                           const TfLiteEvalTensor* bias,
                           TfLiteEvalTensor* output) {
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  const int32_t input_offset = -data.input_zero_point;
  const int32_t filter_offset = -data.filter_zero_point;
  const int32_t output_offset = data.output_zero_point;

  tflite::FullyConnectedParams op_params;
  op_params.input_offset = input_offset;
  op_params.weights_offset = filter_offset;
  op_params.output_offset = output_offset;
  op_params.output_multiplier = data.output_multiplier;
  // Legacy ops used mixed left and right shifts. Now all are +ve-means-left.
  op_params.output_shift = -data.output_shift;
  op_params.quantized_activation_min = data.output_activation_min;
  op_params.quantized_activation_max = data.output_activation_max;

#define TF_LITE_FULLY_CONNECTED(output_data_type)      \
  reference_ops::FullyConnected(                       \
      op_params, tflite::micro::GetTensorShape(input), \
      tflite::micro::GetTensorData<uint8_t>(input),    \
      tflite::micro::GetTensorShape(filter),           \
      tflite::micro::GetTensorData<uint8_t>(filter),   \
      tflite::micro::GetTensorShape(bias),             \
      tflite::micro::GetTensorData<int32_t>(bias),     \
      tflite::micro::GetTensorShape(output),           \
      tflite::micro::GetTensorData<output_data_type>(output))
  switch (output->type) {
    case kTfLiteUInt8:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_OUT_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(output->type), output->type);
      return kTfLiteError;
      #endif

      TF_LITE_FULLY_CONNECTED(uint8_t);
      break;
    case kTfLiteInt16:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_OUT_I16
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(output->type), output->type);
      return kTfLiteError;
      #endif

      TF_LITE_FULLY_CONNECTED(int16_t);
      break;
    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(output->type), output->type);
      return kTfLiteError;
  }

  return kTfLiteOk;
#else
  TF_LITE_KERNEL_LOG(context,
                     "Type %s (%d) is not supported by ARC MLI Library.",
                     TfLiteTypeGetName(input->type), input->type);
  return kTfLiteError;
#endif
}

TfLiteStatus EvalFloat(TfLiteContext* context, TfLiteNode* node,
                       TfLiteFusedActivation activation,
                       const TfLiteEvalTensor* input,
                       const TfLiteEvalTensor* filter,
                       const TfLiteEvalTensor* bias, TfLiteEvalTensor* output) {
#if !defined(TF_LITE_STRIP_REFERENCE_IMPL)
  float output_activation_min, output_activation_max;
  CalculateActivationRange(activation, &output_activation_min,
                           &output_activation_max);
  tflite::FullyConnectedParams op_params;
  op_params.float_activation_min = output_activation_min;
  op_params.float_activation_max = output_activation_max;
  tflite::reference_ops::FullyConnected(
      op_params, tflite::micro::GetTensorShape(input),
      tflite::micro::GetTensorData<float>(input),
      tflite::micro::GetTensorShape(filter),
      tflite::micro::GetTensorData<float>(filter),
      tflite::micro::GetTensorShape(bias),
      tflite::micro::GetTensorData<float>(bias),
      tflite::micro::GetTensorShape(output),
      tflite::micro::GetTensorData<float>(output));
  return kTfLiteOk;
#else
  TF_LITE_KERNEL_LOG(context,
                     "Type %s (%d) is not supported by ARC MLI Library.",
                     TfLiteTypeGetName(input->type), input->type);
  return kTfLiteError;
#endif
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto* params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kBiasTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  // Checks in Prepare ensure input, output and filter types are all the same.
  switch (input->type) {
    case kTfLiteFloat32:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_F32
      context->ReportError(context,
                            "Filter data type %s currently not supported.",
                            TfLiteTypeGetName(filter->type));
      return kTfLiteError;
      #endif

      return EvalFloat(context, node, params->activation, input, filter, bias,
                       output);
    case kTfLiteInt8:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_I8
      context->ReportError(context,
                            "Filter data type %s currently not supported.",
                            TfLiteTypeGetName(filter->type));
      return kTfLiteError;
      #endif

      if (data.is_mli_applicable) {
        return EvalMliQuantizedInt8(context, node, params, data, input, filter,
                                    bias, output);
      } else {
        return EvalQuantizedInt8(context, node, data, input, filter, bias,
                                 output);
      }

    case kTfLiteUInt8:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_U8
      context->ReportError(context,
                            "Filter data type %s currently not supported.",
                            TfLiteTypeGetName(filter->type));
      return kTfLiteError;
      #endif

      return EvalQuantized(context, node, data, input, filter, bias, output);

    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

TfLiteRegistration Register_FULLY_CONNECTED() {
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
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/fully_connected.h"

#include "edge-impulse-sdk/CMSIS/NN/Include/arm_nnfunctions.h"
#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"
#include "sl_mvp_ml_fully_connected.h"

namespace tflite {
namespace sl {
namespace fully_connected {

struct OpData {
  int32_t output_multiplier;
  int output_shift;
  sli_mvp_ml_fully_connected_s8_params_t op_params;
  float16_t *bias_fp16;
  bool use_mvp;
};

constexpr int kInputTensor = 0;
constexpr int kWeightsTensor = 1;
constexpr int kBiasTensor = 2;
constexpr int kOutputTensor = 0;

// TODO(b/169801227): This global struct is needed for the linker to drop unused
// code (for example, by using Register_FULLY_CONNECTED_INT8 instead of
// Register_FULLY_CONNECTED).
TfLiteRegistration fully_connected_registration;

sli_shape_t dims2shape(const TfLiteIntArray *dim)
{
  TFLITE_DCHECK(dim->size <= 4);

  sli_shape_t shape = {0};
  for (int i = 0; i < dim->size; i++) {
    shape.dim[i] = dim->data[i];
  }
  return shape;
}

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context, sizeof(OpData));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  OpData* data = static_cast<OpData*>(node->user_data);
  TfLiteFullyConnectedParams* params =
      reinterpret_cast<TfLiteFullyConnectedParams*>(node->builtin_data);
  const TfLiteTensor* input  = GetInput(context, node, kInputTensor);
  const TfLiteTensor* weight = GetInput(context, node, kWeightsTensor);
  const TfLiteTensor* bias   = GetInput(context, node, kBiasTensor);
  TfLiteTensor*       output = GetOutput(context, node, kOutputTensor);
  int32_t             output_min;
  int32_t             output_max;
  float16_t           *bias_data = nullptr;
  int                 bias_len = 0;

  TF_LITE_ENSURE(context, input  != nullptr);
  TF_LITE_ENSURE(context, output != nullptr);

  if (!(input->type == kTfLiteFloat32 || input->type == kTfLiteInt8)) {
    // Unsupported datatype used by model
    return kTfLiteError;
  }

  if (bias) {
    RuntimeShape bias_shape = GetTensorShape(bias);
    bias_len = bias_shape.FlatSize();
  }

  if (input->type == kTfLiteInt8) {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
    context, params->activation, output, &output_min, &output_max));

    double real_multiplier = 0.0;
    TF_LITE_ENSURE_STATUS(GetQuantizedConvolutionMultipler(
        context, input, weight, bias, output, &real_multiplier));

    data->op_params.input = GetTensorData<int8_t>(input);
    data->op_params.input_shape = dims2shape(input->dims);
    data->op_params.input_offset = -input->params.zero_point;
    data->op_params.weight = GetTensorData<int8_t>(weight);
    data->op_params.weight_shape = dims2shape(weight->dims);
    data->op_params.weight_offset = -weight->params.zero_point;
    data->op_params.bias = nullptr;
    data->op_params.bias_length = bias_len;
    data->op_params.output = GetTensorData<int8_t>(output);
    data->op_params.output_shape = dims2shape(output->dims);
    data->op_params.output_offset = output->params.zero_point;
    data->op_params.output_multiplier = sli_mvp_ml_fully_connected_output_multiplier(real_multiplier);
    data->op_params.activation_min = static_cast<int8_t>(output_min);
    data->op_params.activation_max = static_cast<int8_t>(output_max);

    data->use_mvp = sli_mvp_ml_fully_connected_s8_is_supported(&data->op_params);

    if (data->use_mvp && bias) {
      // Convert int32_t to float16_t as the MVP does not support loading int32 values.
      const int32_t *bias_src = GetTensorData<int32_t>(bias);
      bias_data = static_cast<float16_t *>(context->AllocatePersistentBuffer(context, bias_len * sizeof(float16_t)));
      if (bias_data == nullptr) {
        return kTfLiteError;
      }
      sl_status_t status = sli_mvp_ml_fully_connected_bias_convert(bias_src, bias_data, bias_len);
      if (status != SL_STATUS_OK) {
        return kTfLiteError;
      }
      data->op_params.bias = bias_data;
    }

    if (!data->use_mvp) {
      // In this case we have to convert the output scale factor to a
      // value in the TensorFlow fixed point format (Q.31 + shift)
      int exponent;
      QuantizeMultiplier(real_multiplier, &data->output_multiplier, &exponent);
      data->output_shift = -exponent;
    }
  }

  return kTfLiteOk;
}

TfLiteStatus EvalQuantizedInt8_MVP(TfLiteContext* context, TfLiteNode* node,
                               const OpData& data,
                               const TfLiteEvalTensor* input,
                               const TfLiteEvalTensor* filter,
                               const TfLiteEvalTensor* bias,
                               TfLiteEvalTensor* output) {
  sli_mvp_ml_fully_connected_s8_params_t *params = const_cast<sli_mvp_ml_fully_connected_s8_params_t*>(&data.op_params);
  params->input  = tflite::micro::GetTensorData<int8_t>(input);
  params->output = tflite::micro::GetTensorData<int8_t>(output);

  sl_status_t result = sli_mvp_ml_fully_connected_s8(params);
  if (result == SL_STATUS_OK) {
    return kTfLiteOk;
  } else {
    return kTfLiteError;
  }
}

TfLiteStatus EvalQuantizedInt8(TfLiteContext* context, TfLiteNode* node,
                               const OpData& data,
                               const TfLiteEvalTensor* input,
                               const TfLiteEvalTensor* filter,
                               const TfLiteEvalTensor* bias,
                               TfLiteEvalTensor* output) {
  if (data.use_mvp && input->type == kTfLiteInt8) {
    return EvalQuantizedInt8_MVP(context, node, data, input, filter, bias, output);
  }

  // The 'if' condition can be removed when null handling of bias is added to
  // arm_fully_connected_s8
  if (nullptr != tflite::micro::GetTensorData<int32_t>(bias)) {
    const RuntimeShape output_shape = tflite::micro::GetTensorShape(output);
    TFLITE_DCHECK_EQ(output_shape.DimensionsCount(), 2);
    const int batches = output_shape.Dims(0);
    const int output_depth = output_shape.Dims(1);
    const RuntimeShape filter_shape = tflite::micro::GetTensorShape(filter);
    const int filter_dim_count = filter_shape.DimensionsCount();
    const int accum_depth = filter_shape.Dims(filter_dim_count - 1);
    const RuntimeShape input_shape = tflite::micro::GetTensorShape(input);

    cmsis_nn_fc_params fc_params;
    fc_params.input_offset = data.op_params.input_offset;
    fc_params.output_offset = data.op_params.output_offset;
    fc_params.filter_offset = data.op_params.weight_offset;
    fc_params.activation.min = data.op_params.activation_min;
    fc_params.activation.max = data.op_params.activation_max;

    cmsis_nn_per_tensor_quant_params quant_params;
    quant_params.multiplier = data.output_multiplier;
    // TODO(b/138810107): Figure out whether output shift should be inverted
    quant_params.shift = -data.output_shift;

    cmsis_nn_dims input_dims;
    input_dims.n = batches;
    input_dims.h = 1;
    input_dims.w = 1;
    input_dims.c = accum_depth;

    cmsis_nn_dims filter_dims;
    filter_dims.n = accum_depth;
    filter_dims.h = 1;
    filter_dims.w = 1;
    filter_dims.c = output_depth;

    cmsis_nn_dims bias_dims;
    bias_dims.n = 1;
    bias_dims.h = 1;
    bias_dims.w = 1;
    bias_dims.c = output_depth;

    cmsis_nn_dims output_dims;
    output_dims.n = batches;
    output_dims.h = 1;
    output_dims.w = 1;
    output_dims.c = output_depth;

    cmsis_nn_context ctx;
    ctx.buf = nullptr;
    ctx.size = 0;

    TF_LITE_ENSURE_EQ(
        context,
        arm_fully_connected_s8(
            &ctx, &fc_params, &quant_params, &input_dims,
            tflite::micro::GetTensorData<int8_t>(input), &filter_dims,
            tflite::micro::GetTensorData<int8_t>(filter), &bias_dims,
            tflite::micro::GetTensorData<int32_t>(bias), &output_dims,
            tflite::micro::GetTensorData<int8_t>(output)),
        ARM_MATH_SUCCESS);
  } else {
    tflite::FullyConnectedParams op_params;
    op_params.input_offset = data.op_params.input_offset;
    op_params.weights_offset = data.op_params.weight_offset;
    op_params.output_offset = data.op_params.output_offset;
    op_params.output_multiplier = data.output_multiplier;
    // TODO(b/138810107): Figure out whether output shift should be inverted
    op_params.output_shift = -data.output_shift;
    op_params.quantized_activation_min = data.op_params.activation_min;
    op_params.quantized_activation_max = data.op_params.activation_max;

    reference_integer_ops::FullyConnected(
        op_params, tflite::micro::GetTensorShape(input),
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

TfLiteStatus EvalFloat(TfLiteContext* context, TfLiteNode* node,
                       TfLiteFusedActivation activation,
                       const TfLiteEvalTensor* input,
                       const TfLiteEvalTensor* filter,
                       const TfLiteEvalTensor* bias, TfLiteEvalTensor* output) {
  float output_activation_min, output_activation_max;
  CalculateActivationRange(activation, &output_activation_min,
                           &output_activation_max);
  tflite::FullyConnectedParams op_params;
  op_params.float_activation_min = output_activation_min;
  op_params.float_activation_max = output_activation_max;
  tflite::reference_ops::FullyConnected(
      op_params, tflite::micro::GetTensorShape(input),
      tflite::micro::GetTensorData<float>(input),
      tflite::micro::GetTensorShape(filter),
      tflite::micro::GetTensorData<float>(filter),
      tflite::micro::GetTensorShape(bias),
      tflite::micro::GetTensorData<float>(bias),
      tflite::micro::GetTensorShape(output),
      tflite::micro::GetTensorData<float>(output));
  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto* params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  switch (input->type) {
    case kTfLiteFloat32:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      return EvalFloat(context, node, params->activation, input, filter, bias,
                       output);
    case kTfLiteInt8:
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      return EvalQuantizedInt8(context, node, data, input, filter, bias,
                               output);

    default:
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
  }
  return kTfLiteOk;
}

// Note that the current function names are not ideal at all (this EvalInt8
// function internally calls EvalQuantizedInt8, and there is similar name
// aliasing in the Eval function too). We will be attempting to have a more
// descriptive naming convention but holding off on that for now, since the
// renaming might be coupled with reducing code duplication and some additional
// refactoring.
TfLiteStatus EvalInt8(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const OpData& data = *(static_cast<const OpData*>(node->user_data));

  // Checks in Prepare ensure input, output and filter types are all the same.
  if (input->type != kTfLiteInt8) {
    TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                       TfLiteTypeGetName(input->type), input->type);
    return kTfLiteError;
  }

  return EvalQuantizedInt8(context, node, data, input, filter, bias, output);
}

}  // namespace fully_connected
}  // namespace sl

TfLiteRegistration Register_FULLY_CONNECTED() {
  return {/*init*/sl::fully_connected::Init,
          /*free*/nullptr,
          /*prepare*/sl::fully_connected::Prepare,
          /*invoke*/sl::fully_connected::Eval,
          /*profiling_string*/nullptr,
          /*builtin_code*/0,
          /*custom_name*/nullptr,
          /*version*/0};
}

TfLiteRegistration Register_FULLY_CONNECTED_INT8() {
  return {/*init*/sl::fully_connected::Init,
          /*free*/nullptr,
          /*prepare*/sl::fully_connected::Prepare,
          /*invoke*/sl::fully_connected::EvalInt8,
          /*profiling_string*/nullptr,
          /*builtin_code*/0,
          /*custom_name*/nullptr,
          /*version*/0};
}

}  // namespace tflite

#elif EI_CLASSIFIER_TFLITE_ENABLE_ESP_NN == 1
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

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

#include "edge-impulse-sdk/porting/espressif/ESP-NN/include/esp_nn.h"
#include <esp_timer.h>

long long fc_total_time = 0;

namespace tflite {
namespace {

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context,
                                           sizeof(OpDataFullyConnected));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  auto* data = static_cast<OpDataFullyConnected*>(node->user_data);
  const auto params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteTensor* input =
      GetInput(context, node, kFullyConnectedInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter =
      GetInput(context, node, kFullyConnectedWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);
  const TfLiteTensor* bias =
      GetOptionalInputTensor(context, node, kFullyConnectedBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kFullyConnectedOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  return CalculateOpDataFullyConnected(context, params->activation, input->type,
                                       input, filter, bias, output, data);
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto* params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kFullyConnectedOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const auto& data =
      *(static_cast<const OpDataFullyConnected*>(node->user_data));

  long long start_time = esp_timer_get_time();
  // Checks in Prepare ensure input, output and filter types are all the same.
  switch (input->type) {
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsFloat(params->activation),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<float>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<float>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      break;
    }

    case kTfLiteInt8: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      const int32_t* bias_data =
          nullptr != bias ? tflite::micro::GetTensorData<int32_t>(bias)
                          : nullptr;
      const RuntimeShape& filter_shape = tflite::micro::GetTensorShape(filter);
      const RuntimeShape& output_shape = tflite::micro::GetTensorShape(output);
      const int filter_dim_count = filter_shape.DimensionsCount();
      const int batches = output_shape.Dims(0);
      const int output_depth = output_shape.Dims(1);
      TFLITE_DCHECK_LE(output_depth, filter_shape.Dims(filter_dim_count - 2));
      const int accum_depth = filter_shape.Dims(filter_dim_count - 1);

      const int8_t *input_data = tflite::micro::GetTensorData<int8_t>(input);
      int8_t *output_data = tflite::micro::GetTensorData<int8_t>(output);
      const int8_t *filter_data = tflite::micro::GetTensorData<int8_t>(filter);

      for (int b = 0; b < batches; ++b) {
        esp_nn_fully_connected_s8(input_data, -data.input_zero_point,
                                  accum_depth,
                                  filter_data, -data.filter_zero_point,
                                  bias_data, output_data, output_depth,
                                  data.output_zero_point,
                                  data.output_shift, data.output_multiplier,
                                  data.output_activation_min,
                                  data.output_activation_max);
        input_data += accum_depth;
        output_data += output_depth;
      }
      break;
    }

    case kTfLiteUInt8: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif
      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsQuantized(data),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<uint8_t>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<uint8_t>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<int32_t>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<uint8_t>(output));
      break;
    }
    
    default: {
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
    }
  }
  fc_total_time += esp_timer_get_time() - start_time;
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_FULLY_CONNECTED() {
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

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/fully_connected.h"

#include "edge-impulse-sdk/tensorflow/lite/c/builtin_op_data.h"
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/quantization_util.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/reference/integer_ops/fully_connected.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"
#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/kernel_util.h"

namespace tflite {
namespace {

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  TFLITE_DCHECK(context->AllocatePersistentBuffer != nullptr);
  return context->AllocatePersistentBuffer(context,
                                           sizeof(OpDataFullyConnected));
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->user_data != nullptr);
  TFLITE_DCHECK(node->builtin_data != nullptr);

  auto* data = static_cast<OpDataFullyConnected*>(node->user_data);
  const auto params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteTensor* input =
      GetInput(context, node, kFullyConnectedInputTensor);
  TF_LITE_ENSURE(context, input != nullptr);
  const TfLiteTensor* filter =
      GetInput(context, node, kFullyConnectedWeightsTensor);
  TF_LITE_ENSURE(context, filter != nullptr);
  const TfLiteTensor* bias =
      GetOptionalInputTensor(context, node, kFullyConnectedBiasTensor);
  TfLiteTensor* output = GetOutput(context, node, kFullyConnectedOutputTensor);
  TF_LITE_ENSURE(context, output != nullptr);

  TF_LITE_ENSURE_TYPES_EQ(context, input->type, output->type);
  TF_LITE_ENSURE_MSG(context, input->type == filter->type,
                     "Hybrid models are not supported on TFLite Micro.");

  return CalculateOpDataFullyConnected(context, params->activation, input->type,
                                       input, filter, bias, output, data);
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  TFLITE_DCHECK(node->builtin_data != nullptr);
  const auto* params =
      static_cast<const TfLiteFullyConnectedParams*>(node->builtin_data);

  const TfLiteEvalTensor* input =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedInputTensor);
  const TfLiteEvalTensor* filter =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedWeightsTensor);
  const TfLiteEvalTensor* bias =
      tflite::micro::GetEvalInput(context, node, kFullyConnectedBiasTensor);
  TfLiteEvalTensor* output =
      tflite::micro::GetEvalOutput(context, node, kFullyConnectedOutputTensor);

  TFLITE_DCHECK(node->user_data != nullptr);
  const auto& data =
      *(static_cast<const OpDataFullyConnected*>(node->user_data));

  // Checks in Prepare ensure input, output and filter types are all the same.
  switch (input->type) {
    case kTfLiteFloat32: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_F32
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsFloat(params->activation),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<float>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<float>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<float>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<float>(output));
      break;
    }

    case kTfLiteInt8: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_I8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_integer_ops::FullyConnected(
          FullyConnectedParamsQuantized(data),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<int8_t>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<int8_t>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<int32_t>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<int8_t>(output));
      break;
    }

    case kTfLiteUInt8: {
      #if EI_TFLITE_DISABLE_FULLY_CONNECTED_IN_U8
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                      TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
      #endif

      tflite::reference_ops::FullyConnected(
          FullyConnectedParamsQuantized(data),
          tflite::micro::GetTensorShape(input),
          tflite::micro::GetTensorData<uint8_t>(input),
          tflite::micro::GetTensorShape(filter),
          tflite::micro::GetTensorData<uint8_t>(filter),
          tflite::micro::GetTensorShape(bias),
          tflite::micro::GetTensorData<int32_t>(bias),
          tflite::micro::GetTensorShape(output),
          tflite::micro::GetTensorData<uint8_t>(output));
      break;
    }
    default: {
      TF_LITE_KERNEL_LOG(context, "Type %s (%d) not supported.",
                         TfLiteTypeGetName(input->type), input->type);
      return kTfLiteError;
    }
  }
  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration Register_FULLY_CONNECTED() {
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
