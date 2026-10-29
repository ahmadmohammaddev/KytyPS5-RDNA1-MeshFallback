// SPDX-License-Identifier: GPL-2.0-only
#pragma once
// Isolated opt-in compute compiler adapter lowering, moved from the research dir
// (S-048) so production code never depends on tests. No renderer/native-emitter
// call sites yet; legacy test consumers now include these headers from src.
#include "graphics/shader/recompiler/ir/ShaderIR.h"
#include "graphics/shader/recompiler/MeshComputeAbi.h"
#include <span>
#include <string>
#include <vector>

namespace Libs::Graphics::ShaderRecompiler::MeshCompute {
namespace IR = Libs::Graphics::ShaderRecompiler::IR;
// A non-owning view of ALL post-prologue/SSA blocks and export metadata. This
// adapter accepts a complete straight-line chain, never a selected block of a CFG.
// It is not a guest ISA decoder or a general NGG prologue lowering.
struct Input {
  Libs::Graphics::ShaderType stage = Libs::Graphics::ShaderType::Mesh;
  Libs::Graphics::ShaderMeshInputInfo mesh;
  std::span<IR::Block *const> blocks;
  std::span<const IR::ExportInfo> exports;
  // Required for multi-block inputs: full terminator metadata proves each edge
  // unconditional and the last block a return (not a masked/indirect exit).
  std::span<const IR::BlockInfo> block_info;
  // Research-only parameter-0 xy raw float bits, set 0/binding 2. The fixed
  // 544-byte guarded buffer is independent of the position/index ABI.
  bool enable_uv = false;
  uint32_t uv_capacity = Kyty::MeshOutput::kUvCapacity;
};
struct Result {
  std::vector<uint32_t> binary;
  std::string assembly;
  std::string error;
  uint32_t vertices = 0;
  uint32_t primitives = 0;
  explicit operator bool() const { return error.empty() && !binary.empty(); }
};
// On any unsupported/malformed input: diagnostic + EMPTY binary/assembly.
// Always Vulkan 1.3 SPIR-V. No feature spoofing or handwritten GLSL route.
Result Lower(const Input &input);
} // namespace Libs::Graphics::ShaderRecompiler::MeshCompute
