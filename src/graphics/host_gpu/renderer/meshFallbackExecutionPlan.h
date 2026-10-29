// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/shader/recompiler/MeshComputeAbi.h"
#include <optional>
#include <cstdint>
namespace Libs::Graphics::MeshFallback {
// Pure description, not Vulkan synchronization or allocation. Only consume
// a plan owned by a successful CompileBundleResult; arbitrary caller counts
// are not compiler proof. Fresh initialized guarded storage per in-flight draw
// must remain alive through compute AND graphics completion before reuse.
struct ExecutionPlan {
  uint32_t vertices=0,primitives=0,index_count=0;
  uint64_t geometry_bytes=0,uv_bytes=0;
  uint64_t position_offset=0,position_bytes=0,index_offset=0,index_bytes=0;
  uint64_t uv_offset=0,uv_vertex_bytes=0;
  uint32_t position_stride=16,uv_stride=8,index_element_bytes=4;
  uint32_t geometry_set=0,geometry_binding=1,uv_set=0,uv_binding=2;
  uint32_t dispatch_x=1,dispatch_y=1,dispatch_z=1;
  // Generated modules bake their values: no input payload descriptor binding0.
  bool draw_indexed=false,initialize_guards_and_pending=true;
  bool compute_write_to_vertex_read=false,compute_write_to_index_read=false;
  bool compute_write_to_uv_vertex_read=false;
};
// Internal checked layout arithmetic; caller must supply LOWERING-PROVEN counts.
// Capacity restrictions are validated even for zero, with no partial plan.
inline std::optional<ExecutionPlan> CheckedExecutionLayout(uint32_t vertices,uint32_t primitives,
    uint32_t vertex_capacity,uint32_t primitive_capacity,bool uv,uint32_t uv_capacity) {
  namespace A=Kyty::MeshOutput;
  if(!vertex_capacity || vertex_capacity>A::kMaxVertices || !primitive_capacity || primitive_capacity>A::kMaxPrimitives ||
     vertices>vertex_capacity || primitives>primitive_capacity || (primitives && !vertices) ||
     (uv && (!uv_capacity || uv_capacity>A::kUvCapacity || vertices>uv_capacity)))return std::nullopt;
  ExecutionPlan p;p.vertices=vertices;p.primitives=primitives;
  p.index_count=primitives*3; // bounded <=192 BEFORE multiplication
  p.geometry_bytes=A::kTotalBytes;p.uv_bytes=uv?A::kUvWords*4:0;
  p.position_offset=A::kPositionsOffsetBytes;p.position_bytes=uint64_t(vertices)*16;
  p.index_offset=A::kIndicesOffsetBytes;p.index_bytes=uint64_t(p.index_count)*4;
  p.uv_offset=uv?A::kUvFirstSlotWord*4:0;p.uv_vertex_bytes=uv?uint64_t(vertices)*8:0;
  // Active ranges never consume guards or count/status words.
  if(p.position_offset+p.position_bytes>A::kPositionWords*4 ||
     p.index_offset+p.index_bytes>A::kGuardAfterIndicesWord*4 ||
     (uv && p.uv_offset+p.uv_vertex_bytes>A::kUvGuardWordIndexes[2]*4))return std::nullopt;
  p.draw_indexed=p.index_count!=0;
  p.compute_write_to_vertex_read=p.compute_write_to_index_read=p.draw_indexed;
  p.compute_write_to_uv_vertex_read=p.draw_indexed&&uv;
  return p;
}
}
