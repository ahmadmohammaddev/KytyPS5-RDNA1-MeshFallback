// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/host_gpu/renderer/meshFallbackResources.h"
namespace Libs::Graphics::MeshFallback {
// Non-owning views, valid ONLY for this recording. Caller is the serialized
// recording producer; no worker, concurrent shutdown or later/cross-queue use.
// Obtain/retain BEFORE the first reference; record ALL compute+graphics uses
// before submitting the tick. There is no cancel after recording references.
struct RecordingResources {
	uint64_t                tick     = 0;
	vk::Buffer              geometry = nullptr, uv = nullptr;
	vk::DescriptorSet       descriptors = nullptr;
	vk::DescriptorSetLayout layout      = nullptr;
	explicit                operator bool() const { return geometry && descriptors && layout; }
};
[[nodiscard]] RecordingResources RetainResourcesBeforeUse(CommandScheduler& scheduler,
                                                          DrawResources&&   resources);
} // namespace Libs::Graphics::MeshFallback
