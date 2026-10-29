// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/host_gpu/renderer/meshFallbackRecording.h"

#include "graphics/host_gpu/renderer/commandScheduler.h"
#include "graphics/host_gpu/renderer/meshFallbackRetention.h"
namespace Libs::Graphics::MeshFallback {
RecordingResources RetainResourcesBeforeUse(CommandScheduler& scheduler,
                                            DrawResources&&   resources) {
	// Consume ownership even for invalid/refused preparation. No references may
	// have been recorded, so every failure is immediately safe to roll back.
	auto owned = std::move(resources);
	if (!owned) return {};
	RecordingResources view {scheduler.CurrentTick(), owned.buffers.geometry->Handle(),
	                         owned.buffers.uv ? owned.buffers.uv->Handle() : vk::Buffer {},
	                         owned.descriptors->Set(), owned.descriptors->Layout()};
	if (!RetainBeforeUse(view.tick, std::move(owned),
	                     [&](uint64_t tick, Common::UniqueFunction<void>& cleanup) {
		                     return scheduler.TryRetainForRecording(tick, cleanup);
	                     }))
		return {};
	return view;
}
} // namespace Libs::Graphics::MeshFallback
