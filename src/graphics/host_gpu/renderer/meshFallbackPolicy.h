// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstdint>

namespace Libs::Graphics::MeshFallback {
// Pure admission policy only. ComputeCandidate is NOT permission to record a
// draw: compiler/resource/export/fragment and runtime bundle checks follow.
enum class Mode { Disabled, Compute };
enum class Stage { Unknown, OrdinaryGraphics, Mesh };
enum class Topology { Unknown, TriangleList, Other };
enum class Route { ExistingGraphics, NativeMesh, Unsupported, ComputeCandidate };
enum class Reason {
  None, UnknownStage, InvalidMode, Disabled, Vulkan13, Queue, LocalSize,
  StorageBindings, StorageRange, UnknownDraw, Topology, Indexed, Indirect,
  Restart, VertexCount, InstanceCount, VertexOffset, InstanceOffset, Groups
};
struct Capabilities {
  bool native_mesh_enabled = false; // actual enabled feature, never emulated
  uint32_t api_major = 0, api_minor = 0;
  bool compute_graphics_queue = false;
  uint32_t max_compute_invocations = 0;
  uint32_t max_compute_size_x = 0, max_compute_size_y = 0, max_compute_size_z = 0;
  uint32_t max_compute_groups_x = 0, max_compute_groups_y = 0, max_compute_groups_z = 0;
  uint32_t storage_buffers_per_stage = 0, storage_buffers_per_set = 0;
  uint64_t max_storage_range = 0;
};
struct Draw {
  bool metadata_known = false;
  Topology topology = Topology::Unknown;
  bool indexed = false, indirect = false, primitive_restart = false;
  uint32_t vertex_count = 0, instance_count = 0;
  uint32_t first_vertex = 0, first_instance = 0;
  int32_t base_vertex = 0;
  uint32_t groups_x = 0, groups_y = 0, groups_z = 0, first_group = 0;
};
struct Decision {
  Route route;
  Reason reason;
  bool operator==(const Decision&) const = default;
};
constexpr Decision Select(Mode mode, Stage stage, const Capabilities& caps, const Draw& draw) {
  // Existing supported paths take precedence, irrespective of fallback settings.
  if(stage==Stage::OrdinaryGraphics) return {Route::ExistingGraphics,Reason::None};
  if(stage!=Stage::Mesh) return {Route::Unsupported,Reason::UnknownStage};
  if(caps.native_mesh_enabled) return {Route::NativeMesh,Reason::None};
  auto reject=[](Reason r) {return Decision{Route::Unsupported,r};};
  if(mode==Mode::Disabled) return reject(Reason::Disabled);
  if(mode!=Mode::Compute) return reject(Reason::InvalidMode);
  if(caps.api_major<1 || (caps.api_major==1 && caps.api_minor<3)) return reject(Reason::Vulkan13);
  if(!caps.compute_graphics_queue) return reject(Reason::Queue);
  if(caps.max_compute_invocations<32 || caps.max_compute_size_x<32 ||
     caps.max_compute_size_y<1 || caps.max_compute_size_z<1 ||
     caps.max_compute_groups_x<1 || caps.max_compute_groups_y<1 || caps.max_compute_groups_z<1)
    return reject(Reason::LocalSize);
  // Conservative three-binding contract covers geometry, payload and UV.
  if(caps.storage_buffers_per_stage<3 || caps.storage_buffers_per_set<3) return reject(Reason::StorageBindings);
  if(caps.max_storage_range<1840) return reject(Reason::StorageRange);
  if(!draw.metadata_known) return reject(Reason::UnknownDraw);
  if(draw.topology!=Topology::TriangleList) return reject(Reason::Topology);
  if(draw.indexed) return reject(Reason::Indexed);
  if(draw.indirect) return reject(Reason::Indirect);
  if(draw.primitive_restart) return reject(Reason::Restart);
  if(draw.vertex_count!=3) return reject(Reason::VertexCount);
  if(draw.instance_count!=1) return reject(Reason::InstanceCount);
  if(draw.first_vertex || draw.base_vertex) return reject(Reason::VertexOffset);
  if(draw.first_instance) return reject(Reason::InstanceOffset);
  if(draw.groups_x!=1 || draw.groups_y!=1 || draw.groups_z!=1 || draw.first_group) return reject(Reason::Groups);
  return {Route::ComputeCandidate,Reason::None};
}
} // namespace Libs::Graphics::MeshFallback
