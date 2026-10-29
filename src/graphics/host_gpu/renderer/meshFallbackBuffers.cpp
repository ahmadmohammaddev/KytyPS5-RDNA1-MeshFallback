// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/host_gpu/renderer/meshFallbackBuffers.h"

namespace Libs::Graphics::MeshFallback {
PreparedBuffers<Buffer> PrepareBuffers(GraphicContext& graphics, CommandScheduler& scheduler,
                                       const CompileBundleResult& bundle) {
	if (!bundle) return {};
	return PrepareBufferOwnership<Buffer>(*bundle.plan, [&](uint64_t bytes, bool uv) {
		vk::Result error;
		auto       resource = Buffer::TryCreate(
            graphics, &scheduler, MemoryUsage::Upload,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
                (uv ? vk::BufferUsageFlags {} : vk::BufferUsageFlagBits::eIndexBuffer),
            bytes, error);
		if (resource && resource->Mapped().empty()) {
			resource.reset();
			error = vk::Result::eErrorMemoryMapFailed;
		}
		return BufferAttempt<Buffer> {std::move(resource), static_cast<VkResult>(error)};
	});
}
} // namespace Libs::Graphics::MeshFallback
