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

#define FLATBUFFERS_LOCALE_INDEPENDENT 0
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <initializer_list>
#include <numeric>
#include <vector>

#include "edge-impulse-sdk/third_party/flatbuffers/include/flatbuffers/flexbuffers.h"  // from @flatbuffers
#include "edge-impulse-sdk/tensorflow/lite/c/common.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/compatibility.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/internal/tensor_ctypes.h"
#include "edge-impulse-sdk/tensorflow/lite/kernels/kernel_util.h"

#define FEATURE_TYPE float

namespace tflite {
namespace ops {
namespace custom {
namespace tree_ensemble_classifier {

struct OpDataTree {
  uint32_t num_leaf_nodes;
  uint32_t num_internal_nodes;
  uint32_t num_trees;
  const uint16_t* nodes_modes;
  const uint16_t* nodes_featureids;
  const float* nodes_values;
  const uint16_t* nodes_truenodeids;
  const uint16_t* nodes_falsenodeids;
  const float* nodes_weights;
  const uint8_t* nodes_classids;
  const uint16_t* tree_root_ids;
  const uint8_t* buffer_t;
  size_t buffer_length;
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {

  const uint8_t* buffer_t = reinterpret_cast<const uint8_t*>(buffer);
  const flexbuffers::Map& m = flexbuffers::GetRoot(buffer_t, length).AsMap();

  auto* data = new OpDataTree;

  data->buffer_t = buffer_t;
  data->buffer_length = length;

  data->num_leaf_nodes = m["num_leaf_nodes"].AsUInt32();
  data->num_internal_nodes = m["num_internal_nodes"].AsUInt32();
  data->num_trees = m["num_trees"].AsUInt32();

  data->nodes_modes = (uint16_t*)(m["nodes_modes"].AsBlob().data());
  data->nodes_featureids = (uint16_t*)(m["nodes_featureids"].AsBlob().data());
  data->nodes_values = (float*)(m["nodes_values"].AsBlob().data());
  data->nodes_truenodeids = (uint16_t*)(m["nodes_truenodeids"].AsBlob().data());
  data->nodes_falsenodeids = (uint16_t*)(m["nodes_falsenodeids"].AsBlob().data());
  data->nodes_weights = (float*)(m["nodes_weights"].AsBlob().data());
  data->nodes_classids = (uint8_t*)(m["nodes_classids"].AsBlob().data());
  data->tree_root_ids = (uint16_t*)(m["tree_root_ids"].AsBlob().data());

  return data;
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {

  const OpDataTree* data = static_cast<const OpDataTree*>(node->user_data);
  const flexbuffers::Map& m = flexbuffers::GetRoot(data->buffer_t, data->buffer_length).AsMap();

  // The OOB checks below are very important to prevent vulnerabilities where an adversary sends
  // us a malicious TFLite model, similar to: https://nvd.nist.gov/vuln/detail/CVE-2022-23560

  int num_nodes = data->num_leaf_nodes + data->num_internal_nodes;

  // Check that the tree root ids are valid.
  for (uint32_t i = 0; i < data->num_trees; i++) {
    TF_LITE_ENSURE_EQ(context, data->tree_root_ids[i] < num_nodes, true);
    TF_LITE_ENSURE_EQ(context, data->tree_root_ids[i] >= 0, true);
  }

  // Check that all node indices are valid
  for (uint32_t i = 0; i < data->num_internal_nodes; i++) {
    TF_LITE_ENSURE_EQ(context, data->nodes_truenodeids[i] < num_nodes, true);
    TF_LITE_ENSURE_EQ(context, data->nodes_truenodeids[i] >= 0, true);
    TF_LITE_ENSURE_EQ(context, data->nodes_falsenodeids[i] < num_nodes, true);
    TF_LITE_ENSURE_EQ(context, data->nodes_falsenodeids[i] >= 0, true);
  }

  // Check all node arrays have the same length
  TF_LITE_ENSURE_EQ(context, data->num_internal_nodes, m["nodes_featureids"].AsBlob().size());
  TF_LITE_ENSURE_EQ(context, data->num_internal_nodes, m["nodes_values"].AsBlob().size());
  TF_LITE_ENSURE_EQ(context, data->num_internal_nodes, m["nodes_truenodeids"].AsBlob().size());
  TF_LITE_ENSURE_EQ(context, data->num_internal_nodes, m["nodes_falsenodeids"].AsBlob().size());
  TF_LITE_ENSURE_EQ(context, data->num_leaf_nodes, m["nodes_weights"].AsBlob().size());
  TF_LITE_ENSURE_EQ(context, data->num_leaf_nodes, m["nodes_classids"].AsBlob().size());

  // Check data types are supported. Currently we only support one combination.
  TF_LITE_ENSURE_EQ(context, strncmp(m["tree_index_type"].AsString().c_str(), "uint16", 6), 0);
  TF_LITE_ENSURE_EQ(context, strncmp(m["node_value_type"].AsString().c_str(), "float32", 7), 0);
  TF_LITE_ENSURE_EQ(context, strncmp(m["class_index_type"].AsString().c_str(), "uint8", 5), 0);
  TF_LITE_ENSURE_EQ(context, strncmp(m["class_weight_type"].AsString().c_str(), "float32", 7), 0);
  TF_LITE_ENSURE_EQ(context, strncmp(m["equality_operator"].AsString().c_str(), "leq", 3), 0);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 1);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);
  const TfLiteTensor* input = GetInput(context, node, 0);
  TF_LITE_ENSURE(context, input != nullptr);
  TF_LITE_ENSURE(context, NumDimensions(input) == 2);
  TfLiteTensor* output = GetOutput(context, node, 0);
  TF_LITE_ENSURE(context, output != nullptr);

  int input_width = SizeOfDimension(input, 1);
  int output_width = SizeOfDimension(output, 1);

  // Check that all indices into the input/output tensor are valid
  for (uint32_t i = 0; i < data->num_internal_nodes; i++) {
    TF_LITE_ENSURE(context, data->nodes_featureids[i] < input_width);
    TF_LITE_ENSURE(context, data->nodes_featureids[i] >= 0);
    if (data->nodes_modes[i] == 0) {
      TF_LITE_ENSURE(context, data->nodes_classids[i] < output_width);
      TF_LITE_ENSURE(context, data->nodes_classids[i] >= 0);
    }
  }

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {

  const OpDataTree* data = static_cast<const OpDataTree*>(node->user_data);
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, 0, &input));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context, GetOutputSafe(context, node, 0, &output));

  float* output_data = GetTensorData<float>(output);
  memset(output_data, 0, GetTensorShape(output).FlatSize() * sizeof(float));

  for (uint32_t i = 0; i < data->num_trees; i++) {
    uint16_t ix = data->tree_root_ids[i];
    while (ix < data->num_internal_nodes) {
      if (input->data.f[data->nodes_featureids[ix]] <= data->nodes_values[ix]) {
        ix = data->nodes_truenodeids[ix];
      } else {
        ix = data->nodes_falsenodeids[ix];
      }
    }
    ix -= data->num_internal_nodes;
    output->data.f[data->nodes_classids[ix]] += data->nodes_weights[ix];
  }

  return kTfLiteOk;
}

}  // namespace

TfLiteRegistration* Register_TREE_ENSEMBLE_CLASSIFIER() {

  static TfLiteRegistration r = {
      tree_ensemble_classifier::Init, nullptr,
      tree_ensemble_classifier::Prepare, tree_ensemble_classifier::Eval};
  return &r;
}

TfLiteRegistration* Register_TFLITE_TREE_ENSEMBLE_CLASSIFIER() {
  return Register_TREE_ENSEMBLE_CLASSIFIER();
}

}  // namespace custom
}  // namespace ops
}  // namespace tflite
