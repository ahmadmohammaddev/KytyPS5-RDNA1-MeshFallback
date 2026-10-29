// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace Libs::Graphics::ShaderRecompiler::MeshCompute {
struct BridgeResult {
  std::vector<uint32_t> binary;
  std::string assembly, error;
  // Canonical interface identity for BuildCacheKey (bridge version, UV flag).
  std::vector<uint8_t> interface_identity;
  explicit operator bool() const {return error.empty() && !binary.empty();}
};
// Validated Fragment main, no inputs or smooth float4 parameter0 with proven
// x/y-only scalar reads. Input position=float4 location0, UV=float2 location1;
// output Position and optional float4 location0=(u,v,0,0). No renderer call sites.
BridgeResult BuildVertexBridge(std::span<const uint32_t> fragment, bool uv_available);
}
