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
#include <gtest/gtest.h>
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/kernels/test_util.h"
#include "tensorflow/lite/model.h"

namespace tflite {
namespace {

using ::testing::ElementsAreArray;

class BaseSquaredDifferenceOpModel : public SingleOpModel {
 public:
  BaseSquaredDifferenceOpModel(const TensorData& input1,
                               const TensorData& input2,
                               const TensorData& output) {
    input1_ = AddInput(input1);
    input2_ = AddInput(input2);
    output_ = AddOutput(output);
    SetBuiltinOp(BuiltinOperator_SQUARED_DIFFERENCE,
                 BuiltinOptions_SquaredDifferenceOptions,
                 CreateSquaredDifferenceOptions(builder_).Union());
    BuildInterpreter({GetShape(input1_), GetShape(input2_)});
  }

  int input1() { return input1_; }
  int input2() { return input2_; }

 protected:
  int input1_;
  int input2_;
  int output_;
};

class FloatSquaredDifferenceOpModel : public BaseSquaredDifferenceOpModel {
 public:
  using BaseSquaredDifferenceOpModel::BaseSquaredDifferenceOpModel;

  std::vector<float> GetOutput() { return ExtractVector<float>(output_); }
};

class IntegerSquaredDifferenceOpModel : public BaseSquaredDifferenceOpModel {
 public:
  using BaseSquaredDifferenceOpModel::BaseSquaredDifferenceOpModel;

  std::vector<int32_t> GetOutput() { return ExtractVector<int32_t>(output_); }
};

TEST(FloatSquaredDifferenceOpTest, FloatType_SameShape) {
  FloatSquaredDifferenceOpModel m({TensorType_FLOAT32, {1, 2, 2, 1}},
                                  {TensorType_FLOAT32, {1, 2, 2, 1}},
                                  {TensorType_FLOAT32, {}});
  m.PopulateTensor<float>(m.input1(), {-0.2, 0.2, -1.2, 0.8});
  m.PopulateTensor<float>(m.input2(), {0.5, 0.2, -1.5, 0.5});
  m.Invoke();
  EXPECT_THAT(m.GetOutput(),
              ElementsAreArray(ArrayFloatNear({0.49, 0.0, 0.09, 0.09})));
}

TEST(FloatSquaredDifferenceOpTest, FloatType_VariousInputShapes) {
  std::vector<std::vector<int>> test_shapes = {
      {6}, {2, 3}, {2, 1, 3}, {1, 3, 1, 2}};
  for (int i = 0; i < test_shapes.size(); ++i) {
    FloatSquaredDifferenceOpModel m({TensorType_FLOAT32, test_shapes[i]},
                                    {TensorType_FLOAT32, test_shapes[i]},
                                    {TensorType_FLOAT32, {}});
    m.PopulateTensor<float>(m.input1(), {-2.0, 0.2, 0.3, 0.8, 1.1, -2.0});
    m.PopulateTensor<float>(m.input2(), {1.0, 0.2, 0.6, 0.4, -1.0, -0.0});
    m.Invoke();
    EXPECT_THAT(
        m.GetOutput(),
        ElementsAreArray(ArrayFloatNear({9.0, 0.0, 0.09, 0.16, 4.41, 4.0})))
        << "With shape number " << i;
  }
}

TEST(FloatSquaredDifferenceOpTest, FloatType_WithBroadcast) {
  std::vector<std::vector<int>> test_shapes = {
      {6}, {2, 3}, {2, 1, 3}, {1, 3, 1, 2}};
  for (int i = 0; i < test_shapes.size(); ++i) {
    FloatSquaredDifferenceOpModel m(
        {TensorType_FLOAT32, test_shapes[i]},
        {TensorType_FLOAT32, {}},  // always a scalar
        {TensorType_FLOAT32, {}});
    m.PopulateTensor<float>(m.input1(), {-0.2, 0.2, 0.5, 0.8, 0.11, 1.1});
    m.PopulateTensor<float>(m.input2(), {0.1});
    m.Invoke();
    EXPECT_THAT(
        m.GetOutput(),
        ElementsAreArray(ArrayFloatNear({0.09, 0.01, 0.16, 0.49, 0.0001, 1.0})))
        << "With shape number " << i;
  }
}

TEST(IntegerSquaredDifferenceOpTest, IntegerType_SameShape) {
  IntegerSquaredDifferenceOpModel m({TensorType_INT32, {1, 2, 2, 1}},
                                    {TensorType_INT32, {1, 2, 2, 1}},
                                    {TensorType_INT32, {}});
  m.PopulateTensor<int32_t>(m.input1(), {-2, 2, -15, 8});
  m.PopulateTensor<int32_t>(m.input2(), {5, -2, -3, 5});
  m.Invoke();
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({49, 16, 144, 9}));
}

TEST(IntegerSquaredDifferenceOpTest, IntegerType_VariousInputShapes) {
  std::vector<std::vector<int>> test_shapes = {
      {6}, {2, 3}, {2, 1, 3}, {1, 3, 1, 2}};
  for (int i = 0; i < test_shapes.size(); ++i) {
    IntegerSquaredDifferenceOpModel m({TensorType_INT32, test_shapes[i]},
                                      {TensorType_INT32, test_shapes[i]},
                                      {TensorType_INT32, {}});
    m.PopulateTensor<int32_t>(m.input1(), {-20, 2, 3, 8, 11, -20});
    m.PopulateTensor<int32_t>(m.input2(), {1, 2, 6, 5, -5, -20});
    m.Invoke();
    EXPECT_THAT(m.GetOutput(), ElementsAreArray({441, 0, 9, 9, 256, 0}))
        << "With shape number " << i;
  }
}

TEST(IntegerSquaredDifferenceOpTest, IntegerType_WithBroadcast) {
  std::vector<std::vector<int>> test_shapes = {
      {6}, {2, 3}, {2, 1, 3}, {1, 3, 1, 2}};
  for (int i = 0; i < test_shapes.size(); ++i) {
    IntegerSquaredDifferenceOpModel m(
        {TensorType_INT32, test_shapes[i]},
        {TensorType_INT32, {}},  // always a scalar
        {TensorType_INT32, {}});
    m.PopulateTensor<int32_t>(m.input1(), {-20, 10, 7, 3, 1, 13});
    m.PopulateTensor<int32_t>(m.input2(), {3});
    m.Invoke();
    EXPECT_THAT(m.GetOutput(), ElementsAreArray({529, 49, 16, 0, 4, 100}))
        << "With shape number " << i;
  }
}

}  // namespace
}  // namespace tflite
