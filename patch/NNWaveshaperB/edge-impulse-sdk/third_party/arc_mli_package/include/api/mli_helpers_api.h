/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

/**
 * @file MLI Library Helpers API
 *
 * @brief This header includes declarations for helpers set of functions
 */

#ifndef _MLI_HELPERS_API_H_
#define _MLI_HELPERS_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mli_types.h"

/**
 * Set of helper defines to index the shape array.
 */
#define FMAP_H_DIM_CHW 1 // height
#define FMAP_W_DIM_CHW 2 // width
#define FMAP_C_DIM_CHW 0 // channels

#define KRNL_H_DIM_CHW 2 // kernel height
#define KRNL_W_DIM_CHW 3 // kernel width
#define KRNL_D_DIM_CHW 1 // kernel depth aka input channels
#define KRNL_C_DIM_CHW 0 // output channels

#define FMAP_H_DIM_HWC 0 // height
#define FMAP_W_DIM_HWC 1 // width
#define FMAP_C_DIM_HWC 2 // channels

#define KRNL_H_DIM_HWC 1 // kernel height
#define KRNL_W_DIM_HWC 2 // kernel width
#define KRNL_D_DIM_HWC 3 // kernel depth aka input channels
#define KRNL_C_DIM_HWC 0 // output channels

// for Depthwise convolution hwc kernel
#define KRNL_DW_D_DIM_HWC 0 // Depthwise convolution hwc kernel depth (must be == 1)
#define KRNL_DW_H_DIM_HWC 1 // Depthwise convolution hwc kernel height 
#define KRNL_DW_W_DIM_HWC 2 // Depthwise convolution hwc kernel width
#define KRNL_DW_C_DIM_HWC 3 // Depthwise convolution hwc output channels

/** 
 * @brief Count Number of Elements in Tensor
 *
 * @detail Function calculates the number of elements in a tensor starting from the provided 
 * dimension number (dimension numbering starts from 0). If start_dim=0 function calculates total number of elements 
 * Function returns 0 if input tensor rank is invalid or start_dim is bigger then input rank
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in         [I] Input tensor (of any shape)
 * @param start_dim  [I] Start dimension for counting
 *
 * @return Number of elements in tensor
 */
uint32_t mli_hlp_count_elem_num(const mli_tensor *in, uint32_t start_dim);

/** 
 * @brief Get Tensor Basic Element Size
 *
 * @detail This function returns size of tensor basic element in bytes. It returns 0 if  
 * pointer to tensor is invalid, or tensor contains unsupported element type.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in         [I] Input tensor (of any shape)
 *
 * @return Size of tensor basic element (bytes)
 */
uint32_t mli_hlp_tensor_element_size(const mli_tensor *in);

/** 
 * @brief Convert Tensor
 *
 * @detail This function copies elements from input tensor to output with data conversion according to 
 * the output tensor type parameters. This operation does not change tensor shape. It copies it from input to output. 
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input tensor (of any shape)
 * @param out     [O] Output tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_hlp_convert_tensor(mli_tensor *in, mli_tensor *out);

/** 
 * @brief Point to Sub-Tensor
 *
 * @detail This function points to sub tensors in input tensor. This can be considered as indexing in 
 * a multidimensional array. This function performs operations on pointers and doesn’t copy data 
 * (only points to subsequence of data in input). For this reason, this function takes only parameters that 
 * can be translated to starting coordinates and size of required data.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input tensor (of any shape)
 * @param cfg     [I] Configuration structure (for more info see @ref mli_point_to_subtsr_cfg)
 * @param out     [O] Output tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_hlp_point_to_subtensor(const mli_tensor *in, const mli_point_to_subtsr_cfg *cfg, mli_tensor *out);

/**
 * @brief Create a Sub-Tensor from a larger tensor
 *
 * @detail This function points to sub tensors in input tensor. This function performs operations 
 * on pointers and doesn’t copy data (only points to subsequence of data in input).
 * For this reason, depending on the parameters, it can happen that the sub tensor contains
 * data that is not adjacent in memory.
 *
 * For more info on primitive see MLI Documentation
 *
 * @param in      [I] Input tensor (of any shape)
 * @param cfg     [I] Configuration structure (for more info see @ref mli_sub_tensor_cfg)
 * @param out     [O] Output tensor. Result will be stored here
 *
 * @return MLI status code
 */
mli_status mli_hlp_create_subtensor(const mli_tensor *in, const mli_sub_tensor_cfg *cfg, mli_tensor *out);

uint32_t mli_hlp_tensor_scale_shift(const mli_tensor *in);

int32_t mli_hlp_tensor_scale(const mli_tensor *in, const uint32_t scale_idx);

int16_t mli_hlp_tensor_zero_offset(const mli_tensor *in, const uint32_t zero_idx);



#ifdef __cplusplus
}
#endif

#endif //_MLI_HELPERS_API_H_
