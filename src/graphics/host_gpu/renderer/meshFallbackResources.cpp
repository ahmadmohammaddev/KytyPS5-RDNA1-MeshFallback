// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/host_gpu/renderer/meshFallbackResources.h"

#include "graphics/host_gpu/graphicContext.h"
namespace Libs::Graphics::MeshFallback {
DrawResources PrepareResources(GraphicContext& graphics, CommandScheduler& scheduler,
                               const CompileBundleResult& bundle) {
	auto buffers = PrepareBuffers(graphics, scheduler, bundle);
	return AttachDescriptors<Buffer, ComputeDescriptors>(
	    std::move(buffers), [&](const auto& local, vk::Result& error, DescriptorFailure& failure) {
		    return ComputeDescriptors::TryCreate(graphics.device, *bundle.plan, local, error,
		                                         failure);
	    });
}
} // namespace Libs::Graphics::MeshFallback
