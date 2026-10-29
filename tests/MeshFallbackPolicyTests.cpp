// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/host_gpu/renderer/meshFallbackPolicy.h"
#include <cstdlib>
#include <iostream>
namespace P=Libs::Graphics::MeshFallback;
int failures=0;
void Check(bool value,const char* name) {if(!value){++failures;std::cerr<<"FAIL: "<<name<<'\n';}}
int main() {
  P::Capabilities c{.api_major=1,.api_minor=3,.compute_graphics_queue=true,
    .max_compute_invocations=32,.max_compute_size_x=32,.max_compute_size_y=1,.max_compute_size_z=1,
    .max_compute_groups_x=1,.max_compute_groups_y=1,.max_compute_groups_z=1,
    .storage_buffers_per_stage=3,.storage_buffers_per_set=3,.max_storage_range=1840};
  P::Draw d{.metadata_known=true,.topology=P::Topology::TriangleList,.vertex_count=3,.instance_count=1,
    .groups_x=1,.groups_y=1,.groups_z=1};
  auto select=[&](){return P::Select(P::Mode::Compute,P::Stage::Mesh,c,d);};
  Check(select()==P::Decision{P::Route::ComputeCandidate,P::Reason::None},"exact bound is candidate, never drawable readiness");
  auto caps=c;auto draw=d;
  auto reject=[&](P::Reason reason){Check(select()==P::Decision{P::Route::Unsupported,reason},"precise negative admission reason");c=caps;d=draw;};
  c.api_minor=2;reject(P::Reason::Vulkan13);
  c.api_major=0;reject(P::Reason::Vulkan13);
  c.compute_graphics_queue=false;reject(P::Reason::Queue);
  c.max_compute_invocations=31;reject(P::Reason::LocalSize);
  c.max_compute_size_x=31;reject(P::Reason::LocalSize);
  c.max_compute_size_y=0;reject(P::Reason::LocalSize);
  c.max_compute_size_z=0;reject(P::Reason::LocalSize);
  c.max_compute_groups_x=0;reject(P::Reason::LocalSize);
  c.max_compute_groups_y=0;reject(P::Reason::LocalSize);
  c.max_compute_groups_z=0;reject(P::Reason::LocalSize);
  c.storage_buffers_per_stage=2;reject(P::Reason::StorageBindings);
  c.storage_buffers_per_set=2;reject(P::Reason::StorageBindings);
  c.max_storage_range=1839;reject(P::Reason::StorageRange);
  d.metadata_known=false;reject(P::Reason::UnknownDraw);
  d.topology=P::Topology::Unknown;reject(P::Reason::Topology);
  d.topology=P::Topology::Other;reject(P::Reason::Topology);
  d.indexed=true;reject(P::Reason::Indexed);
  d.indirect=true;reject(P::Reason::Indirect);
  d.primitive_restart=true;reject(P::Reason::Restart);
  for(auto n:{0u,2u,4u,0xffffffffu}){d.vertex_count=n;reject(P::Reason::VertexCount);}
  for(auto n:{0u,2u,0xffffffffu}){d.instance_count=n;reject(P::Reason::InstanceCount);}
  d.first_vertex=1;reject(P::Reason::VertexOffset);
  d.base_vertex=-1;reject(P::Reason::VertexOffset);
  d.base_vertex=1;reject(P::Reason::VertexOffset);
  d.first_instance=1;reject(P::Reason::InstanceOffset);
  d.groups_x=0;reject(P::Reason::Groups);
  d.groups_x=2;reject(P::Reason::Groups);
  d.groups_y=2;reject(P::Reason::Groups);
  d.groups_z=2;reject(P::Reason::Groups);
  d.first_group=1;reject(P::Reason::Groups);
  Check(P::Select(P::Mode::Compute,P::Stage::Unknown,c,d).reason==P::Reason::UnknownStage,"unknown stage rejects");
  Check(P::Select(static_cast<P::Mode>(99),P::Stage::Mesh,c,d).reason==P::Reason::InvalidMode,"invalid mode rejects");
  Check(P::Select(P::Mode::Compute,P::Stage::Mesh,{},{}).route==P::Route::Unsupported,"unknown capability/draw defaults reject");
  // Independent exhaustive coarse state space: supported existing routes and
  // disabled fallback must be invariant even with incompatible draw metadata.
  for(unsigned bits=0;bits<512;++bits) {
    auto cc=c;auto dd=d;
    cc.native_mesh_enabled=bits&1;cc.compute_graphics_queue=bits&2;
    cc.api_minor=(bits&4)?3:2;cc.max_compute_invocations=(bits&8)?32:0;
    dd.metadata_known=bits&16;dd.indexed=bits&32;dd.indirect=bits&64;
    dd.vertex_count=(bits&128)?3:0;dd.instance_count=(bits&256)?1:0;
    for(auto mode:{P::Mode::Disabled,P::Mode::Compute}) {
      Check(P::Select(mode,P::Stage::OrdinaryGraphics,cc,dd).route==P::Route::ExistingGraphics,"ordinary path unchanged");
      const auto decision=P::Select(mode,P::Stage::Mesh,cc,dd);
      if(cc.native_mesh_enabled) Check(decision.route==P::Route::NativeMesh,"native always preferred");
      else if(mode==P::Mode::Disabled) Check(decision==P::Decision{P::Route::Unsupported,P::Reason::Disabled},"disabled cannot choose compute");
    }
  }
  std::cout<<"mesh fallback policy: "<<(failures?"FAIL":"PASS")<<" (CPU only; renderer unwired)\n";
  return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
