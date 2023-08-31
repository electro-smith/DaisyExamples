/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

/**
 * @file MLI Library Kernels API
 *
 * @brief This header includes declarations for kernels set of functions
 */

#ifndef _MLI_KERNELS_API_H_
#define _MLI_KERNELS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mli_krn_avepool_spec_api.h"
#include "mli_krn_conv2d_spec_api.h"
#include "mli_krn_depthwise_conv2d_spec_api.h"
#include "mli_krn_maxpool_spec_api.h"
#include "mli_types.h"



//================================================
//
// Convolution group of kernels
//
//================================================
/**
 * @brief 2D convolution
 *
 * @detail This kernel implements a general 2D convolution operation. It applies each filter of weights tensor 
 * to each framed area of the size of input tensor
 *
 * To implicitly insert additional points to sides of feature map (considering only width and height dimensions), 
 * ensure that you set the padding parameters. Padding influences how feature map is divided into patches 
 * for applying kernels because values of padded points are always zero.
 *
 * ReLU activation function may be applied to result of convolution. 
 *
 * For full list of specialized and highly optimized versions of kernel see @ref mli_krn_conv2d_spec_api.h
 * For more info on primitive see MLI Documentation
 * 
 * @param in      [I] Input feature map tensor (3-dimensional tensor)
 * @param weights [I] Convolution filters weights tensor (4-dimensional tensor)
 * @param bias    [I] Convolution filters biases tensor (1-dimensional tensor)
 * @param cfg     [I] Convolution parameters structure (for more info see @ref mli_conv2d_cfg)
 * @param out     [O] Output feature map tensor. Result is stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_conv2d_chw_fx8(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_generic(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_generic(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_hwc_fx8(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_hwc_fx16(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_hwc_fx8w16d(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);
        
mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32_generic(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

char * mli_debug_krn_conv2d_nhwc_sa8_sa8_sa32(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

/**
 * @brief 2D Depthwise convolution
 *
 * @detail This kernel implements a 2D depthwise convolution operation applying each filter channel 
 * to each input channel separatelly. As a result, output image depth is the same as input image depth.
 *
 * MLI implementation of depthwise convolution is compatible with caffe implementation of convolution layer 
 * with group parameter equal to number of input channels. In comparison with TensorFlow implementation 
 * (tf.nn.depthwise_conv2d in python API), this implementation does not support channel multiplier feature. 
 * Hence, the last dimension of weights tensor must be equal to 1.
 *
 * ReLU activation function may be applied to result of convolution. 
 *
 * For full list of specialized and highly optimized versions of kernel see @ref mli_krn_depthwise_conv2d_spec_api.h
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature map tensor (3-dimensional tensor)
 * @param weights [I] Convolution filters weights tensor (4-dimensional tensor)
 * @param bias    [I] Convolution filters biases tensor (1-dimensional tensor)
 * @param cfg     [I] Convolution parameters structure (for more info see @ref mli_conv2d_cfg)
 * @param out     [O] Output feature map tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_depthwise_conv2d_chw_fx8(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_depthwise_conv2d_chw_fx16(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_depthwise_conv2d_chw_fx8w16d(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);
mli_status mli_krn_depthwise_conv2d_hwcn_sa8_sa8_sa32(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_depthwise_conv2d_chw_fx8_generic(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_depthwise_conv2d_chw_fx16_generic(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_depthwise_conv2d_hwcn_sa8_sa8_sa32_generic(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_conv2d_cfg * cfg,
        mli_tensor * out);
        
char * mli_debug_krn_depthwise_conv2d_hwcn_sa8_sa8_sa32(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

//================================================
//
// Pooling group of kernels
//
//================================================
/**
 * @brief Average pooling
 *
 * @detail This kernel implements an average pooling operation. Each channel of input is considered independently, 
 * which means that the analysis fragment includes only neighbor points of the channel. For each fragment 
 * of input tensor, average value over all considered ponts is defined as the output value. 
 * The fragment size is defined in configuration structure according to kernel_width and kernel_height values.
 *
 * Window positioning and moving is performed according to stride and padding parameters. 
 * This logic is similar to convolution 2D operation. Average Pooling primitive does not analyze an area smaller 
 * than kernel size (typically, this occurs on the right and bottom borders). In this case, ensure that you set 
 * padding parameters explicitly in order not to miss valid border values. Padded values do not participate 
 * in the calculations. So when a fragment includes padded values, only the existing values are analysed 
 * (this also implies reducing of divider for average calculation).
 *
 * For full list of specialized and highly optimized versions of kernel see @ref mli_krn_avepool_spec_api.h
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature map tensor (3-dimensional tensor)
 * @param cfg     [I] Pooling parameters structure (for more info see @ref mli_pool_cfg)
 * @param out     [O] Output feature map tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_avepool_chw_fx8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx16(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_sa8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

/**
 * @brief MAX pooling
 *
 * @detail This kernel implements a max pooling operation. Each channel of input is considered independently, 
 * which means that the analysis fragment includes only neighbor points of the channel. For each fragment 
 * of input tensor, maximum value is being defined as the output value. The fragment size is defined in configuration
 * structure according to kernel_width and kernel_height values.
 *
 * Splitting input on fragments is performed according to stride and padding parameters. This logic is similar to 
 * convolution 2D operation
 *
 * For full list of specialized and highly optimized versions of kernel see @ref mli_krn_maxpool_spec_api.h
 * For more info on primitive see MLI Documentation

 * @param in      [I] Input feature map tensor (3-dimensional tensor)
 * @param cfg     [I] Pooling parameters structure (for more info see @ref mli_pool_cfg)
 * @param out     [O] Output feature map tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_maxpool_chw_fx8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_chw_fx16(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_chw_fx8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_chw_fx16_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_hwc_fx8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_hwc_fx16(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_hwc_sa8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_hwc_fx8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_hwc_fx16_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_maxpool_hwc_sa8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

//================================================
//
// Common group of kernels
//
//================================================
/**
 * @brief Fully connected
 *
 * @detail This kernel implements fully connected layer, also usually referred to as the inner product or dense layer.
 *
 * Ensure that the weight for this kernel is a 2-dimensional tensor (matrix of shape [M, N]),
 * and Bias must be 1-dimensional tensor of shape [M]. Shape of input tensor is not considered and only total number 
 * of elements is considered (must be equal to N). Kernel outputs a 1-dimensional tensor of shape [M].
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature tensor (of any shape)
 * @param weights [I] Weights tensor (2-dimensional tensor)
 * @param bias    [I] Biases tensor (1-dimensional tensor)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_fully_connected_fx8(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        mli_tensor * out);

mli_status mli_krn_fully_connected_fx16(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        mli_tensor * out);

mli_status mli_krn_fully_connected_fx8w16d(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        mli_tensor * out);

mli_status mli_krn_fully_connected_sa8_sa8_sa32(
        const mli_tensor * in,
        const mli_tensor * weights,
        const mli_tensor * bias,
        mli_tensor * out);
/**
 * @brief Long Short Term Memory (LSTM) Cell
 *
 * @detail This kernel implements the default non-peephole implementation of long short term memory (LSTM) cell 
 *
 * The Kernel supports three types of output activation: Sigmoid, Hyperbolic tangent and No activation (identity function)
 * Kernel supports three modes of input processing: ONE_TO_ONE, BATCH_TO_BATCH, BATCH_TO_LAST
 * Kernel REQUIRES extra intermediate tensor for calculations. It must be passed through configuration structure.
 *
 * For more info on primitive see MLI Documentation.
 *
 * @param in        [I] Input feature tensor (of any shape or with the batchsize in the first dimensions for batched modes)
 * @param prev_out  [I] Previous output feature tensor (1-dimensional tensor)
 * @param weights   [I] Weights tensor (set of 4 matrixes in the [i,g,f,o] order: 3-dimensional tensor)
 * @param bias      [I] Biases tensor (set of 4 vectors in the [i,g,f,o] order: 2-dimensional tensor)
 * @param cfg       [I] LSTM Configuration structure (for more info see @ref mli_rnn_cell_cfg)
 * @param cell      [I/O] Cell memory state (1-dimensional tensor)
 * @param out       [O] Output feature tensor. Result will be stored here (single output or batch of outputs depending on mode)
 *
 * @return MLI status code
 */
mli_status mli_krn_lstm_cell_fx8(
        const mli_tensor * in,
        const mli_tensor * prev_out,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_rnn_cell_cfg * cfg,
        mli_tensor * cell,
        mli_tensor * out);

mli_status mli_krn_lstm_cell_fx16(
        const mli_tensor * in,
        const mli_tensor * prev_out,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_rnn_cell_cfg * cfg,
        mli_tensor * cell,
        mli_tensor * out);

mli_status mli_krn_lstm_cell_fx8w16d(
        const mli_tensor * in,
        const mli_tensor * prev_out,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_rnn_cell_cfg * cfg,
        mli_tensor * cell,
        mli_tensor * out);

/**
 * @brief Basic Recurrent Neural Network Cell
 *
 * @detail This kernel implements the basic recurrent cell without memory state.
 *
 * The Kernel supports three types of output activation: Sigmoid, Hyperbolic tangent and No activation (identity function)
 * Kernel supports three modes of input processing: ONE_TO_ONE, BATCH_TO_BATCH, BATCH_TO_LAST 
 * To support user-specific complex recurrent cells beside LSTM, basic RNN cell kernel in One-to-One mode 
 * can work with matrices with stacked weights to produce stacked output tensor. For example, if weights tensor 
 * is 3-dimensionl tensor of shape [L, M, M+N], and Bias of shape [L, M], the output tensor is of shape [L, M]. 
 * Kernel REQUIRES extra intermediate tensor for calculations in BATCH-TO-LAST mode. It must be passed through 
 * configuration structure.
 *
 * For more info on primitive see MLI Documentation.
 *
 * @param in        [I] Input feature tensor (of any shape or with the batchsize in the first dimensions for batched modes)
 * @param prev_out  [I] Previous output feature tensor (1-dimensional tensor)
 * @param weights   [I] Weights tensor (2-dimensional tensor. 3-dimensional tensor in case of stacked output for ONE-TO_ONE mode)
 * @param bias      [I] Biases tensor (1-dimensional tensor. 2-dimensional tensor in case of stacked output for ONE-TO_ONE mode)
 * @param cfg       [I] Configuration structure (for more info see @ref mli_rnn_cell_cfg)
 * @param out       [O] Output feature tensor. Result will be stored here (single output or batch of outputs depending on mode)
 *
 * @return MLI status code
 */
mli_status mli_krn_basic_rnn_cell_fx8(
        const mli_tensor * in,
        const mli_tensor * prev_out,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_rnn_cell_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_basic_rnn_cell_fx16(
        const mli_tensor * in,
        const mli_tensor * prev_out,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_rnn_cell_cfg * cfg,
        mli_tensor * out);

mli_status mli_krn_basic_rnn_cell_fx8w16d(
        const mli_tensor * in,
        const mli_tensor * prev_out,
        const mli_tensor * weights,
        const mli_tensor * bias,
        const mli_rnn_cell_cfg * cfg,
        mli_tensor * out);



//================================================
//
// Activation group of kernels
//
//================================================
/**
 * @brief ReLU Activation function
 *
 * @detail This kernel represents Rectified Linear unit (ReLU). It performs various types of the rectifier activation on input. 
 * The following types of ReLU supported by this type of kernel: General ReLU, ReLU1, ReLU6
 * Kernel outputs a tensor of the same shape and type as input tensor.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature tensor (of any shape)
 * @param cfg     [I] Configuration structure (for more info see @ref mli_relu_cfg)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_relu_fx8(const mli_tensor * in, const mli_relu_cfg * cfg, mli_tensor * out);
mli_status mli_krn_relu_fx16(const mli_tensor * in, const mli_relu_cfg * cfg, mli_tensor * out);

/**
 * @brief Leaky ReLU Activation function
 *
 * @detail This kernel represents Rectified Linear unit (ReLU) with a negative slope. 
 * The function accepts two tensors as input and one as output. The first input tensor is the feature map 
 * to be processed by the kernel, and the second input is a tensor-scalar that holds 
 * a negative slope coefficient(Note, special tensor-scalar form can be used. see mli_tensor description in MLI Documentation).
 * Kernel outputs a tensor of the same shape and type as input tensor.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in           [I] Input feature tensor (of any shape)
 * @param slope_coeff  [I] Slope coefficient scalar tensor 
 * @param out          [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_leaky_relu_fx8(const mli_tensor * in, const mli_tensor * slope_coeff, mli_tensor * out);
mli_status mli_krn_leaky_relu_fx16(const mli_tensor * in, const mli_tensor * slope_coeff, mli_tensor * out);

/**
 * @brief Sigmoid Activation function
 *
 * @detail This kernel performs sigmoid (also mentioned as logistic) activation function on input tensor element-wise
 * and stores the result to the output tensor. Kernel outputs a tensor of the same shape and type as input tensor.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature tensor (of any shape)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_sigm_fx8(const mli_tensor * in, mli_tensor * out);
mli_status mli_krn_sigm_fx16(const mli_tensor * in, mli_tensor * out);

/**
 * @brief Hyperbolic Tangent Activation function
 *
 * @detail This kernel performs hyperbolic tangent activation function on input tensor element-wise 
 * and store result to the output tensor. Kernel outputs a tensor of the same shape and type as input tensor.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature tensor (of any shape)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_tanh_fx8(const mli_tensor * in, mli_tensor * out);
mli_status mli_krn_tanh_fx16(const mli_tensor * in, mli_tensor * out);

/**
 * @brief Softmax
 *
 * @detail This kernel performs activation function which is a generalization of the logistic function.
 * The SoftMax function is often used as the final layer of a neural network-based classifier and it's output can be considered 
 * as a probability distribution over N different possible outcomes. The sum of all the entries tends to 1
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature tensor (of any shape)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_softmax_fx8(const mli_tensor * in, mli_tensor * out);
mli_status mli_krn_softmax_fx16(const mli_tensor * in, mli_tensor * out);



//================================================
//
// Elementwise group of kernels
//
//================================================
/*
 * @brief Elementwise Addition
 *
 * @detail This kernel adds two tensors of the same shape element-wise  and stores results to the output tensor 
 * saving the shape of inputs. It supports simple broadcasting of single value (scalar tensor) on general tensor.
 * One of the operands can be a scalar (Note, special tensor-scalar form can be used. see mli_tensor description in 
 * MLI Documentation)
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in1     [I] First input feature tensor (or scalar tensor)
 * @param in2     [I] Second input feature tensor (or scalar tensor)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_eltwise_add_fx8(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);
mli_status mli_krn_eltwise_add_fx16(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);

/*
 * @brief Elementwise Subtraction
 *
 * @detail This kernel subtracts element-wise, the second input tensor (subtrahend) from the first input tensor (minuend) 
 * and stores results to the output tensor It supports simple broadcasting of single value (scalar tensor) on general tensor.
 * One of the operands can be a scalar (Note, special tensor-scalar form can be used. see mli_tensor description in MLI Documentation)
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in1     [I] Minuend input feature tensor (or scalar tensor)
 * @param in2     [I] Subtrahend input feature tensor (or scalar tensor)
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_eltwise_sub_fx8(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);
mli_status mli_krn_eltwise_sub_fx16(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);

/* @brief Elementwise Multiplication
 *
 * @detail This kernel multiplies two tensors of the same shape element-wise and store results to the output tensor 
 * saving the shape of inputs. It supports simple broadcasting of single value (scalar tensor) on general tensor.
 * One of the operands can be a scalar (Note, special tensor-scalar form can be used. see mli_tensor description in MLI Documentation)
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in1     [I] First input feature tensor (or scalar tensor)
 * @param in2     [I] Second input feature tensor (or scalar tensor) 
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_eltwise_mul_fx8(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);
mli_status mli_krn_eltwise_mul_fx16(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);

/* @brief Elementwise MAX/MIN
 *
 * @detail This kernel finds element-wise maximum / minimum of inputs operands and store results to the output tensor
 * saving the shape of inputs. It supports simple broadcasting of single value (scalar tensor) on general tensor.
 * One of the operands can be a scalar (Note, special tensor-scalar form can be used. see mli_tensor description in MLI Documentation)
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in1     [I] First input feature tensor (or scalar tensor)
 * @param in2     [I] Second input feature tensor (or scalar tensor) 
 * @param out     [O] Output feature tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_eltwise_min_fx8(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);
mli_status mli_krn_eltwise_min_fx16(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);
mli_status mli_krn_eltwise_max_fx8(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);
mli_status mli_krn_eltwise_max_fx16(const mli_tensor * in1, const mli_tensor * in2, mli_tensor * out);



//================================================
//
// Data manipulation group of kernels
//
//================================================
/** 
 * @brief Permute Tensor
 *
 * @detail The kernel permutes dimensions of input tensor according to provided order. In other words, it transposes input tensors.
 * The new order of dimensions is given by perm_dim array of kernel configuration structure. Output dimension #idx 
 * corresponds to the dimension of input tensor with #perm_dim[idx]. Tensor's data is reordered according to new shape.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input tensor (of any shape)
 * @param cfg     [I] Permute parameters structure (for more info see @ref mli_permute_cfg)
 * @param out     [O] Output tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_permute_fx8(const mli_tensor * in, const mli_permute_cfg * cfg, mli_tensor * out);
mli_status mli_krn_permute_fx16(const mli_tensor * in, const mli_permute_cfg * cfg, mli_tensor * out);

/** 
 * @brief Concatenation
 *
 * @detail This kernel concatenates multiple input tensors along one dimension to produce a single output tensor.
 * The kernel takes array of pointers to input tensors. Kernel configuration structure keeps number of 
 * input tensors (number of pointer in the array) and axis along which concatenation should be performed.
 * The shape of all input tensors must be the same except the target dimension for concatenation.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param inputs  [I] Tensors for concatenations
 * @param cfg     [I] Concatenation configuration structure (for more info see @ref mli_concat_cfg)
 * @param out     [O] Output tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_concat_fx8(const mli_tensor ** inputs, const mli_concat_cfg * cfg, mli_tensor * out);
mli_status mli_krn_concat_fx16(const mli_tensor ** inputs, const mli_concat_cfg * cfg, mli_tensor * out);

/** 
 * @brief 2D Padding 
 *
 * @detail The kernel performs zero padding of borders across height and width dimensions of vision specific input 
 * feature maps. Padding for each side of image (top, bottom, left, right) is configured separately according to input 
 * configuration structure, but the same padding for each side is used across all channels. Padding for HWC and CHW layouts 
 * of input tensor is implemented as separate functions.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input feature map tensor (3-dimensional tensor)
 * @param cfg     [I] 2D Padding configuration structure (for more info see @ref mli_padding2d_cfg)
 * @param out     [O] Output tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_krn_padding2d_chw_fx8(const mli_tensor * in, const mli_padding2d_cfg * cfg, mli_tensor * out);
mli_status mli_krn_padding2d_chw_fx16(const mli_tensor * in, const mli_padding2d_cfg * cfg, mli_tensor * out);
mli_status mli_krn_padding2d_hwc_fx8(const mli_tensor * in, const mli_padding2d_cfg * cfg, mli_tensor * out);
mli_status mli_krn_padding2d_hwc_fx16(const mli_tensor * in, const mli_padding2d_cfg * cfg, mli_tensor * out);

#ifdef __cplusplus
}
#endif
#endif    //_MLI_KERNELS_API_H_
