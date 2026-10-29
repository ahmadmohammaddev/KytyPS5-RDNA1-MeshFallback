// SPDX-License-Identifier: GPL-2.0-only
#include "graphics/host_gpu/renderer/meshFallbackResources.h"

#include <array>
#include <new>

namespace Libs::Graphics::MeshFallback {
ComputeDescriptors::~ComputeDescriptors() {
	if (pool) device.destroyDescriptorPool(pool);
	if (layout) device.destroyDescriptorSetLayout(layout);
}

std::unique_ptr<ComputeDescriptors>
ComputeDescriptors::TryCreate(vk::Device device, const ExecutionPlan& plan,
                              const PreparedBuffers<Buffer>& buffers, vk::Result& error,
                              DescriptorFailure& failure) {
	error   = vk::Result::eErrorInitializationFailed;
	failure = DescriptorFailure::InvalidInput;
	const auto canonical =
	    CheckedExecutionLayout(plan.vertices, plan.primitives, 64, 64, plan.uv_bytes != 0, 64);
	if (!device || !canonical || *canonical != plan || !buffers ||
	    buffers.uv_required != (plan.uv_bytes != 0) || !buffers.geometry->Handle() ||
	    buffers.geometry->Size() != plan.geometry_bytes ||
	    (plan.uv_bytes && (!buffers.uv->Handle() || buffers.uv->Size() != plan.uv_bytes)) ||
	    (!plan.uv_bytes && buffers.uv))
		return {};
	auto owned = std::unique_ptr<ComputeDescriptors>(new (std::nothrow) ComputeDescriptors(device));
	if (!owned) {
		error   = vk::Result::eErrorOutOfHostMemory;
		failure = DescriptorFailure::HostAllocation;
		return {};
	}
	const uint32_t                                count = plan.uv_bytes ? 2 : 1;
	std::array<vk::DescriptorSetLayoutBinding, 2> bindings {};
	for (uint32_t i = 0; i < count; ++i) {
		bindings[i].binding         = i + 1;
		bindings[i].descriptorType  = vk::DescriptorType::eStorageBuffer;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags      = vk::ShaderStageFlagBits::eCompute;
	}
	vk::DescriptorSetLayoutCreateInfo layout {};
	layout.bindingCount = count;
	layout.pBindings    = bindings.data();
	failure             = DescriptorFailure::Layout;
	error               = device.createDescriptorSetLayout(&layout, nullptr, &owned->layout);
	if (error != vk::Result::eSuccess) return {};
	const vk::DescriptorPoolSize size(vk::DescriptorType::eStorageBuffer, count);
	vk::DescriptorPoolCreateInfo pool {};
	pool.maxSets       = 1;
	pool.poolSizeCount = 1;
	pool.pPoolSizes    = &size;
	failure            = DescriptorFailure::Pool;
	error              = device.createDescriptorPool(&pool, nullptr, &owned->pool);
	if (error != vk::Result::eSuccess) return {};
	vk::DescriptorSetAllocateInfo allocate {};
	allocate.descriptorPool     = owned->pool;
	allocate.descriptorSetCount = 1;
	allocate.pSetLayouts        = &owned->layout;
	failure                     = DescriptorFailure::Set;
	error                       = device.allocateDescriptorSets(&allocate, &owned->set);
	if (error != vk::Result::eSuccess) return {};
	const std::array<vk::DescriptorBufferInfo, 2> infos {
	    {{buffers.geometry->Handle(), 0, plan.geometry_bytes},
	     {buffers.uv ? buffers.uv->Handle() : vk::Buffer {}, 0, plan.uv_bytes}}};
	std::array<vk::WriteDescriptorSet, 2> writes {};
	for (uint32_t i = 0; i < count; ++i) {
		writes[i].dstSet          = owned->set;
		writes[i].dstBinding      = i + 1;
		writes[i].descriptorCount = 1;
		writes[i].descriptorType  = vk::DescriptorType::eStorageBuffer;
		writes[i].pBufferInfo     = &infos[i];
	}
	// Vulkan update is void: prerequisites are checked above; validation is
	// retained through cleanup in the real-device tests. No invented status.
	device.updateDescriptorSets(count, writes.data(), 0, nullptr);
	failure = DescriptorFailure::None;
	error   = vk::Result::eSuccess;
	return owned;
}
} // namespace Libs::Graphics::MeshFallback
