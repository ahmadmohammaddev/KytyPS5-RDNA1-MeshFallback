// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/host_gpu/renderer/meshFallbackPolicy.h"
#include "graphics/host_gpu/renderer/meshFallbackExecutionPlan.h"
#include "graphics/shader/recompiler/MeshComputeCacheKey.h"
#include "graphics/shader/recompiler/MeshVertexBridge.h"

namespace Libs::Graphics::MeshFallback {
// CPU-owned, isolated compile product; NOT permission to record a draw.
// No descriptor allocation, Vulkan handles, renderer or disk-cache mutation.
struct CompileBundleResult {
  std::vector<uint32_t> compute,vertex,fragment;
  std::vector<uint8_t> key,interface_identity;
  size_t geometry_bytes=0,uv_bytes=0;
  std::optional<ExecutionPlan> plan;
  Decision decision{Route::Unsupported,Reason::None};
  std::string error;
  explicit operator bool() const {return error.empty() && plan.has_value() && !compute.empty() && !vertex.empty() && !fragment.empty() && !key.empty();}
};
// source_identity remains the caller's complete canonical source/frontend-options
// identity, not a hash. Exact generated modules are additionally part of the key.
// Specialization is derived ONLY from the admitted Draw, never a second context.
inline CompileBundleResult CompileBundle(Mode mode,const Capabilities& caps,const Draw& draw,
    const ShaderRecompiler::IR::Program& source,
    ShaderRecompiler::MeshCompute::CompileRequest request,
    std::span<const uint32_t> fragment,std::span<const uint8_t> source_identity) {
  namespace M=ShaderRecompiler::MeshCompute;
  CompileBundleResult out;
  const auto stage=source.stage==ShaderType::Mesh?Stage::Mesh:Stage::Unknown;
  out.decision=Select(mode,stage,caps,draw);
  auto reject=[&](const std::string& error){out.error=error;return out;};
  if(out.decision.route!=Route::ComputeCandidate)return reject("bundle policy did not select compute candidate");
  if(source_identity.empty())return reject("bundle requires canonical source identity");
  if(request.specialization)return reject("bundle derives specialization from admitted draw only");
  auto bridge=M::BuildVertexBridge(fragment,request.enable_uv);
  if(!bridge)return reject("bundle fragment: "+bridge.error);
  M::NonIndexedDraw context;
  context.count=draw.vertex_count;context.base_vertex=uint32_t(draw.base_vertex);
  context.first_instance=draw.first_instance;context.group_count_x=draw.groups_x;
  context.group_count_y=draw.groups_y;context.group_count_z=draw.groups_z;context.first_group=draw.first_group;
  request.specialization=&context;
  auto compiled=M::Compile(source,request);
  if(!compiled)return reject("bundle compute: "+compiled.error);
  const auto plan=CheckedExecutionLayout(compiled.vertices,compiled.primitives,request.mesh.max_vertices,
      request.mesh.max_primitives,request.enable_uv,request.uv_capacity);
  if(!plan)return reject("bundle output proof violates execution layout");
  std::vector<uint8_t> identity(source_identity.begin(),source_identity.end());
  auto word=[&](uint32_t w){for(unsigned i=0;i<4;++i)identity.push_back(uint8_t(w>>(i*8)));};
  // Unambiguous length prefix for the opaque identity and every exact module.
  std::vector<uint8_t> full;full.swap(identity);
  word(0x424e444c);word(2); // bundle identity domain/version
  word(uint32_t(uint64_t(full.size())));word(uint32_t(uint64_t(full.size())>>32));
  identity.insert(identity.end(),full.begin(),full.end());
  for(auto module:{std::span<const uint32_t>(compiled.binary),std::span<const uint32_t>(bridge.binary),fragment}) {
    word(uint32_t(uint64_t(module.size())));word(uint32_t(uint64_t(module.size())>>32));
    for(auto w:module)word(w);
  }
  auto key=M::BuildCacheKey(M::Backend::Compute,request,identity,bridge.interface_identity);
  // Publish only once EVERY compiler/interface/identity step succeeds.
  out.plan=plan;
  out.compute=std::move(compiled.binary);out.vertex=std::move(bridge.binary);
  out.fragment.assign(fragment.begin(),fragment.end());out.key=std::move(key);
  out.interface_identity=std::move(bridge.interface_identity);
  out.geometry_bytes=Kyty::MeshOutput::kTotalBytes;
  out.uv_bytes=request.enable_uv?Kyty::MeshOutput::kUvWords*4:0;
  return out;
}
}
