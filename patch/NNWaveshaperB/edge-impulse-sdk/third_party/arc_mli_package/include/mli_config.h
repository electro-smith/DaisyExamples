/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

/**
 * @file MLI Library Configuration header
 *
 * @brief This header defines MLI Library configuration 
 */

#ifndef _MLI_CONFIG_H_
#define _MLI_CONFIG_H_
/**
* Define Library build configuration options
*/

/**
* Concatenate primitive: Maximum number of tensors that might be concatenated.
*/
#define MLI_CONCAT_MAX_TENSORS (8)

/**
* Library Debug mode
*/
#define     DBG_MODE_RELEASE   (0) /*< No debug. Messages:OFF; Assertions:OFF; ReturnCodes: Always OK */
#define     DBG_MODE_RET_CODES (1) /*< Return codes mode. Messages:OFF; Assertions:OFF; ReturnCodes: Valid Return*/
#define     DBG_MODE_ASSERT    (2) /*< Assert. Messages:OFF; Assertions:ON; Extra Assertions:OFF; ReturnCodes: Valid Return */
#define     DBG_MODE_DEBUG     (3) /*< Debug. Messages:ON; Assertions:ON; Extra Assertions:OFF; ReturnCodes: Valid Return */
#define     DBG_MODE_FULL      (4) /*< Full Debug. Messages:ON; Assertions:ON; Extra Assertions:ON; ReturnCodes: Valid Return */

#ifndef MLI_DEBUG_MODE
#define MLI_DEBUG_MODE (DBG_MODE_RELEASE)
#endif

/**
* Define platform specific data
*/
#include <stdint.h>

#if defined (__CCAC__)

#include <arc/xy.h>

#ifdef __FXAPI__
#include <stdfix.h>
#else
#error "ARC FX Library (FXAPI) is a required dependency"
#endif

#endif // if defined (__CCAC__)


/*
* Define the platform (according to pre-defined macro or according to HW config)
* 1 - ARCV2DSP ISA
* 2 - ARCV2DSP ISA with XY Memory
* 3 - ARCV2DSP ISA with 64bit operands (HS Only)
*/

#if defined(V2DSP_XY) || ((defined __Xxy) && !(defined(V2DSP) || defined(V2DSP_WIDE)))
/* Platform with XY memory (EM9D or EM11D) */
#undef V2DSP_XY
#define ARC_PLATFORM (2)
#define ARC_PLATFORM_STR  "ARCv2DSP XY"

#elif defined(V2DSP_WIDE) || ((defined __Xdsp_wide) && !(defined(V2DSP) || defined(V2DSP_XY)))
/* Platform with wide DSP ISA (HS45D or HS47D) */
#undef V2DSP_WIDE
#define ARC_PLATFORM (3)
#define ARC_PLATFORM_STR  "ARCv2DSP Wide"

#elif defined(V2DSP) || ((defined(__Xdsp2) || defined(__Xdsp_complex)) && !(defined(V2DSP_XY) || defined(V2DSP_WIDE)))
/* Platform with DSP ISA (EM5D or EM7D) */
#undef V2DSP
#define ARC_PLATFORM (1)
#define ARC_PLATFORM_STR  "ARCv2DSP"

#else
#error "Target platform is undefined or defined incorrectly"
#endif

#define     V2DSP      (1)
#define     V2DSP_XY   (2)
#define     V2DSP_WIDE (3)

/*
* Re-define ML pointers for XY specific platform
*
* MLI_PTR is used for all the read pointers
* MLI_CONV_OUT_PTR is used for the output buffers of all weigths based kernels.
* this means all the kernels that perform a convolution like operation between inputs and weights.
* MLI_OUT_PTR is used for the output of all other kernels.
*/
#if (ARC_PLATFORM == V2DSP_XY)
#define MLI_PTR(p) __xy p *
#define MLI_PTR_IS_XY true
#define MLI_OUT_PTR(p) __xy p *
#define MLI_OUT_PTR_IS_XY true
#define MLI_CONV_OUT_PTR(p) p *
#define MLI_CONV_OUT_PTR_IS_XY false
#else
#define MLI_PTR(p) p *
#define MLI_PTR_IS_XY false
#define MLI_OUT_PTR(p) p *
#define MLI_OUT_PTR_IS_XY false
#define MLI_CONV_OUT_PTR(p) p *
#define MLI_CONV_OUT_PTR_IS_XY false
#endif

#endif // _MLI_CONFIG_H_
