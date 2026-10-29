// SPDX-License-Identifier: GPL-2.0-only
// Compile-only boundary: checks the REAL Buffer adapter, not a mock
// declaration.
#include "graphics/host_gpu/renderer/meshFallbackBuffers.h"
#include "graphics/host_gpu/renderer/meshFallbackResources.h"
Libs::Graphics::MeshFallback::DrawResources CheckMeshResourcesBoundary(
    Libs::Graphics::GraphicContext &graphics,
    Libs::Graphics::CommandScheduler &scheduler,
    const Libs::Graphics::MeshFallback::CompileBundleResult &bundle) {
  return Libs::Graphics::MeshFallback::PrepareResources(graphics, scheduler,
                                                        bundle);
}
Libs::Graphics::MeshFallback::PreparedBuffers<Libs::Graphics::Buffer>
CheckMeshBufferBoundary(
    Libs::Graphics::GraphicContext &graphics,
    Libs::Graphics::CommandScheduler &scheduler,
    const Libs::Graphics::MeshFallback::CompileBundleResult &bundle) {
  return Libs::Graphics::MeshFallback::PrepareBuffers(graphics, scheduler,
                                                      bundle);
}
