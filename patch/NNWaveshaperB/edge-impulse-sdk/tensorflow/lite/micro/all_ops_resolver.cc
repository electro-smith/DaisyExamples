/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include "edge-impulse-sdk/tensorflow/lite/micro/all_ops_resolver.h"

#include "edge-impulse-sdk/tensorflow/lite/micro/kernels/micro_ops.h"

namespace tflite {

AllOpsResolver::AllOpsResolver() {
  // Please keep this list of Builtin Operators in alphabetical order.
  AddAbs();
  AddAdd();
  AddAddN();
  AddArgMax();
  AddArgMin();
  AddAveragePool2D();
  AddBatchMatMul();
  AddBatchToSpaceNd();
  AddCeil();
  AddComplexAbs();
  AddConcatenation();
  AddConv2D();
  AddCos();
  AddDepthwiseConv2D();
  AddDequantize();
  // AddDetectionPostprocess();
  AddDiv();
  AddElu();
  AddEqual();
  AddEthosU();
  AddExp();
  AddExpandDims();
  AddFloor();
  AddFullyConnected();
#ifndef TF_LITE_STATIC_MEMORY
  AddGather();
#endif // TF_LITE_STATIC_MEMORY
  AddGreater();
  AddGreaterEqual();
  AddHardSwish();
  AddImag();
  AddL2Normalization();
  AddL2Pool2D();
  AddLeakyRelu();
  AddLess();
  AddLessEqual();
  AddLog();
  AddLogicalAnd();
  AddLogicalNot();
  AddLogicalOr();
  AddLogistic();
  AddLogSoftmax();
  AddMaxPool2D();
  AddMaximum();
  AddMean();
  AddMinimum();
  AddMul();
  AddNeg();
  AddNotEqual();
  AddPack();
  AddPad();
  AddPadV2();
  AddPrelu();
  AddQuantize();
  AddReal();
  AddReduceMax();
  AddReduceMin();
  AddRelu();
  AddRelu6();
  AddReshape();
  AddResizeNearestNeighbor();
  AddRfft2D();
  AddRound();
  AddRsqrt();
#ifndef TF_LITE_STATIC_MEMORY
  AddSelect();
  AddSelectV2();
#endif // TF_LITE_STATIC_MEMORY
  AddShape();
  AddSin();
  AddSlice();
  AddSoftmax();
  AddSpaceToBatchNd();
  AddSplit();
  AddSplitV();
  AddSqrt();
  AddSquare();
  AddSquaredDifference();
  AddSqueeze();
  AddStridedSlice();
  AddSub();
  AddSum();
  AddSvdf();
  AddTanh();
  AddTranspose();
  AddTransposeConv();
  AddTreeEnsembleClassifier();
  AddUnpack();
}

}  // namespace tflite
