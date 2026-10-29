// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/host_gpu/renderer/meshFallbackExecutionPlan.h"

#include <memory>
#include <utility>
#include <vulkan/vulkan_core.h>
namespace Libs::Graphics::MeshFallback {
enum class BufferFailure { None, InvalidPlan, Geometry, Uv };
template <class Resource>
struct BufferAttempt {
	std::unique_ptr<Resource> resource;
	VkResult                  error = VK_ERROR_INITIALIZATION_FAILED;
};
// Direct owner of UNSUBMITTED resources. Before recording GPU references,
// transfer the complete owner to a completion lease (meshFallbackRetention.h).
// This type alone makes no retirement claim; destruction is immediate.
template <class Resource>
struct PreparedBuffers {
	std::unique_ptr<Resource> geometry, uv;
	VkResult                  error       = VK_ERROR_INITIALIZATION_FAILED;
	BufferFailure             failure     = BufferFailure::InvalidPlan;
	bool                      uv_required = false;
	explicit                  operator bool() const {
        return error == VK_SUCCESS && failure == BufferFailure::None && bool(geometry) &&
               (!uv_required || bool(uv));
	}
};
// Factory takes byte size and whether this is UV. Same transaction used by the
// production Buffer adapter and CPU fault tests; no test-only global behavior.
template <class Resource, class Factory>
PreparedBuffers<Resource> PrepareBufferOwnership(const ExecutionPlan& plan, Factory create) {
	PreparedBuffers<Resource> out;
	const auto                canonical =
	    CheckedExecutionLayout(plan.vertices, plan.primitives, 64, 64, plan.uv_bytes != 0, 64);
	if (!canonical || !(*canonical == plan)) return out;
	auto geometry = create(plan.geometry_bytes, false);
	auto error    = [](const auto& attempt) {
        return attempt.error == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : attempt.error;
	};
	if (geometry.error != VK_SUCCESS || !geometry.resource) {
		out.error   = error(geometry);
		out.failure = BufferFailure::Geometry;
		return out;
	}
	if (plan.uv_bytes) {
		auto uv = create(plan.uv_bytes, true);
		if (uv.error != VK_SUCCESS || !uv.resource) {
			out.error   = error(uv);
			out.failure = BufferFailure::Uv;
			return out;
		}
		out.uv = std::move(uv.resource);
	}
	out.uv_required = plan.uv_bytes != 0;
	out.geometry    = std::move(geometry.resource);
	out.error       = VK_SUCCESS;
	out.failure     = BufferFailure::None;
	return out;
}
} // namespace Libs::Graphics::MeshFallback
