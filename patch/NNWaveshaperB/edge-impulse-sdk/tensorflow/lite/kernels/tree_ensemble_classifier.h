/* Copyright 2023 Edge Impulse Inc. All Rights Reserved.

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

#ifndef TENSORFLOW_LITE_MICRO_KERNELS_TREE_ENSEMBLE_CLASSIFIER_H_
#define TENSORFLOW_LITE_MICRO_KERNELS_TREE_ENSEMBLE_CLASSIFIER_H_

#include "edge-impulse-sdk/tensorflow/lite/c/common.h"

namespace tflite {
namespace ops {
namespace custom {

TfLiteRegistration* Register_TREE_ENSEMBLE_CLASSIFIER();

}
}
}  // namespace tflite

#endif  // TENSORFLOW_LITE_MICRO_KERNELS_TREE_ENSEMBLE_CLASSIFIER_H_
