// Copyright 2019 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cstdarg>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "gmock/gmock.h"
#include "spvcir_pass.h"
#include "test/opt/assembly_builder.h"
#include "test/opt/pass_fixture.h"
#include "test/opt/pass_utils.h"

namespace spvtools {
namespace opt {
namespace {

using ::testing::HasSubstr;
using ::testing::MatchesRegex;

// For a given result Id returns the previously constructed instruction.
template <typename T>
T& get(uint32_t id, spirv_cross::ParsedIR* ir) {
  return spirv_cross::variant_get<T>(ir->ids[id]);
}

// For a given result Id returns the instruction if it was previously
// constructed and had the same result Type otherwise returns nullptr.
template <typename T>
T* maybe_get(uint32_t id, spirv_cross::ParsedIR* ir) {
  if (id >= ir->ids.size())
    return nullptr;
  else if (ir->ids[id].get_type() == static_cast<spirv_cross::Types>(T::type))
    return &get<T>(id, ir);
  else
    return nullptr;
}

std::string SelectiveJoin(const std::vector<const char*>& strings,
                          const std::function<bool(const char*)>& skip_dictator,
                          char delimiter = '\n') {
  std::ostringstream oss;
  for (const auto* str : strings) {
    if (!skip_dictator(str)) oss << str << delimiter;
  }
  return oss.str();
}

std::string JoinAllInsts(const std::vector<const char*>& insts) {
  return SelectiveJoin(insts, [](const char*) { return false; });
}

bool createSpvcIr(spirv_cross::ParsedIR* ir, std::string text) {
  std::vector<uint32_t> binary;
  std::unique_ptr<IRContext> context =
      BuildModule(SPV_ENV_UNIVERSAL_1_1, nullptr, text,
                  SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
  if (!context) return false;
  context->module()->ToBinary(&binary, false);
  ir->spirv =
      std::vector<uint32_t>(binary.data(), binary.data() + binary.size());
  return true;
}

class SpvcIrParsingTest : public PassTest<::testing::Test> {
 protected:
  void SetUp() override {
    input_ = R"(
              OpCapability Shader
         %5 = OpExtInstImport "GLSL.std.450"
              OpCapability VulkanMemoryModelKHR
              OpExtension "SPV_KHR_vulkan_memory_model"
              OpMemoryModel Logical VulkanKHR
              OpEntryPoint Vertex %1 "shader"
         %2 = OpTypeVoid
         %3 = OpTypeFunction %2
         %1 = OpFunction %2 None %3
         %4 = OpLabel
              OpReturn
              OpFunctionEnd
    )";

    before_ = {
        // clang-format off
              "OpCapability Shader",
              "OpCapability VulkanMemoryModelKHR",
              "OpExtension \"SPV_KHR_vulkan_memory_model\"",
              "OpMemoryModel Logical VulkanKHR"
        // clang-format on
    };

    after_ = {
        // clang-format off
             "OpEntryPoint Vertex %1 \"shader\"",
        "%2 = OpTypeVoid",
        "%3 = OpTypeFunction %2",
        "%1 = OpFunction %2 None %3",
        "%4 = OpLabel",
             "OpReturn",
             "OpFunctionEnd"
        // clang-format on
    };

    SetAssembleOptions(SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
    std::vector<uint32_t> binary;
    std::unique_ptr<IRContext> context =
        BuildModule(SPV_ENV_UNIVERSAL_1_1, nullptr, input_,
                    SPV_TEXT_TO_BINARY_OPTION_PRESERVE_NUMERIC_IDS);
    assert(context && "context");
    context->module()->ToBinary(&binary, false);
    ir_.spirv =
        std::vector<uint32_t>(binary.data(), binary.data() + binary.size());
  }
  std::string input_;
  spirv_cross::ParsedIR ir_;
  std::vector<const char*> before_;
  std::vector<const char*> after_;
};

TEST_F(SpvcIrParsingTest, OpExtInstImportInsruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_ext = maybe_get<spirv_cross::SPIRExtension>(5, &ir_);
  ASSERT_NE(spir_ext, nullptr);
  EXPECT_EQ(spir_ext->ext, spirv_cross::SPIRExtension::GLSL);
}

TEST_F(SpvcIrParsingTest, OpCapabilityInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  ASSERT_EQ(ir_.declared_capabilities.size(), 2);
  EXPECT_EQ(ir_.declared_capabilities[0], spv::Capability::CapabilityShader);
  EXPECT_EQ(ir_.declared_capabilities[1],
            spv::Capability::CapabilityVulkanMemoryModelKHR);
}

TEST_F(SpvcIrParsingTest, OpExtensionInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  ASSERT_EQ(ir_.declared_extensions.size(), 1);
  EXPECT_EQ(ir_.declared_extensions[0], "SPV_KHR_vulkan_memory_model");
}

TEST_F(SpvcIrParsingTest, OpMemoryModelInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  EXPECT_EQ(ir_.addressing_model, spv::AddressingModel::AddressingModelLogical);
  EXPECT_EQ(ir_.memory_model, spv::MemoryModel::MemoryModelVulkanKHR);
}

TEST_F(SpvcIrParsingTest, OpEntryPointInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  ASSERT_EQ(ir_.entry_points.size(), 1);
  const auto functionId = 1;
  const auto entry = *ir_.entry_points.begin();
  EXPECT_EQ(entry.first, static_cast<spirv_cross::FunctionID>(functionId));
  EXPECT_EQ(entry.second.orig_name, "shader");
  EXPECT_EQ(entry.second.model, spv::ExecutionModelVertex);
  EXPECT_EQ(entry.second.self,
            static_cast<spirv_cross::FunctionID>(functionId));
  EXPECT_EQ(ir_.meta[functionId].decoration.alias, "shader");
}

TEST_F(SpvcIrParsingTest, OpTypeVoidInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func_type = maybe_get<spirv_cross::SPIRType>(2, &ir_);
  ASSERT_NE(func_type, nullptr);
  EXPECT_EQ(func_type->basetype, spirv_cross::SPIRType::Void);
}

TEST_F(SpvcIrParsingTest, OpTypeBoolInstruction) {
  const std::vector<const char*> middle = {"%25 = OpTypeBool",
                                           "%27 = OpTypeFunction %25"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto bool_type = maybe_get<spirv_cross::SPIRType>(25, &ir);
  ASSERT_NE(bool_type, nullptr);
  EXPECT_EQ(bool_type->basetype, spirv_cross::SPIRType::Boolean);
  EXPECT_EQ(bool_type->width, 1);
}

TEST_F(SpvcIrParsingTest, OpTypeFloatInstruction16) {
  const std::vector<const char*> middle = {"%25 = OpTypeFloat 16",
                                           "%27 = OpTypeFunction %25"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto bool_type = maybe_get<spirv_cross::SPIRType>(25, &ir);
  ASSERT_NE(bool_type, nullptr);
  EXPECT_EQ(bool_type->basetype, spirv_cross::SPIRType::Half);
  EXPECT_EQ(bool_type->width, 16);
}

TEST_F(SpvcIrParsingTest, OpTypeFloatInstruction32) {
  const std::vector<const char*> middle = {"%25 = OpTypeFloat 32",
                                           "%27 = OpTypeFunction %25"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto bool_type = maybe_get<spirv_cross::SPIRType>(25, &ir);
  ASSERT_NE(bool_type, nullptr);
  EXPECT_EQ(bool_type->basetype, spirv_cross::SPIRType::Float);
  EXPECT_EQ(bool_type->width, 32);
}

TEST_F(SpvcIrParsingTest, OpTypeFloatInstruction64) {
  const std::vector<const char*> middle = {"%25 = OpTypeFloat 64",
                                           "%27 = OpTypeFunction %25"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto bool_type = maybe_get<spirv_cross::SPIRType>(25, &ir);
  ASSERT_NE(bool_type, nullptr);
  EXPECT_EQ(bool_type->basetype, spirv_cross::SPIRType::Double);
  EXPECT_EQ(bool_type->width, 64);
}

TEST_F(SpvcIrParsingTest, OpTypeVectorInstruction) {
  const std::vector<const char*> middle = {"%6 = OpTypeFloat 32",
                                           "%7 = OpTypeVector %6 4"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto vector_type = maybe_get<spirv_cross::SPIRType>(7, &ir);
  ASSERT_NE(vector_type, nullptr);
  EXPECT_EQ(vector_type->basetype, spirv_cross::SPIRType::Float);
  EXPECT_EQ(vector_type->vecsize, 4);
  EXPECT_EQ(vector_type->self, static_cast<spirv_cross::TypeID>(7));
  EXPECT_EQ(vector_type->parent_type, static_cast<spirv_cross::TypeID>(6));
}

TEST_F(SpvcIrParsingTest, OpTypeMatrixInstruction) {
  const std::vector<const char*> middle = {"%6 = OpTypeFloat 32",
                                           "%8 = OpTypeVector %6 4",
                                           "%7 = OpTypeMatrix %8 4"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto matrix_type = maybe_get<spirv_cross::SPIRType>(7, &ir);
  ASSERT_NE(matrix_type, nullptr);
  EXPECT_EQ(matrix_type->columns, 4);
  EXPECT_EQ(matrix_type->self, static_cast<spirv_cross::TypeID>(7));
  EXPECT_EQ(matrix_type->parent_type, static_cast<spirv_cross::TypeID>(8));
}

TEST_F(SpvcIrParsingTest, OpTypeImageInstruction) {
  const std::vector<const char*> cap = {"OpCapability SampledBuffer"};
  const std::vector<const char*> middle = {
      // clang-format off
     "%5 = OpTypeInt 32 0",
     "%6 = OpTypeImage %5 Buffer 0 0 0 2 R32ui"
      // clang format on
  };
  std::string spirv = JoinAllInsts(Concat(cap,(Concat(Concat(before_, middle), after_))));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto image_type = maybe_get<spirv_cross::SPIRType>(6, &ir);
  ASSERT_NE(image_type, nullptr);
  EXPECT_EQ(image_type->image.type, static_cast<spirv_cross::TypeID>(5));
  EXPECT_EQ(image_type->image.dim, spv::Dim::DimBuffer);
  EXPECT_EQ(image_type->image.depth, 0);
  EXPECT_EQ(image_type->image.arrayed, 0);
  EXPECT_EQ(image_type->image.ms, 0);
  EXPECT_EQ(image_type->image.sampled, 2);
  EXPECT_EQ(image_type->image.format, spv::ImageFormat::ImageFormatR32ui);

}

TEST_F(SpvcIrParsingTest, OpTypeSampledImageInstruction) {
  const std::vector<const char*> cap = {"OpCapability SampledBuffer"};
  const std::vector<const char*> middle = {
      // clang-format off
     "%5 = OpTypeInt 32 0",
     "%6 = OpTypeImage %5 Buffer 0 0 0 2 R32ui",
     "%18 = OpTypeSampledImage %6",
      // clang-format on
  };
  std::string spirv =
      JoinAllInsts(Concat(cap, (Concat(Concat(before_, middle), after_))));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto sampledImage_type = maybe_get<spirv_cross::SPIRType>(18, &ir);
  ASSERT_NE(sampledImage_type, nullptr);
  EXPECT_EQ(sampledImage_type->basetype, spirv_cross::SPIRType::SampledImage);
  EXPECT_EQ(sampledImage_type->self, static_cast<spirv_cross::TypeID>(18));
}

TEST_F(SpvcIrParsingTest, OpTypeSamplerInstruction) {
  const std::vector<const char*> cap = {"OpCapability SampledBuffer"};
  const std::vector<const char*> middle = {
      // clang-format off
     "%12 = OpTypeSampler",
      // clang-format on
  };
  std::string spirv =
      JoinAllInsts(Concat(cap, (Concat(Concat(before_, middle), after_))));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto sampledImage_type = maybe_get<spirv_cross::SPIRType>(12, &ir);
  ASSERT_NE(sampledImage_type, nullptr);
}

TEST_F(SpvcIrParsingTest, OpTypeArrayInstruction) {
  const std::vector<const char*> middle = {
      // clang-format off
      "%13 = OpTypeInt 32 0",
      "%14 = OpConstant %13 70",
      " %7 = OpTypeVector %13 4",
      "%15 = OpTypeArray %7 %14"
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto array_type = maybe_get<spirv_cross::SPIRType>(15, &ir);
  ASSERT_NE(array_type, nullptr);
  EXPECT_EQ(array_type->parent_type, static_cast<spirv_cross::TypeID>(7));
  ASSERT_EQ(array_type->array_size_literal.size(), 1);
  EXPECT_TRUE(array_type->array_size_literal[0]);
  EXPECT_EQ(array_type->array[0], 70);

  auto constant_type = maybe_get<spirv_cross::SPIRConstant>(14, &ir);
  EXPECT_TRUE(constant_type->is_used_as_array_length);
}

TEST_F(SpvcIrParsingTest, OpTypeArrayInstructionSpec) {
  const std::vector<const char*> middle = {
      // clang-format off
      "%12 = OpTypeInt 32 0",
       "%13 = OpTypeFloat 32",
      "%14 = OpSpecConstant %12 3",
       " %7 = OpTypeVector %13 4",
      "%15 = OpTypeArray %7 %14"
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto array_type = maybe_get<spirv_cross::SPIRType>(15, &ir);
  ASSERT_NE(array_type, nullptr);
  EXPECT_EQ(array_type->parent_type, static_cast<spirv_cross::TypeID>(7));
  ASSERT_EQ(array_type->array_size_literal.size(), 1);
  EXPECT_FALSE(array_type->array_size_literal[0]);
  EXPECT_EQ(array_type->array[0], 14);

  auto constant_type = maybe_get<spirv_cross::SPIRConstant>(14, &ir);
  EXPECT_TRUE(constant_type->is_used_as_array_length);
}
TEST_F(SpvcIrParsingTest, OpTypeRuntimeArrayInstruction) {
  const std::vector<const char*> middle = {
      // clang-format off
      "%15 = OpTypeFloat 32",
      "%17 = OpTypeRuntimeArray %15",
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto run_type = maybe_get<spirv_cross::SPIRType>(17, &ir);
  ASSERT_NE(run_type, nullptr);
  ASSERT_EQ(run_type->array.size(), 1);
  EXPECT_EQ(run_type->array[0], 0);
  ASSERT_EQ(run_type->array_size_literal.size(), 1);
  EXPECT_TRUE(run_type->array_size_literal[0]);
  EXPECT_EQ(run_type->parent_type, static_cast<spirv_cross::TypeID>(15));
}

TEST_F(SpvcIrParsingTest, OpTypeFunctionInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func = maybe_get<spirv_cross::SPIRFunctionPrototype>(3, &ir_);
  ASSERT_NE(func, nullptr);
  EXPECT_EQ(func->return_type, static_cast<spirv_cross::TypeID>(2));
  EXPECT_TRUE(func->parameter_types.empty());
}

TEST_F(SpvcIrParsingTest, OpFunctionInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func = maybe_get<spirv_cross::SPIRFunction>(1, &ir_);
  ASSERT_NE(func, nullptr);
  EXPECT_EQ(func->type, spirv_cross::TypeFunction);
  EXPECT_EQ(func->return_type, static_cast<spirv_cross::TypeID>(2));
  EXPECT_EQ(func->function_type, static_cast<spirv_cross::TypeID>(3));
  EXPECT_TRUE(func->arguments.empty());
}

TEST_F(SpvcIrParsingTest, OpLabelInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto func = maybe_get<spirv_cross::SPIRFunction>(1, &ir_);
  ASSERT_NE(func, nullptr);
  auto block = maybe_get<spirv_cross::SPIRBlock>(4, &ir_);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(func->blocks.size(), static_cast<size_t>(1));
  EXPECT_EQ(func->blocks.data()[0], static_cast<spirv_cross::TypeID>(4));
  EXPECT_EQ(func->entry_block, static_cast<spirv_cross::TypeID>(4));
}

TEST_F(SpvcIrParsingTest, OpReturnInstruction) {
  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      input_, true, false, &ir_);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto block = maybe_get<spirv_cross::SPIRBlock>(4, &ir_);
  ASSERT_NE(block, nullptr);
  EXPECT_EQ(block->terminator, spirv_cross::SPIRBlock::Return);
}

TEST_F(SpvcIrParsingTest, SpvOpSourceInstruction) {
  const std::vector<const char*> middle = {"OpSource HLSL 500"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  EXPECT_FALSE(ir.source.es);
  EXPECT_EQ(ir.source.version, 450);
  EXPECT_TRUE(ir.source.known);
  EXPECT_TRUE(ir.source.hlsl);
}

TEST_F(SpvcIrParsingTest, SpvOpTypeIntInstruction) {
  const std::vector<const char*> middle = {"%16 = OpTypeInt 32 1"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto int_type = maybe_get<spirv_cross::SPIRType>(16, &ir);
  ASSERT_NE(int_type, nullptr);
  EXPECT_EQ(int_type->width, 32);
  EXPECT_EQ(int_type->basetype, spirv_cross::SPIRType::Int);
  EXPECT_EQ(int_type->vecsize, 1);
  EXPECT_EQ(int_type->columns, 1);
  EXPECT_EQ(int_type->array.size(), 0);
  EXPECT_EQ(int_type->type_alias, static_cast<spirv_cross::TypeID>(0));
  EXPECT_EQ(int_type->parent_type, static_cast<spirv_cross::TypeID>(0));
  EXPECT_TRUE(int_type->member_name_cache.empty());
}

TEST_F(SpvcIrParsingTest, SpvOpTypeIntInstructionUnsigned) {
  const std::vector<const char*> middle = {"%16 = OpTypeInt 32 0"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto int_type = maybe_get<spirv_cross::SPIRType>(16, &ir);
  ASSERT_NE(int_type, nullptr);
  EXPECT_EQ(int_type->width, 32);
  EXPECT_EQ(int_type->basetype, spirv_cross::SPIRType::UInt);
  EXPECT_EQ(int_type->vecsize, 1);
  EXPECT_EQ(int_type->columns, 1);
  EXPECT_EQ(int_type->array.size(), 0);
  EXPECT_EQ(int_type->type_alias, static_cast<spirv_cross::TypeID>(0));
  EXPECT_EQ(int_type->parent_type, static_cast<spirv_cross::TypeID>(0));
  EXPECT_TRUE(int_type->member_name_cache.empty());
}

TEST_F(SpvcIrParsingTest, SpvOpConstantInstruction) {
  const std::vector<const char*> middle = {" %8 = OpTypeInt 32 1",
                                           "%13 = OpConstant %8 100"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_constant = maybe_get<spirv_cross::SPIRConstant>(13, &ir);
  ASSERT_NE(spir_constant, nullptr);
  EXPECT_EQ(spir_constant->constant_type, static_cast<spirv_cross::TypeID>(8));
  ASSERT_EQ(spir_constant->m.columns, 1);
  ASSERT_EQ(spir_constant->vector_size(), 1);
  EXPECT_EQ(spir_constant->scalar(0, 0), 100);
  EXPECT_FALSE(spir_constant->specialization);
  EXPECT_FALSE(spir_constant->is_used_as_array_length);
  EXPECT_FALSE(spir_constant->is_used_as_lut);
  EXPECT_EQ(spir_constant->subconstants.size(), 0);
  EXPECT_EQ(spir_constant->specialization_constant_macro_name, "");
}

TEST_F(SpvcIrParsingTest, SpvOpConstantInstruction64) {
  const std::vector<const char*> middle = {" %8 = OpTypeInt 64 1",
                                           "%13 = OpConstant %8 0xF1F2F3F4"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_constant = maybe_get<spirv_cross::SPIRConstant>(13, &ir);
  ASSERT_NE(spir_constant, nullptr);
  EXPECT_EQ(spir_constant->constant_type, static_cast<spirv_cross::TypeID>(8));
  ASSERT_EQ(spir_constant->m.columns, 1);
  ASSERT_EQ(spir_constant->vector_size(), 1);
  EXPECT_EQ(spir_constant->scalar_u64(0, 0), 0xF1F2F3F4);
  EXPECT_FALSE(spir_constant->specialization);
  EXPECT_FALSE(spir_constant->is_used_as_array_length);
  EXPECT_FALSE(spir_constant->is_used_as_lut);
  EXPECT_EQ(spir_constant->subconstants.size(), 0);
  EXPECT_EQ(spir_constant->specialization_constant_macro_name, "");
}

TEST_F(SpvcIrParsingTest, SpvOpSpecConstantInstruction) {
  const std::vector<const char*> middle = {"%13 = OpTypeFloat 32",
                                           "%14 = OpSpecConstant %13 3.14159"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_constant = maybe_get<spirv_cross::SPIRConstant>(14, &ir);
  ASSERT_NE(spir_constant, nullptr);
  EXPECT_EQ(spir_constant->constant_type, static_cast<spirv_cross::TypeID>(13));
  ASSERT_EQ(spir_constant->m.columns, 1);
  ASSERT_EQ(spir_constant->vector_size(), 1);
  EXPECT_FLOAT_EQ(spir_constant->scalar_f32(0, 0), 3.14159);
  EXPECT_TRUE(spir_constant->specialization);
  EXPECT_FALSE(spir_constant->is_used_as_array_length);
  EXPECT_FALSE(spir_constant->is_used_as_lut);
  EXPECT_EQ(spir_constant->subconstants.size(), 0);
  EXPECT_EQ(spir_constant->specialization_constant_macro_name, "");
}

TEST_F(SpvcIrParsingTest, SpvOpTypePointer) {
  const std::vector<const char*> middle = {" %8 =  OpTypeInt 32 1",
                                           "%16 =  OpTypePointer Output %8"};
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_pointer = maybe_get<spirv_cross::SPIRType>(16, &ir);
  ASSERT_NE(spir_pointer, nullptr);
  EXPECT_TRUE(spir_pointer);
  EXPECT_EQ(spir_pointer->pointer_depth, 1);
  EXPECT_EQ(spir_pointer->storage, spv::StorageClass::StorageClassOutput);
  EXPECT_EQ(spir_pointer->parent_type, static_cast<spirv_cross::TypeID>(8));
  EXPECT_EQ(spir_pointer->width, 32);
  EXPECT_EQ(spir_pointer->basetype, spirv_cross::SPIRType::Int);
  EXPECT_EQ(spir_pointer->vecsize, 1);
  EXPECT_EQ(spir_pointer->columns, 1);
  EXPECT_EQ(spir_pointer->array.size(), 0);
  EXPECT_EQ(spir_pointer->type_alias, static_cast<spirv_cross::TypeID>(0));
  EXPECT_TRUE(spir_pointer->member_name_cache.empty());
}

TEST_F(SpvcIrParsingTest, SpvOpVariableInstruction) {
  const std::vector<const char*> middle = {
      " %8 =  OpTypeInt 32 1",
      "%16 =  OpTypePointer Output %8",
      "%17 =  OpVariable %16 Output",
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, false, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_variable = maybe_get<spirv_cross::SPIRVariable>(17, &ir);
  ASSERT_NE(spir_variable, nullptr);
  EXPECT_EQ(spir_variable->basetype, static_cast<spirv_cross::TypeID>(16));
  EXPECT_EQ(spir_variable->storage, spv::StorageClass::StorageClassOutput);
  EXPECT_EQ(spir_variable->initializer, static_cast<spirv_cross::TypeID>(0));
  EXPECT_EQ(spir_variable->basevariable, static_cast<spirv_cross::TypeID>(0));
}

TEST_F(SpvcIrParsingTest, OpDecorateInstruction) {
  const std::vector<const char*> middle = {
      // clang-format off
             "OpDecorate %17  Location 0",
      " %8 =  OpTypeInt 32 1",
      "%16 =  OpTypePointer Output %8",
      "%17 =  OpVariable %16 Output",
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_decoration = ir.find_meta(17);
  ASSERT_NE(spir_decoration, nullptr);
  auto& work_offsets = spir_decoration->decoration_word_offset;
  auto itr = work_offsets.find(spv::DecorationLocation);
  EXPECT_NE(itr, end(work_offsets));
  auto& work_offset = itr->second;
  EXPECT_EQ(work_offset, 28);
}

TEST_F(SpvcIrParsingTest, OpDecorateInstruction2) {
  const std::vector<const char*> middle = {
      // clang-format off
             "OpDecorate %17  Location 0",
             "OpDecorate %18  Location 1",
             "OpDecorate %19  Location 2",
             "OpDecorate %20  Location 3",
      " %8 =  OpTypeInt 32 1",
      "%16 =  OpTypePointer Output %8",
      "%17 =  OpVariable %16 Output",
      "%18 =  OpVariable %16 Output",
      "%19 =  OpVariable %16 Output",
      "%20 =  OpVariable %16 Output"
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  createSpvcIr(&ir, spirv);

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_decoration = ir.find_meta(20);
  ASSERT_NE(spir_decoration, nullptr);
  auto& work_offsets = spir_decoration->decoration_word_offset;
  auto itr = work_offsets.find(spv::DecorationLocation);
  EXPECT_NE(itr, end(work_offsets));
  auto& work_offset = itr->second;
  EXPECT_EQ(work_offset, 40);
}

TEST_F(SpvcIrParsingTest, OpNameInstruction) {
  const std::vector<const char*> middle = {
      // clang-format off
             "OpDecorate %17  Location 0",
             "OpName %17 \"var\""
      " %8 =  OpTypeInt 32 1",
      "%16 =  OpTypePointer Output %8",
      "%17 =  OpVariable %16 Output",
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  ASSERT_EQ(createSpvcIr(&ir, spirv), true)
      << "Could not create IRContext for input:\n" + spirv + "\n";

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  const auto var_name = ir.get_name(17);
  EXPECT_EQ(var_name, "var");
}

TEST_F(SpvcIrParsingTest, OpTypeStructInstruction) {
  const std::vector<const char*> middle = {
      // clang-format off
      " %8 =  OpTypeInt 32 1",
      "%16 =  OpTypePointer Output %8",
      "%22 =  OpTypeStruct %8 %8 %8 %8",
      "%20 =  OpTypeStruct %8 %8 %16",
      "%21 =  OpTypeStruct %8 %8 %16"
      // clang-format on
  };
  std::string spirv = JoinAllInsts(Concat(Concat(before_, middle), after_));
  spirv_cross::ParsedIR ir;
  ASSERT_EQ(createSpvcIr(&ir, spirv), true)
      << "Could not create IRContext for input:\n" + spirv + "\n";

  auto result = SinglePassRunAndDisassemble<SpvcIrPass, spirv_cross::ParsedIR*>(
      spirv, true, true, &ir);
  ASSERT_EQ(Pass::Status::SuccessWithoutChange, std::get<1>(result))
      << " SinglePassRunAndDisassemble failed on input:\n "
      << std::get<0>(result);

  auto spir_struct = maybe_get<spirv_cross::SPIRType>(20, &ir);
  ASSERT_NE(spir_struct, nullptr);
  EXPECT_EQ(spir_struct->basetype, spirv_cross::SPIRType::Struct);
  ASSERT_EQ(spir_struct->member_types.size(), 3);
  EXPECT_EQ(spir_struct->member_types.data()[0],
            static_cast<spirv_cross::TypeID>(8));
  EXPECT_EQ(spir_struct->member_types.data()[1],
            static_cast<spirv_cross::TypeID>(8));
  EXPECT_EQ(spir_struct->member_types.data()[2],
            static_cast<spirv_cross::TypeID>(16));
}

}  // namespace
}  // namespace opt
}  // namespace spvtools