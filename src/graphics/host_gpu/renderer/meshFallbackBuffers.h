// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/host_gpu/renderer/cache/streamBuffer.h"
#include "graphics/host_gpu/renderer/meshFallbackBufferPreparation.h"
#include "graphics/host_gpu/renderer/meshFallbackBundle.h"
namespace Libs::Graphics::MeshFallback {
// Uses real Buffer; no descriptor/command work. Only retain unsubmitted ownership
// here. The future scheduler adapter must defer destruction after GPU completion.
[[nodiscard]] PreparedBuffers<Buffer> PrepareBuffers(GraphicContext&            graphics,
                                                     CommandScheduler&          scheduler,
                                                     const CompileBundleResult& bundle);
} // namespace Libs::Graphics::MeshFallback
