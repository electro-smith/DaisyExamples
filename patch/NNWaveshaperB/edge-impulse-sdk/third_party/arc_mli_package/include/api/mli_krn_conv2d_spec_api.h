/* This file is generated, do not edit!
 * edit following template file instead:
 * header_filetemplate.txt
 */
/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

#ifndef _MLI_KRN_CONV2D_SPEC_API_H_
#define _MLI_KRN_CONV2D_SPEC_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mli_types.h"

//===================================================================
// Convolution 2d specialization kernels implementation
//===================================================================
char * mli_debug_krn_conv2d_chw_fx16(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch3_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch4_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k2x2_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k2x2_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k3x3_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k3x3_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k4x4_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k4x4_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k5x5_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k5x5_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k6x6_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k6x6_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k7x7_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k7x7_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k5x5_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k5x5_ch1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x2_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x3_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k2x1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k3x1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1xn_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_knx1_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_ch1_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch3_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch4_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k1x1_ch8_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k2x2_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k2x2_ch1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k3x3_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx16_k3x3_ch1_krnpad(
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

char * mli_debug_krn_conv2d_chw_fx8(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch3_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch4_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k2x2_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k2x2_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k3x3_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k3x3_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k4x4_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k4x4_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k5x5_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k5x5_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k6x6_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k6x6_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k7x7_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k7x7_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x2_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x3_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k2x1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k3x1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1xn_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_knx1_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_ch1_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch3_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch4_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k1x1_ch8_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k2x2_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k2x2_ch1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k3x3_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8_k3x3_ch1_krnpad(
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

char * mli_debug_krn_conv2d_chw_fx8w16d(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch1_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch3_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch4_str1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k2x2_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k2x2_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k3x3_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k3x3_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k4x4_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k4x4_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k5x5_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k5x5_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k6x6_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k6x6_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k7x7_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k7x7_ch1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x2_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x3_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k2x1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k3x1_str1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1xn_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_knx1_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_ch1_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_str1(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch1_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch3_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch4_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k1x1_ch8_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k2x2_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k2x2_ch1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k3x3_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_k3x3_ch1_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_chw_fx8w16d_generic(
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

mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32_k3x3_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32_k5x5_krnpad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32_k3x3_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32_k5x5_nopad(
        const mli_tensor * in, 
        const mli_tensor * weights, 
        const mli_tensor * bias, 
        const mli_conv2d_cfg * cfg, 
        mli_tensor * out);

mli_status mli_krn_conv2d_nhwc_sa8_sa8_sa32_k1x1_nopad(
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


#ifdef __cplusplus
}
#endif
#endif //_MLI_KRN_CONV2D_SPEC_API_H_
