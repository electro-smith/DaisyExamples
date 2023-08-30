/*
* Copyright 2019-2020, Synopsys, Inc.
* All rights reserved.
*
* This source code is licensed under the BSD-3-Clause license found in
* the LICENSE file in the root directory of this source tree.
*
*/

/**
 * @file MLI Data Move API
 *
 * @brief This header includes declarations for data movement functions
 */

#ifndef _MLI_MOV_API_H_
#define _MLI_MOV_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "mli_types.h"

typedef struct _mli_mov_cfg {
    uint32_t offset[MLI_MAX_RANK];
    uint32_t size[MLI_MAX_RANK]; // if zero, compute from input and other parameters
    uint32_t sub_sample_step[MLI_MAX_RANK];
    uint32_t dst_offset[MLI_MAX_RANK];
    int32_t  dst_mem_stride[MLI_MAX_RANK]; // if zero, compute from input and other parameters
    uint8_t  perm_dim[MLI_MAX_RANK];
    uint8_t  padding_pre[MLI_MAX_RANK];
    uint8_t  padding_post[MLI_MAX_RANK];
} mli_mov_cfg_t;

typedef enum _mli_mov_state {
    MLI_MOV_STATE_INVALID = 0,
    MLI_MOV_STATE_OPEN,
    MLI_MOV_STATE_PREPARED,
    MLI_MOV_STATE_DMA_CONFIGURED,
    MLI_MOV_STATE_DMA_RUNNING,
    MLI_MOV_STATE_DONE
} mli_mov_state;

typedef struct _mli_mov_handle_t {
    int dma_ch;
    int num_ch;
    mli_mov_state state;
} mli_mov_handle_t;

//---------------------------------------------------------------------
// Synchronous data movement functions
//---------------------------------------------------------------------

/** 
 * @brief Synchronous copy from src tensor to dst tensor
 *
 * @detail This function will perform a data copy from the src tensor to the dst tensor
 * according to the settings in the cfg struct.
 * The destination tensor needs to contain a valid pointer to a large enough buffer.
 * the size of this buffer needs to be specified in the capacity field of the dst tensor.
 * the other fields of the dst tensor will be filled by the copy function.
 *
 * The function will return once the complete data transfer is finished.
 *
 * @param src  [I] pointer to source tensor.
 * @param dst  [I] pointer to destination tensor.
 * @param cfg  [I] pointer to config struct
 *
 * @return MLI status code
 */
mli_status
mli_mov_tensor_sync(const mli_tensor* src, const mli_mov_cfg_t* cfg, mli_tensor* dst);


//---------------------------------------------------------------------
// Asynchronous data movement functions
//---------------------------------------------------------------------

/** 
 * @brief Prepare asynchronous copy from src to dst
 *
 * @detail This function will prepare a data copy from the src tensor to the dst tensor
 * according to the settings in the cfg struct.
 * The destination tensor needs to contain a valid pointer to a large enough buffer.
 * the size of this buffer needs to be specified in the capacity field of the dst tensor.
 * the other fields of the dst tensor will be filled by the copy function.
 * The handle needs to be obtained using the mli_mov_acquire_handle() function.
 *
 * The function returns after the transfer has been prepared. It still needs to be started
 * by the mli_mov_start() function.
 *
 * @param h    [I] pointer to a handle for an available dma channel.
 * @param src  [I] pointer to source tensor.
 * @param dst  [I] pointer to destination tensor.
 * @param cfg  [I] pointer to config struct
 *
 * @return MLI status code
 */
mli_status
mli_mov_prepare(mli_mov_handle_t* h, const mli_tensor* src, const mli_mov_cfg_t* cfg, mli_tensor* dst);

/** 
 * @brief Register a callback for a datatransfer
 *
 * @detail This function will register a callback function that will be called after
 * the data transfer has been completed. Note that the callback needs to be registered before the
 * transfer has been started. otherwise in case of a fast transfer it could happen that the transfer
 * finished before the callback got registered.
 * the callback function takes one parameter, and the value of cookie is passed to the callback function.
 * The handle needs to be obtained using the mli_mov_acquire_handle() function.
 *
 * Registration of a callback function is optional.
 *
 * @param h    [I] pointer to a handle for an available dma channel.
 * @param cb   [I] function pointer to a callback.
 * @param cookie [I] this parameter will be passed to the callback function.
 *
 * @return MLI status code
 */
mli_status
mli_mov_registercallback(mli_mov_handle_t* h, void (*cb)(int32_t), int32_t cookie);

/** 
 * @brief Start asynchronous copy from src to dst
 *
 * @detail This function will start the data copy from the src tensor to the dst tensor
 * as prepared by the prepare function.
 * Before this function is called the mli_mov_prepare() has to be called.
 *
 * The function returns after the transfer has been started. A callback or wait for done
 * can be used to synchronize on the transfer complete
 *
 * @param h    [I] pointer to a handle for an available dma channel.
 * @param src  [I] pointer to source tensor.
 * @param dst  [I] pointer to destination tensor.
 * @param cfg  [I] pointer to config struct
 *
 * @return MLI status code
 */
mli_status
mli_mov_start(mli_mov_handle_t* h, const mli_tensor* src, const mli_mov_cfg_t* cfg, mli_tensor* dst);

/** 
 * @brief Polling function to detect if transfer has completed
 *
 * @detail This function will return true when the transfer is completed, and false in all
 * other cases
 *
 * @param h    [I] pointer to a handle for an available dma channel.
 *
 * @return bool transfer is done.
 */
bool
mli_mov_isdone(mli_mov_handle_t* h);

/** 
 * @brief Synchronize to transfer complete
 *
 * @detail This function will do active polling and return after the transfer has completed.
 *
 * @param h    [I] pointer to a handle for an available dma channel.
 *
 * @return MLI status code
 */
mli_status
mli_mov_wait(mli_mov_handle_t* h);


//---------------------------------------------------------------------
// functions to set available resources (e.g. dma channels)
//---------------------------------------------------------------------
/** 
 * @brief set dma channels that can be used by mli_mov functions
 *
 * @detail This function is used to set a pool of the dma channels
 * that can be used by the mli_mov functions.
 * These channels should not be used by other functions.
 * the acquire and release functions can be used to obtain channels from this pool
 *
 * @param ch_offset  [I] first dma channel that can by used
 * @param num_ch     [I] number of dma channels that can be used
 *
 * @return MLI status code
 */
mli_status
mli_mov_set_num_dma_ch(int ch_offset, int num_ch);

/** 
 * @brief Acquire dma channel(s)
 *
 * @detail This function is used to obtain one or more dma channels from the pool
 *
 * @param num_ch [I] number of requested dma channels
 * @param h      [O] pointer a handle that will be filled with the dma channel offset that can be used.
 *
 * @return MLI status code
 */
mli_status
mli_mov_acquire_handle(int num_ch, mli_mov_handle_t* h);

/** 
 * @brief Release dma channle(s)
 *
 * @detail This function will release the dma channels from the handle h back to the pool.
 *
 * @param h    [I] pointer to a handle for an available dma channel.
 *
 * @return MLI status code
 */
mli_status
mli_mov_release_handle(mli_mov_handle_t* h);


//---------------------------------------------------------------------
// Helper functions to fill mli_mov_cfg_t
//---------------------------------------------------------------------

/** 
 * @brief Construction of cfg struct for full tensor copy
 *
 * @detail This function will fill the cfg struct with the values needed for a full tensor copy
 * it will put all the other fields to a neutral value.
 *
 * @param cfg  [O] pointer to config struct
 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_copy(mli_mov_cfg_t* cfg);

/** 
 * @brief Construction of cfg struct for slicing
 *
 * @detail This function will fill the cfg struct with the values needed for copying a slice
 * from the source to the destination tensor.
 *
 * @param cfg  [O] pointer to config struct
 * @param offsets [I] array of size MLI_MAX_RANK that contains the top left coordinate of the slice
 * @param sizes   [I] array of size MLI_MAX_RANK that contains the size of the slice
 * @param dst_mem_stride [I] array of size MLI_MAX_RANK that contains the number of elements to the next dimension in the destination tensor

 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_slice(mli_mov_cfg_t* cfg, int* offsets, int* sizes, int* dst_mem_stride);

/** 
 * @brief Construction of cfg struct for concatenation
 *
 * @detail This function will fill the cfg struct with the values needed for copying a complete tensor
 * into a larger tensor at a specified position.
 *
 * @param cfg  [O] pointer to config struct
 * @param dst_offsets [I] array of size MLI_MAX_RANK that contains the top left coordinate in the dst tensor where the src needs to be copied
 * @param dst_mem_stride [I] array of size MLI_MAX_RANK that contains the number of elements to the next dimension in the destination tensor

 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_concat(mli_mov_cfg_t* cfg, int* dst_offsets, int* dst_mem_stride);

/** 
 * @brief Construction of cfg struct for subsampling
 *
 * @detail This function will fill the cfg struct with the values needed for subsampling a tensor
 * a subsample step of 3 means that every third sample is copied to the output.
 *
 * @param cfg  [O] pointer to config struct
 * @param subsample_step [I] array of size MLI_MAX_RANK that contains the subsample step for each dimension.
 * @param dst_mem_stride [I] array of size MLI_MAX_RANK that contains the number of elements to the next dimension in the destination tensor

 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_subsample(mli_mov_cfg_t* cfg, int* sub_sample_step, int* dst_mem_stride);

/** 
 * @brief Construction of cfg struct for permutaion or transposing a tensor
 *
 * @detail This function will fill the cfg struct with the values needed for reordering the order of the dimensions in a tensor.
 *
 * @param cfg  [O] pointer to config struct
 * @param perm_dim [I] array of size MLI_MAX_RANK that contains the index of the source dimension for each output dimension
 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_permute(mli_mov_cfg_t* cfg, uint8_t* perm_dim);

/** 
 * @brief Construction of cfg struct for padding
 *
 * @detail This function will fill the cfg struct with the values needed adding zero padding to a tensor in CHW layout.
 *
 * @param cfg  [O] pointer to config struct
 * @param padleft [I] amount of pixels to padd to the left
 * @param padright [I] amount of pixels to padd to the right
 * @param padtop [I] amount of pixels to padd to the top
 * @param padbot [I] amount of pixels to padd to the bottom
 * @param dst_mem_stride [I] array of size MLI_MAX_RANK that contains the number of elements to the next dimension in the destination tensor
 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_padding2d_chw(mli_mov_cfg_t* cfg, uint8_t padleft, uint8_t padright, uint8_t padtop, uint8_t padbot, int* dst_mem_stride);

/** 
 * @brief Construction of cfg struct for padding
 *
 * @detail This function will fill the cfg struct with the values needed adding zero padding to a tensor in HWC layout.
 *
 * @param cfg  [O] pointer to config struct
 * @param padleft [I] amount of pixels to padd to the left
 * @param padright [I] amount of pixels to padd to the right
 * @param padtop [I] amount of pixels to padd to the top
 * @param padbot [I] amount of pixels to padd to the bottom
 * @param dst_mem_stride [I] array of size MLI_MAX_RANK that contains the number of elements to the next dimension in the destination tensor
 *
 * @return MLI status code
 */
mli_status
mli_mov_cfg_for_padding2d_hwc(mli_mov_cfg_t* cfg, uint8_t padleft, uint8_t padright, uint8_t padtop, uint8_t padbot, int* dst_mem_stride);

/** 
 * @brief Construction of cfg struct
 *
 * @detail This function will fill the cfg struct
 *
 * @param cfg  [O] pointer to config struct
 * @param offsets [I] array of size MLI_MAX_RANK that contains the top left coordinate of the slice
 * @param sizes   [I] array of size MLI_MAX_RANK that contains the size of the slice
 * @param subsample_step [I] array of size MLI_MAX_RANK that contains the subsample step for each dimension.
 * @param dst_offsets [I] array of size MLI_MAX_RANK that contains the top left coordinate in the dst tensor where the src needs to be copied
 * @param dst_mem_stride [I] array of size MLI_MAX_RANK that contains the number of elements to the next dimension in the destination tensor
 * @param perm_dim [I] array of size MLI_MAX_RANK that contains the index of the source dimension for each output dimension
 * @param padleft [I] amount of pixels to padd to the left
 * @param padright [I] amount of pixels to padd to the right
 * @param padtop [I] amount of pixels to padd to the top
 * @param padbot [I] amount of pixels to padd to the bottom
 *
 * @return MLI status code
 */

mli_status
mli_mov_cfg_all(
    mli_mov_cfg_t* cfg,
    int* offsets,
    int* sizes,
    int* subsample_step,
    int* dst_offsets,
    int* dst_mem_stride,
    uint8_t* perm_dim,
    uint8_t padleft,
    uint8_t padright,
    uint8_t padtop,
    uint8_t padbot);


#ifdef __cplusplus
}
#endif

#endif //_MLI_MOV_API_H_
