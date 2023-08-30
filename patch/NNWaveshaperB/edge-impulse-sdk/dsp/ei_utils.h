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
#ifndef __EI_UTILS__H__
#define __EI_UTILS__H__

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

// Stringify
#define ei_xstr(a) ei_str(a)
#define ei_str(a) #a

// Bit manipulation

//Set bit y (0-indexed) of x to '1' by generating a a mask with a '1' in the proper bit location and ORing x with the mask.
#define SET_BIT_POS(x,y) (x |= (1 << y))

//Set bit y (0-indexed) of x to '0' by generating a mask with a '0' in the y position and 1's elsewhere then ANDing the mask with x.
#define CLEAR_BIT_POS(x,y) (x &= ~(1<< y))

//Return '1' if the bit value at position y within x is '1' and '0' if it's 0 by ANDing x with a bit mask where the bit in y's position is '1' and '0' elsewhere and comparing it to all 0's.  Returns '1' in least significant bit position if the value of the bit is '1', '0' if it was '0'.
#define TEST_BIT_POS(x,y) ((0u == (x & (1<<y)))?0u:1u)

//Toggle bit y (0-index) of x to the inverse: '0' becomes '1', '1' becomes '0' by XORing x with a bitmask where the bit in position y is '1' and all others are '0'.
#define TOGGLE_BIT_POS(x,y) (x ^= (1<<y))

// Set the flag bits in word.
#define 	SET_BIT_MASK(y, flag)   ( y |= (flag) )

// Clear the flag bits in word.
#define 	CLEAR_BIT_MASK(y, flag)   ( y &= ~(flag) )

// Flip the flag bits in word.
#define 	TOGGLE_BIT_MASK(y, flag)   ( y ^= (flag) )

// Test whether all the flag bits in word are set.
#define 	TEST_BIT_MASK(y, flag)   ( ((y)&(flag)) == (flag) )

#define EI_TRY(x) { auto res = (x); if(res != EIDSP_OK) { return res; } }

#endif  //!__EI_UTILS__H__