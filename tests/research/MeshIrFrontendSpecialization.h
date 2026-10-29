#pragma once
// Opt-in research specialization. Never invoked by the production compiler.
#include "graphics/shader/recompiler/ir/ShaderIR.h"
#include <array>
#include <string>
#include <vector>

namespace Kyty::MeshResearch {
struct NonIndexedDraw {
  uint32_t count = 3;
  uint32_t base_vertex = 0;
  uint32_t first_instance = 0;
  uint32_t index_bytes = 0;
  uint32_t index_low = 0, index_high = 0;
  uint32_t group_count_x = 1, group_count_y = 1, group_count_z = 1;
  uint32_t first_group = 0;
};
// Substitute only declared runtime constants, then let ordinary compiler passes
// remove unreachable indexed reads. All validation precedes any mutation.
inline bool SpecializeNonIndexedDraw(IR::Program& program, const NonIndexedDraw& draw,
                                     std::string& error) {
  error.clear();
  if (program.stage != Libs::Graphics::ShaderType::Mesh || program.wave_size != 32) {
    error = "specialization requires Mesh wave32"; return false;
  }
  if (draw.count != 3 || draw.index_bytes || draw.index_low || draw.index_high ||
      draw.group_count_x != 1 || draw.group_count_y != 1 || draw.group_count_z != 1 || draw.first_group) {
    error = "specialization requires non-indexed three-vertex single-group draw"; return false;
  }
  const std::array<uint32_t,7> values{draw.count,draw.base_vertex,draw.first_instance,0,0,0,0};
  std::vector<std::pair<IR::Inst*,uint32_t>> replacements;
  for(auto* block:program.blocks) {
    if (!block) { error="null specialization block"; return false; }
    for(auto& inst:*block) {
      const auto op=inst.GetOpcode();
      if(op==IR::ValueOpcode::MeshDrawParameter) {
        const auto a=inst.Arg(0);
        if(inst.Flags<uint64_t>() || !a.IsImmediate() || a.GetType()!=IR::Type::U32 || a.U32()>=values.size()) {
          error="unsupported draw parameter specialization"; return false;
        }
        replacements.emplace_back(&inst,values[a.U32()]);
      } else if(op==IR::ValueOpcode::GetBuiltin) {
        const auto a=inst.Arg(0),b=inst.Arg(1);
        if(!a.IsImmediate() || !b.IsImmediate() || a.GetType()!=IR::Type::U32 || b.GetType()!=IR::Type::U32) {
          error="malformed builtin specialization"; return false;
        }
        if(a.U32()==static_cast<uint32_t>(IR::StageInputKind::WorkgroupId)) {
          if(b.U32()>2 || inst.Flags<uint64_t>()) { error="unsupported workgroup component"; return false; }
          replacements.emplace_back(&inst,0);
        }
      }
    }
  }
  for(auto [inst,value]:replacements) inst->ReplaceUsesWith(IR::Value(value));
  return true;
}
} // namespace Kyty::MeshResearch
