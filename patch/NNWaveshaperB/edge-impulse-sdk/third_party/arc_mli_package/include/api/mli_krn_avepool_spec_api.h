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

#ifndef _MLI_KRN_AVEPOOL_SPEC_API_H_
#define _MLI_KRN_AVEPOOL_SPEC_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mli_types.h"

//===================================================================
// AvePooling specialization kernels implementation
//===================================================================
char * mli_debug_krn_avepool_chw_fx16(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

mli_status mli_krn_avepool_chw_fx16_k2x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k3x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k4x4_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k5x5_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k6x6_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k7x7_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k8x8_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k9x9_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k10x10_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k2x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k3x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k4x4_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k5x5_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k6x6_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k7x7_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k8x8_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k9x9_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k10x10_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k1xn_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k1x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k1x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_knx1_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k2x1_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k3x1_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k1xn_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k1x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k1x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_knx1_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k2x1_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_k3x1_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx16_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
char * mli_debug_krn_avepool_chw_fx8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

mli_status mli_krn_avepool_chw_fx8_k2x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k3x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k4x4_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k5x5_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k6x6_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k7x7_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k8x8_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k9x9_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k10x10_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k2x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k3x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k4x4_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k5x5_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k6x6_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k7x7_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k8x8_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k9x9_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k10x10_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k1xn_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k1x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k1x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_knx1_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k2x1_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k3x1_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k1xn_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k1x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k1x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_knx1_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k2x1_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_k3x1_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_chw_fx8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
char * mli_debug_krn_avepool_hwc_fx16(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

mli_status mli_krn_avepool_hwc_fx16_k2x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx16_k3x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx16_k2x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx16_k3x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx16_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
char * mli_debug_krn_avepool_hwc_fx8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

mli_status mli_krn_avepool_hwc_fx8_k2x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx8_k3x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx8_k2x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx8_k3x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_fx8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
char * mli_debug_krn_avepool_hwc_sa8(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

mli_status mli_krn_avepool_hwc_sa8_k2x2_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_sa8_k3x3_nopad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_sa8_k2x2_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_sa8_k3x3_krnpad(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);
mli_status mli_krn_avepool_hwc_sa8_generic(const mli_tensor * in, const mli_pool_cfg * cfg, mli_tensor * out);

#ifdef __cplusplus
}
#endif
#endif //_MLI_KRN_AVEPOOL_SPEC_API_H_
