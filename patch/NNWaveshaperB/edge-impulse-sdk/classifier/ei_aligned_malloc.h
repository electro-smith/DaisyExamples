/*
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS
 * IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _EDGE_IMPULSE_ALIGNED_MALLOC_H_
#define _EDGE_IMPULSE_ALIGNED_MALLOC_H_

#include <assert.h>
#include "../porting/ei_classifier_porting.h"

#ifdef __cplusplus
namespace {
#endif // __cplusplus

/**
* Based on https://github.com/embeddedartistry/embedded-resources/blob/master/examples/c/malloc_aligned.c
*/

/**
* Simple macro for making sure memory addresses are aligned
* to the nearest power of two
*/
#ifndef align_up
#define align_up(num, align) \
	(((num) + ((align) - 1)) & ~((align) - 1))
#endif

//Number of bytes we're using for storing the aligned pointer offset
typedef uint16_t offset_t;
#define PTR_OFFSET_SZ sizeof(offset_t)

/**
* aligned_malloc takes in the requested alignment and size
*	We will call malloc with extra bytes for our header and the offset
*	required to guarantee the desired alignment.
*/
__attribute__((unused)) void * ei_aligned_calloc(size_t align, size_t size)
{
	void * ptr = NULL;

	//We want it to be a power of two since align_up operates on powers of two
	assert((align & (align - 1)) == 0);

	if(align && size)
	{
		/*
		 * We know we have to fit an offset value
		 * We also allocate extra bytes to ensure we can meet the alignment
		 */
		uint32_t hdr_size = PTR_OFFSET_SZ + (align - 1);
		void * p = ei_calloc(size + hdr_size, 1);

		if(p)
		{
			/*
			 * Add the offset size to malloc's pointer (we will always store that)
			 * Then align the resulting value to the arget alignment
			 */
			ptr = (void *) align_up(((uintptr_t)p + PTR_OFFSET_SZ), align);

			//Calculate the offset and store it behind our aligned pointer
			*((offset_t *)ptr - 1) = (offset_t)((uintptr_t)ptr - (uintptr_t)p);

		} // else NULL, could not malloc
	} //else NULL, invalid arguments

	return ptr;
}

/**
* aligned_free works like free(), but we work backwards from the returned
* pointer to find the correct offset and pointer location to return to free()
* Note that it is VERY BAD to call free() on an aligned_malloc() pointer.
*/
__attribute__((unused)) void ei_aligned_free(void * ptr)
{
	assert(ptr);

	/*
	* Walk backwards from the passed-in pointer to get the pointer offset
	* We convert to an offset_t pointer and rely on pointer math to get the data
	*/
	offset_t offset = *((offset_t *)ptr - 1);

	/*
	* Once we have the offset, we can get our original pointer and call free
	*/
	void * p = (void *)((uint8_t *)ptr - offset);
	ei_free(p);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _EDGE_IMPULSE_ALIGNED_MALLOC_H_
