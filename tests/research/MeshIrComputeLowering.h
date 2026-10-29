#pragma once
// Opt-in offline research only: no renderer/native-emitter call sites.
#include "graphics/shader/recompiler/ir/ShaderIR.h"
#include <span>
#include <string>
#include <vector>

namespace Kyty::MeshResearch {
namespace IR = Libs::Graphics::ShaderRecompiler::IR;
// A non-owning view of ALL post-prologue/SSA blocks and export metadata. This
// proof accepts a complete straight-line chain, never a selected block of a CFG.
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
  uint32_t uv_capacity = 64;
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
} // namespace Kyty::MeshResearch
