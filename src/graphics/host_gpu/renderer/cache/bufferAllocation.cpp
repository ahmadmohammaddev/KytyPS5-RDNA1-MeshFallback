// SPDX-License-Identifier: GPL-2.0-only
// Buffer allocation/host memory is separated from command recording so the
// recoverable boundary can be tested against real VMA without a renderer.
#include "graphics/host_gpu/renderer/cache/streamBuffer.h"
#include "graphics/host_gpu/renderer/cache/bufferAllocationRetry.h"
#include "common/assert.h"
#include "common/logging/log.h"
#include "common/profiler.h"
#include "common/ramStats.h"
#include "graphics/host_gpu/graphicContext.h"
#include "graphics/host_gpu/vramStats.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>
#include <fmt/format.h>
namespace Libs::Graphics {
namespace {
[[nodiscard]] VmaAllocationCreateFlags AllocationFlags(MemoryUsage usage) {
	switch (usage) {
		case MemoryUsage::Upload:
		case MemoryUsage::Stream:
			return VMA_ALLOCATION_CREATE_MAPPED_BIT |
			       VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		case MemoryUsage::Download:
			return VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		case MemoryUsage::DeviceLocal: return {};
	}
	return {};
}

[[nodiscard]] VmaMemoryUsage AllocationUsage(MemoryUsage usage) {
	switch (usage) {
		case MemoryUsage::DeviceLocal:
		case MemoryUsage::Stream: return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		case MemoryUsage::Upload:
		case MemoryUsage::Download: return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
	}
	return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
}

}
Buffer::Buffer(EmptyTag, GraphicContext& graphics, CommandScheduler* scheduler, MemoryUsage usage,
               uint64_t cpu_address, uint64_t size) noexcept
    : m_graphics(&graphics), m_scheduler(scheduler), m_usage(usage), m_cpu_address(cpu_address),
      m_size(size) {}

Buffer::Buffer(GraphicContext& graphics, CommandScheduler& scheduler, MemoryUsage usage,
               uint64_t cpu_address, vk::BufferUsageFlags flags, uint64_t size,
               bool transfer_shared, bool host_cached, bool sparse_residency)
    : Buffer(EmptyTag {}, graphics, &scheduler, usage, cpu_address, size) {
	KYTY_PROFILER_FUNCTION();
	EXIT_IF(graphics.allocator == nullptr || size == 0);
	const auto result = Initialize(flags, transfer_shared, host_cached, sparse_residency);
	EXIT_NOT_IMPLEMENTED(result != vk::Result::eSuccess);
}

std::unique_ptr<Buffer> Buffer::TryCreate(GraphicContext& graphics, CommandScheduler* scheduler,
                                          MemoryUsage usage, vk::BufferUsageFlags flags,
                                          uint64_t size, vk::Result& error) {
	error = vk::Result::eErrorInitializationFailed;
	if (!graphics.allocator || !graphics.device || !size || !flags ||
	    (usage != MemoryUsage::DeviceLocal && usage != MemoryUsage::Upload &&
	     usage != MemoryUsage::Download && usage != MemoryUsage::Stream))
		return {};
	if (flags & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
		error = vk::Result::eErrorFeatureNotPresent;
		return {};
	}
	std::unique_ptr<Buffer> buffer(new (std::nothrow)
	                                   Buffer(EmptyTag {}, graphics, scheduler, usage, 0, size));
	if (!buffer) {
		error = vk::Result::eErrorOutOfHostMemory;
		return {};
	}
	error = buffer->Initialize(flags, false, false, false);
	if (error != vk::Result::eSuccess) return {};
	return buffer;
}

vk::Result Buffer::Initialize(vk::BufferUsageFlags flags, bool transfer_shared, bool host_cached,
                              bool sparse_residency) {
	auto&                graphics    = *m_graphics;
	const auto           usage       = m_usage;
	const auto           size        = m_size;
	const auto           cpu_address = m_cpu_address;
	vk::BufferCreateInfo buffer_info {};
	buffer_info.size  = size;
	buffer_info.usage = flags;
	const std::array<uint32_t, 2> families {graphics.queue_family, graphics.transfer_queue_family};
	if (transfer_shared && graphics.transfer_queue != nullptr) {
		buffer_info.sharingMode           = vk::SharingMode::eConcurrent;
		buffer_info.queueFamilyIndexCount = static_cast<uint32_t>(families.size());
		buffer_info.pQueueFamilyIndices   = families.data();
	}
	if (sparse_residency) {
		// No memory here: the owner binds pages to the ranges it uses (unbound ranges read zero).
		EXIT_IF(!graphics.sparse_residency_buffer_enabled || usage != MemoryUsage::DeviceLocal ||
		        host_cached ||
		        static_cast<bool>(flags & vk::BufferUsageFlagBits::eShaderDeviceAddress));
		buffer_info.flags =
		    vk::BufferCreateFlagBits::eSparseBinding | vk::BufferCreateFlagBits::eSparseResidency;
		vk::Buffer sparse_buffer = nullptr;
		RequireVulkanSuccess(graphics.device.createBuffer(&buffer_info, nullptr, &sparse_buffer),
		                     "create sparse residency buffer");
		m_buffer = sparse_buffer;
		m_sparse = true;
		return vk::Result::eSuccess;
	}

	// Buffers with device addresses (every guest buffer) take dedicated memory, a driver allocation
	// each (~100 us on Windows; chenxiao07/KytyPS5 af29b93d7: 8400 buffers while a save loads,
	// 100-400 in a frame that streams a new area). KYTY_BDA_SHARED_BLOCKS (default on; =0 dedicated
	// memory for every one) lets the ones up to 64 MiB share VMA's memory blocks instead (256 MiB
	// blocks; a buffer past half a block gets its own memory anyway, so the cap keeps a block at
	// four or more buffers; VMA frees an empty block, keeping at most one per memory type, or none
	// over budget): nothing needs the allocation's offset to be 0. The BDA page table stores
	// BufferDeviceAddress() + page offset per page (BufferCache::ChangeRegister), and
	// vkGetBufferDeviceAddress already includes the placement; mapped pointers come from VMA's
	// pMappedData and flushes/invalidates go through vmaFlush/InvalidateAllocation, both
	// allocation-relative. The allocator was created with
	// VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT, so its blocks carry the device-address flag.
	static const bool shared_blocks = [] {
		const auto* value = std::getenv("KYTY_BDA_SHARED_BLOCKS");
		return value == nullptr || std::strcmp(value, "0") != 0;
	}();
	static constexpr uint64_t SharedBlockMax = 64ull * 1024 * 1024;
	const bool with_bda = bool(flags & vk::BufferUsageFlagBits::eShaderDeviceAddress);
	const VmaAllocationCreateFlags bda_flag = with_bda && !(shared_blocks && size <= SharedBlockMax)
	                                              ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
	                                              : 0;
	VmaAllocationCreateInfo        allocation_info {};
	allocation_info.flags =
	    VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT | bda_flag | AllocationFlags(usage);
	allocation_info.usage          = AllocationUsage(usage);
	allocation_info.preferredFlags = usage == MemoryUsage::DeviceLocal
	                                     ? VkMemoryPropertyFlags {}
	                                     : VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	if (host_cached) {
		allocation_info.flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT | bda_flag |
		                        VMA_ALLOCATION_CREATE_MAPPED_BIT |
		                        VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		allocation_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		allocation_info.requiredFlags =
		    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		allocation_info.preferredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}

	VmaAllocationInfo allocation_result {};
	VkBuffer          native_buffer = VK_NULL_HANDLE;
	const auto        create        = [&] {
        return static_cast<vk::Result>(
            vmaCreateBuffer(graphics.allocator, static_cast<const VkBufferCreateInfo*>(buffer_info),
		                                  &allocation_info, &native_buffer, &m_allocation, &allocation_result));
	};
	size_t     released = 0;
	const auto result   = Detail::RetryBufferAllocation(
        create,
        [&] {
            graphics.LogMemoryBudget();
            released = graphics.ReleaseRetainedMemory();
        },
        [&] {
            allocation_info.flags &=
                ~static_cast<VmaAllocationCreateFlags>(VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT);
        },
        [&](vk::Result result, bool over_budget) {
            static std::atomic<uint32_t> reported {0};
            if (reported.fetch_add(1, std::memory_order_relaxed) < 8) {
                Log::WriteToConsoleAndLog(fmt::format(
                    "Kyty VRAM: a {}-byte buffer did not fit the memory budget; released {} "
			          "retained bytes "
			          "and retried: {}\n",
                    size, released,
                    result != vk::Result::eSuccess
			              ? vk::to_string(result)
			              : std::string(over_budget
			                                ? "created past the budget (may page to system memory)"
			                                : "created")));
            }
        },
        vk::Result::eSuccess);
	if (result != vk::Result::eSuccess) return result;

	m_buffer = native_buffer;
	if (with_bda) {
		vk::BufferDeviceAddressInfo address_info {};
		address_info.buffer = m_buffer;
		m_device_address    = graphics.device.getBufferAddress(address_info);
		EXIT_IF(m_device_address == 0);
	}

	VkMemoryPropertyFlags properties = 0;
	vmaGetAllocationMemoryProperties(graphics.allocator, m_allocation, &properties);
	m_coherent = (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
	if (allocation_result.pMappedData != nullptr) {
		m_mapped = {static_cast<uint8_t*>(allocation_result.pMappedData),
		            static_cast<size_t>(size)};
		Common::RamStats::Range((properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0
		                            ? "Vulkan mapped device buffer"
		                            : "Vulkan mapped system buffer",
		                        m_mapped.data(), size);
	}
	if (VramStats::Enabled()) {
		const auto kind     = cpu_address != 0                    ? VramStats::Kind::GuestBuffer
		                      : usage != MemoryUsage::DeviceLocal ? VramStats::Kind::RingBuffer
		                                                          : VramStats::Kind::OtherBuffer;
		m_vram_bytes        = static_cast<uint64_t>(allocation_result.size);
		m_vram_kind         = static_cast<uint8_t>(kind);
		m_vram_device_local = (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0;
		VramStats::Note(kind, m_vram_device_local, static_cast<int64_t>(m_vram_bytes));
		VramStats::RegisterBuffer(this, {m_vram_bytes, cpu_address, static_cast<uint8_t>(usage),
		                                 m_vram_device_local, host_cached});
	}
	return vk::Result::eSuccess;
}

Buffer::~Buffer() {
	if (m_vram_bytes != 0) {
		VramStats::Note(static_cast<VramStats::Kind>(m_vram_kind), m_vram_device_local,
		                -static_cast<int64_t>(m_vram_bytes));
		VramStats::UnregisterBuffer(this);
	}
	if (m_buffer != nullptr) {
		if (m_sparse) {
			// The owner frees the bound pages (after this, or before: the buffer is no longer used).
			m_graphics->device.destroyBuffer(m_buffer, nullptr);
		} else {
			vmaDestroyBuffer(m_graphics->allocator, m_buffer, m_allocation);
		}
	}
}

vk::DeviceAddress Buffer::BufferDeviceAddress() const noexcept {
	EXIT_IF(m_device_address == 0);
	return m_device_address;
}

bool Buffer::IsInBounds(uint64_t address, uint64_t size) const noexcept {
	return address >= m_cpu_address && size <= Size() && address - m_cpu_address <= Size() - size;
}

void Buffer::MarkContentWritten() {
	// Reusing a revision would make a retained GPU-content result appear current again.
	EXIT_IF(m_content_revision == UINT64_MAX);
	++m_content_revision;
}

void Buffer::Flush(uint64_t offset, uint64_t size) {
	EXIT_IF(m_mapped.empty() || offset > Size() || size > Size() - offset);
	if (!IsCoherent() && size != 0) {
		const auto result =
		    vmaFlushAllocation(m_graphics->allocator, m_allocation, offset, size);
		EXIT_NOT_IMPLEMENTED(static_cast<vk::Result>(result) != vk::Result::eSuccess);
	}
}

void Buffer::Invalidate(uint64_t offset, uint64_t size) {
	EXIT_IF(m_usage != MemoryUsage::Download || offset > Size() || size > Size() - offset);
	if (!IsCoherent() && size != 0) {
		const auto result =
		    vmaInvalidateAllocation(m_graphics->allocator, m_allocation, offset, size);
		EXIT_NOT_IMPLEMENTED(static_cast<vk::Result>(result) != vk::Result::eSuccess);
	}
}

}
