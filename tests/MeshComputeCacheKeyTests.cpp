// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/shader/recompiler/MeshComputeCacheKey.h"
#include <iostream>
namespace M=Libs::Graphics::ShaderRecompiler::MeshCompute;
int main(){
 int failures=0;auto check=[&](bool b){if(!b)++failures;};
 M::CompileRequest r;M::NonIndexedDraw draw;r.specialization=&draw;
 std::vector<uint8_t> source{1,2},iface{3};M::CacheVersions v;
 auto key=[&](){return M::BuildCacheKey(M::Backend::Compute,r,source,iface,v);};
 auto base=key();check(base==key());
 check(base!=M::BuildCacheKey(M::Backend::NativeMesh,r,source,iface,v));
 auto change=[&](uint32_t& n){++n;check(key()!=base);--n;};
 for(auto& n:r.mesh.threads_num)change(n);
 for(auto* n:{&r.mesh.lds_size_dwords,&r.mesh.scratch_size_dwords,&r.mesh.host_subgroup_size,&r.mesh.wave_size,
   &r.mesh.input_primitive,&r.mesh.primitives_per_group,&r.mesh.vertices_per_group,&r.mesh.max_vertices,
   &r.mesh.max_primitives,&r.mesh.provoking_vertex,&r.mesh.split_groups,&r.uv_capacity,
   &draw.count,&draw.base_vertex,&draw.first_instance,&draw.index_bytes,&draw.index_low,&draw.index_high,
   &draw.group_count_x,&draw.group_count_y,&draw.group_count_z,&draw.first_group,
   &v.schema,&v.compiler,&v.geometry_abi,&v.uv_abi,&v.bridge})change(*n);
 r.enable_uv=true;check(key()!=base);r.enable_uv=false;
 r.specialization=nullptr;check(key()!=base);
 M::NonIndexedDraw copied=draw;r.specialization=&copied;check(key()==base);
 source={1};iface={2,3};check(key()!=base); // same concatenated bytes, different boundary
 source={1,2};iface={4};check(key()!=base);
 source={1,3};iface={3};check(key()!=base);
 source={1,2};check(key()==base);
 check(base[0]==0x4b && base[1]==0x4d && base[2]==0x43 && base[3]==0x4b);
 std::cout<<"mesh cache canonical identity: "<<(failures?"FAIL":"PASS")<<" (CPU only, caches unwired)\n";
 return failures?1:0;
}
