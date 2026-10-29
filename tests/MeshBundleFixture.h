// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/host_gpu/renderer/meshFallbackBundle.h"
#include <spirv-tools/libspirv.hpp>
#include <algorithm>
namespace KytyMeshFixture {
// Public fixture compiler with reference Vulkan1.3 policy limits, NOT admission
// of the actual runtime device. Runtime independently checks requested API/limits.
Libs::Graphics::MeshFallback::CompileBundleResult Build(unsigned variant,std::span<const uint32_t> fragment);
inline std::vector<uint32_t> ApiModule(std::span<const uint32_t> module,bool api13) {
  spvtools::SpirvTools source(SPV_ENV_VULKAN_1_3),target(api13?SPV_ENV_VULKAN_1_3:SPV_ENV_VULKAN_1_2);
  std::string text;std::vector<uint32_t> out;
  if(!source.Validate(module.data(),module.size()) || !source.Disassemble(module.data(),module.size(),&text) ||
     !target.Assemble(text,&out) || !target.Validate(out))return {};
  return out;
}
inline bool Matches(std::span<const uint32_t> executed,std::span<const uint32_t> proven,bool api13) {
  const auto expected=ApiModule(proven,api13);
  return !expected.empty() && std::equal(expected.begin(),expected.end(),executed.begin(),executed.end());
}
}
