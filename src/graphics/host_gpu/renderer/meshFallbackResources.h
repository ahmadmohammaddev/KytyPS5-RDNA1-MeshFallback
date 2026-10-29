// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "graphics/host_gpu/renderer/meshFallbackBuffers.h"

namespace Libs::Graphics::MeshFallback {
enum class DescriptorFailure { None, InvalidInput, HostAllocation, Layout, Pool, Set };

// Dedicated pool: never shares DescriptorHeap reuse or its fatal failure paths.
// Direct destruction requires unsubmitted/completed work. A completion lease
// must retain this object and its buffers in flight; device must outlive both.
class ComputeDescriptors {
public:
	~ComputeDescriptors();
	ComputeDescriptors(const ComputeDescriptors&)            = delete;
	ComputeDescriptors& operator=(const ComputeDescriptors&) = delete;
	[[nodiscard]] static std::unique_ptr<ComputeDescriptors>
	TryCreate(vk::Device device, const ExecutionPlan& plan, const PreparedBuffers<Buffer>& buffers,
	          vk::Result& error, DescriptorFailure& failure);
	[[nodiscard]] vk::DescriptorSetLayout Layout() const { return layout; }
	[[nodiscard]] vk::DescriptorSet       Set() const { return set; }

private:
	explicit ComputeDescriptors(vk::Device device): device(device) {}
	vk::Device              device;
	vk::DescriptorSetLayout layout = nullptr;
	vk::DescriptorPool      pool   = nullptr;
	vk::DescriptorSet       set    = nullptr;
};

// Members destroy in reverse order: descriptors first, then their buffers.
template <class BufferResource, class DescriptorResource>
struct PreparedResources {
	PreparedResources()                             = default;
	PreparedResources(PreparedResources&&) noexcept = default;
	// Replacing an owner member-by-member would destroy buffers before their
	// descriptor pool. Require explicit scope destruction and move construction.
	PreparedResources&                  operator=(PreparedResources&&) = delete;
	PreparedBuffers<BufferResource>     buffers;
	std::unique_ptr<DescriptorResource> descriptors;
	VkResult                            error   = VK_ERROR_INITIALIZATION_FAILED;
	DescriptorFailure                   failure = DescriptorFailure::InvalidInput;
	explicit                            operator bool() const {
        return bool(buffers) && bool(descriptors) && error == VK_SUCCESS &&
               failure == DescriptorFailure::None;
	}
};

template <class BufferResource, class DescriptorResource, class Factory>
PreparedResources<BufferResource, DescriptorResource>
AttachDescriptors(PreparedBuffers<BufferResource>&& buffers, Factory create) {
	// Consume even on failure: no partial buffers escape the transaction.
	auto                                                  local = std::move(buffers);
	PreparedResources<BufferResource, DescriptorResource> out;
	if (!local) {
		out.error = local.error == VK_SUCCESS ? VK_ERROR_INITIALIZATION_FAILED : local.error;
		return out;
	}
	vk::Result error       = vk::Result::eErrorInitializationFailed;
	auto       failure     = DescriptorFailure::InvalidInput;
	auto       descriptors = create(local, error, failure);
	if (!descriptors || error != vk::Result::eSuccess || failure != DescriptorFailure::None) {
		out.error =
		    error == vk::Result::eSuccess ? VK_ERROR_INITIALIZATION_FAILED : VkResult(error);
		out.failure =
		    failure == DescriptorFailure::None ? DescriptorFailure::InvalidInput : failure;
		return out;
	}
	out.buffers     = std::move(local);
	out.descriptors = std::move(descriptors);
	out.error       = VK_SUCCESS;
	out.failure     = DescriptorFailure::None;
	return out;
}
using DrawResources = PreparedResources<Buffer, ComputeDescriptors>;
[[nodiscard]] DrawResources PrepareResources(GraphicContext& graphics, CommandScheduler& scheduler,
                                             const CompileBundleResult& bundle);
} // namespace Libs::Graphics::MeshFallback
