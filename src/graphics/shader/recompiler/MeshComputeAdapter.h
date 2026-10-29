// SPDX-License-Identifier: GPL-2.0-only
#pragma once
// Isolated production-owned compiler adapter for the bounded mesh fallback
// research (S-048): the shared pre-resource preparation, the bounded draw
// specialization and the SPIR-V lowering live under this directory so
// production code has no dependency on tests. This adapter has no renderer,
// pipeline-cache or draw-recording call sites yet; enabling the fallback in the
// renderer stays a separate, not-started phase. Native/default compile and draw
// behavior is unchanged.
#include "graphics/shader/recompiler/MeshComputeLowering.h"
#include "graphics/shader/recompiler/MeshComputeSpecialization.h"
#include "graphics/shader/recompiler/ir/passes/PreparePreResource.h"

#include <string>
#include <set>

namespace Libs::Graphics::ShaderRecompiler::MeshCompute {

// Caller-owned stage expectations: declared output capacities and the direct
// draw context the specialization may bake into the module. `specialization`
// must be supplied whenever the complete program still contains live
// MeshDrawParameter values from the NGG prologue; leaving it null rejects any
// program whose draw parameters survive the shared preparation.
struct CompileRequest {
  Libs::Graphics::ShaderMeshInputInfo mesh;
  bool enable_uv = false;
  uint32_t uv_capacity = Kyty::MeshOutput::kUvCapacity;
  // Applied to a private clone of raw frontend IR on every compilation.
  const NonIndexedDraw* specialization = nullptr;
};

// Compile raw, unsealed frontend IR without modifying it. Prepared/specialized
// inputs are rejected: they have lost runtime draw expressions. Each call clones
// owned IR before normalization and specialization.
// Compile one complete translated mesh program into a bounded Vulkan 1.3
// GLCompute module. The adapter always runs the shared production pre-resource
// preparation (never a test-side copy of those passes), applies the requested
// specialization before one further shared preparation removes inactive
// indexed reads, and then lowers the complete retained block graph. Any
// unsupported or malformed input returns the stable diagnostic with EMPTY
// binary and assembly outputs; no partial module is produced and the default
// native path is untouched.
[[nodiscard]] inline Result Compile(const IR::Program& source, const CompileRequest& request) {
  Result result;
  auto reject = [&](const char* message) { result.error=message; return result; };
  if (source.stage != Libs::Graphics::ShaderType::Mesh) {
    result.error = "unsupported stage; the adapter lowers complete mesh programs only";
    return result;
  }
  if(source.wave_size!=32 || request.mesh.wave_size!=source.wave_size || request.mesh.host_subgroup_size!=32)
    return reject("adapter requires matching wave32 program and request");
  if(source.blocks.empty() || source.blocks.size()>16 || source.block_info.size()!=source.blocks.size())
    return reject("adapter requires complete bounded frontend CFG metadata");
  std::set<const IR::Block*> seen;
  std::set<uint32_t> ids;
  size_t count=0;
  for(size_t i=0;i<source.blocks.size();++i) {
    const auto* block=source.blocks[i];
    if(!block || !seen.insert(block).second) return reject("null or duplicate frontend block");
    if(block->IsSsaSealed()) return reject("adapter requires raw unspecialized frontend IR");
    const auto pred=block->ImmPredecessors(),succ=block->ImmSuccessors();
    if((i==0 ? !pred.empty() : pred.size()!=1 || pred[0]!=source.blocks[i-1]) ||
       (i+1==source.blocks.size() ? !succ.empty() : succ.size()!=1 || succ[0]!=source.blocks[i+1]))
      return reject("adapter requires complete ordered linear frontend CFG");
    const auto& info=source.block_info[i];const auto& t=info.terminator;
    namespace CFG=Libs::Graphics::ShaderRecompiler::CFG;
    if(!ids.insert(info.id).second || t.condition!=CFG::BranchCondition::Always ||
       t.loop_header || t.merge_block!=UINT32_MAX || t.continue_block!=UINT32_MAX ||
       (i+1==source.blocks.size() ? t.kind!=CFG::TerminatorKind::Return :
        t.kind!=CFG::TerminatorKind::Branch || t.true_block!=source.block_info[i+1].id))
      return reject("unsupported frontend terminator");
    if(block->Instructions().size()>4096-count) return reject("frontend instruction bound exceeded");
    count+=block->Instructions().size();
    for(const auto& inst:*block) {
      const auto op=inst.GetOpcode();
      if(op<IR::ValueOpcode{} || op>=IR::ValueOpcode::Count ||
         inst.NumArgs()!=IR::NumArgsOf(op) || inst.Parent()!=block)
        return reject("malformed frontend instruction");
    }
  }
  IR::Program program;
  if(!IR::CloneProgram(source,program)) return reject("frontend IR references unowned blocks or values");
  // Exact shared production preparation prefix before any resource tracking.
  IR::PreparePreResource(program);
  if (request.specialization != nullptr) {
    std::string error;
    if (!SpecializeNonIndexedDraw(program, *request.specialization, error)) {
      result.error = error; // caller-owned source remains unchanged
      return result;
    }
    // Ordinary propagation/DCE removes the now-inactive indexed reads.
    IR::NormalizePreResource(program);
  }
  Input input;
  input.stage = program.stage;
  input.mesh = request.mesh;
  input.blocks = program.blocks;
  input.exports = program.export_info;
  input.block_info = program.block_info;
  input.enable_uv = request.enable_uv;
  input.uv_capacity = request.uv_capacity;
  return Lower(input);
}

} // namespace Libs::Graphics::ShaderRecompiler::MeshCompute
