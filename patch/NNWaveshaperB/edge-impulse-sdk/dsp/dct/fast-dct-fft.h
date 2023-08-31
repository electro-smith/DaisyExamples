/*
 * Copyright (c) 2022 Project Nayuki. (MIT License)
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

#ifndef __FAST_DCT_FFT__H__
#define __FAST_DCT_FFT__H__


#include <stdbool.h>
#include <stddef.h>
#include "../kissfft/kiss_fft.h"

namespace ei {
namespace dct {

int transform(float vector[], size_t len);
int inverse_transform(float vector[], size_t len);

} // namespace dct
} // namespace ei

#endif  //!__FAST-DCT-FFT__H__