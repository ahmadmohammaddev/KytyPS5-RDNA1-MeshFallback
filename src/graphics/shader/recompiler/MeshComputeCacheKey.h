// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/shader/recompiler/MeshComputeAdapter.h"
#include <span>
#include <vector>
namespace Libs::Graphics::ShaderRecompiler::MeshCompute {
// Isolated identity, not wired to the native program/disk caches. Compare ALL
// bytes; a digest may index a cache but can never replace exact equality.
enum class Backend : uint32_t { NativeMesh=1, Compute=2 };
struct CacheVersions { uint32_t schema=1, compiler=1, geometry_abi=1, uv_abi=1, bridge=1; };
// source_identity must be the full canonical source+frontend-options key, not a
// shader hash. interface_identity must encode complete export/fragment bridge
// layout. These opaque byte strings are length-prefixed to avoid ambiguity.
inline std::vector<uint8_t> BuildCacheKey(Backend backend, const CompileRequest& r,
    std::span<const uint8_t> source_identity, std::span<const uint8_t> interface_identity,
    CacheVersions versions={}) {
  std::vector<uint8_t> out;
  auto word=[&](uint32_t v){for(unsigned i=0;i<4;++i) out.push_back(uint8_t(v>>(i*8)));};
  auto blob=[&](std::span<const uint8_t> bytes){uint64_t n=bytes.size();word(uint32_t(n));word(uint32_t(n>>32));out.insert(out.end(),bytes.begin(),bytes.end());};
  word(0x4b434d4b);word(versions.schema);word(uint32_t(backend));word(versions.compiler);
  word(versions.geometry_abi);word(versions.uv_abi);word(versions.bridge);
  blob(source_identity);blob(interface_identity);
  const auto& m=r.mesh;
  for(auto n:m.threads_num) word(n);
  for(auto n:{m.lds_size_dwords,m.scratch_size_dwords,m.host_subgroup_size,m.wave_size,
      m.input_primitive,m.primitives_per_group,m.vertices_per_group,m.max_vertices,
      m.max_primitives,m.provoking_vertex,m.split_groups}) word(n);
  word(r.enable_uv?1:0);word(r.uv_capacity);word(r.specialization?1:0);
  if(r.specialization) {
    const auto& d=*r.specialization;
    for(auto n:{d.count,d.base_vertex,d.first_instance,d.index_bytes,d.index_low,d.index_high,
        d.group_count_x,d.group_count_y,d.group_count_z,d.first_group}) word(n);
  }
  return out;
}
} // namespace Libs::Graphics::ShaderRecompiler::MeshCompute
