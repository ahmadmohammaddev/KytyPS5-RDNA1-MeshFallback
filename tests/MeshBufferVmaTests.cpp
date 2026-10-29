// SPDX-License-Identifier: GPL-2.0-only
// REAL Buffer + REAL VMA. Only vkCreateBuffer refusal is injected in this
// process; successful calls and destruction go to the actual driver. No
// physical OOM test.
#include "graphics/host_gpu/graphicContext.h"
#include "graphics/host_gpu/renderer/cache/streamBuffer.h"
#include "graphics/host_gpu/renderer/meshFallbackBufferPreparation.h"
#include "graphics/host_gpu/renderer/meshFallbackResources.h"
#include "graphics/host_gpu/renderer/meshFallbackRetention.h"
#include <filesystem>
#include <fstream>
#include <spirv-tools/libspirv.hpp>
#ifdef _WIN32
#include <windows.h>
#undef MemoryBarrier
#endif
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <memory>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
// Link-only guard for the legacy sparse constructor's fatal diagnostics. The
// tested non-sparse/non-BDA factory MUST NEVER enter it. Emulator links the
// real diagnostic implementation; no VMA/driver allocation function is stubbed
// here.
namespace Libs::Graphics {
// Unreachable renderer device-loss reporter: the broker stays disabled here.
void DumpDeviceLossDiagnostics(GraphicContext &, uint64_t, bool) {
  std::fputs(
      "FAIL: unexpected renderer device-loss reporter in allocation test\n",
      stderr);
  std::abort();
}
void RequireVulkanSuccess(vk::Result, const char *) {
  std::fputs("FAIL: unexpected legacy sparse/fatal path in recoverable test\n",
             stderr);
  std::abort();
}
} // namespace Libs::Graphics
namespace B = Libs::Graphics::MeshFallback;
using namespace Libs::Graphics;
namespace {
int failures = 0, created = 0, destroyed = 0, refused = 0;
bool inject = false;
std::atomic<int> errors = 0, warnings = 0;
PFN_vkCreateBuffer real_create = nullptr;
PFN_vkDestroyBuffer real_destroy = nullptr;
PFN_vkCreateDescriptorSetLayout real_layout = nullptr;
PFN_vkDestroyDescriptorSetLayout real_destroy_layout = nullptr;
PFN_vkCreateDescriptorPool real_pool = nullptr;
PFN_vkDestroyDescriptorPool real_destroy_pool = nullptr;
PFN_vkAllocateDescriptorSets real_sets = nullptr;
PFN_vkUpdateDescriptorSets real_update = nullptr;
int descriptor_fail = 0, layouts = 0, pools = 0, updates = 0;
int descriptor_count = 0;
VkBuffer expected_buffers[2]{};
void Check(bool ok, const char *why) {
  if (!ok) {
    ++failures;
    std::printf("[vma] FAIL: %s\n", why);
  }
}
VKAPI_ATTR VkResult VKAPI_CALL
Layout(VkDevice d, const VkDescriptorSetLayoutCreateInfo *info,
       const VkAllocationCallbacks *a, VkDescriptorSetLayout *out) {
  Check(info->bindingCount == descriptor_count, "exact layout binding count");
  for (uint32_t i = 0; i < info->bindingCount; ++i)
    Check(info->pBindings[i].binding == i + 1 &&
              info->pBindings[i].descriptorCount == 1 &&
              info->pBindings[i].descriptorType ==
                  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
              info->pBindings[i].stageFlags == VK_SHADER_STAGE_COMPUTE_BIT,
          "geometry1/UV2 storage-compute layout, no input0");
  if (descriptor_fail == 1) {
    *out = VK_NULL_HANDLE;
    return VK_ERROR_OUT_OF_HOST_MEMORY;
  }
  auto r = real_layout(d, info, a, out);
  if (r == VK_SUCCESS)
    ++layouts;
  return r;
}
VKAPI_ATTR void VKAPI_CALL DestroyLayout(VkDevice d, VkDescriptorSetLayout l,
                                         const VkAllocationCallbacks *a) {
  --layouts;
  real_destroy_layout(d, l, a);
}
VKAPI_ATTR VkResult VKAPI_CALL Pool(VkDevice d,
                                    const VkDescriptorPoolCreateInfo *info,
                                    const VkAllocationCallbacks *a,
                                    VkDescriptorPool *out) {
  Check(info->maxSets == 1 && info->poolSizeCount == 1 &&
            info->pPoolSizes[0].descriptorCount == descriptor_count &&
            info->pPoolSizes[0].type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        "dedicated exact-size pool");
  if (descriptor_fail == 2) {
    *out = VK_NULL_HANDLE;
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  }
  auto r = real_pool(d, info, a, out);
  if (r == VK_SUCCESS)
    ++pools;
  return r;
}
VKAPI_ATTR void VKAPI_CALL DestroyPool(VkDevice d, VkDescriptorPool p,
                                       const VkAllocationCallbacks *a) {
  --pools;
  real_destroy_pool(d, p, a);
}
VKAPI_ATTR VkResult VKAPI_CALL Sets(VkDevice d,
                                    const VkDescriptorSetAllocateInfo *info,
                                    VkDescriptorSet *out) {
  if (descriptor_fail == 3) {
    *out = VK_NULL_HANDLE;
    return VK_ERROR_OUT_OF_POOL_MEMORY;
  }
  return real_sets(d, info, out);
}
VKAPI_ATTR void VKAPI_CALL Update(VkDevice d, uint32_t count,
                                  const VkWriteDescriptorSet *writes,
                                  uint32_t copies,
                                  const VkCopyDescriptorSet *copy) {
  ++updates;
  Check(count == descriptor_count && copies == 0,
        "exact descriptor update count");
  for (uint32_t i = 0; i < count; ++i)
    Check(writes[i].dstBinding == i + 1 && writes[i].descriptorCount == 1 &&
              writes[i].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
              writes[i].pBufferInfo->buffer == expected_buffers[i] &&
              writes[i].pBufferInfo->offset == 0 &&
              writes[i].pBufferInfo->range == (i ? 544 : 1840),
          "exact full guarded buffer ranges/handles bound");
  real_update(d, count, writes, copies, copy);
}
VKAPI_ATTR VkResult VKAPI_CALL Create(VkDevice d, const VkBufferCreateInfo *i,
                                      const VkAllocationCallbacks *a,
                                      VkBuffer *b) {
  if (inject) {
    ++refused;
    *b = VK_NULL_HANDLE;
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  }
  auto r = real_create(d, i, a, b);
  if (r == VK_SUCCESS)
    ++created;
  return r;
}
VKAPI_ATTR void VKAPI_CALL Destroy(VkDevice d, VkBuffer b,
                                   const VkAllocationCallbacks *a) {
  if (b)
    ++destroyed;
  real_destroy(d, b, a);
}
VKAPI_ATTR VkBool32 VKAPI_CALL
Validation(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
           vk::DebugUtilsMessageTypeFlagsEXT,
           const vk::DebugUtilsMessengerCallbackDataEXT *data, void *) {
  if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
    ++errors;
  if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
    ++warnings;
  std::printf("[vma] validation: %s\n", data->pMessage);
  return VK_FALSE;
}
struct Runtime {
  vk::detail::DynamicLoader loader;
  std::unique_ptr<GraphicContext> graphics = std::make_unique<GraphicContext>();
  vk::DebugUtilsMessengerEXT messenger = nullptr;
  ~Runtime() {
    if (graphics->device) {
      Check(graphics->device.waitIdle() == vk::Result::eSuccess,
            "idle before cleanup");
      if (graphics->allocator) {
        vmaDestroyAllocator(graphics->allocator);
        graphics->allocator = nullptr;
      }
      graphics->device.destroy();
      graphics->device = nullptr;
    }
    if (messenger)
      graphics->instance.destroyDebugUtilsMessengerEXT(messenger);
    if (graphics->instance)
      graphics->instance.destroy();
  }
  int Init(bool api13, bool require_rx) {
    auto get = loader.getProcAddress<PFN_vkGetInstanceProcAddr>(
        "vkGetInstanceProcAddr");
    if (!get) {
      std::puts("[vma] SKIPPED: Vulkan loader unavailable");
      return 41;
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(get);
    uint32_t count = 0;
    if (vk::enumerateInstanceLayerProperties(&count, nullptr) !=
        vk::Result::eSuccess) {
      Check(false, "layer enumeration count");
      return 1;
    }
    std::vector<vk::LayerProperties> layers(count);
    if (vk::enumerateInstanceLayerProperties(&count, layers.data()) !=
        vk::Result::eSuccess) {
      Check(false, "layer enumeration data");
      return 1;
    }
    if (std::none_of(layers.begin(), layers.end(), [](const auto &l) {
          return std::strcmp(l.layerName.data(),
                             "VK_LAYER_KHRONOS_validation") == 0;
        })) {
      std::puts("[vma] SKIPPED: mandatory Khronos validation unavailable");
      return 41;
    }
    const char *layer = "VK_LAYER_KHRONOS_validation";
    const char *extensions[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                                VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME};
    const auto sync =
        vk::ValidationFeatureEnableEXT::eSynchronizationValidation;
    vk::DebugUtilsMessengerCreateInfoEXT debug{};
    debug.sType = vk::StructureType::eDebugUtilsMessengerCreateInfoEXT;
    debug.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    debug.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    debug.pfnUserCallback = Validation;
    vk::ValidationFeaturesEXT validation{};
    validation.sType = vk::StructureType::eValidationFeaturesEXT;
    validation.pNext = &debug;
    validation.enabledValidationFeatureCount = 1;
    validation.pEnabledValidationFeatures = &sync;
    const uint32_t api = api13 ? VK_API_VERSION_1_3 : VK_API_VERSION_1_2;
    vk::ApplicationInfo app{};
    app.sType = vk::StructureType::eApplicationInfo;
    app.pApplicationName = "MeshBufferVmaTests";
    app.apiVersion = api;
    vk::InstanceCreateInfo info{};
    info.sType = vk::StructureType::eInstanceCreateInfo;
    info.pNext = &validation;
    info.pApplicationInfo = &app;
    info.enabledLayerCount = 1;
    info.ppEnabledLayerNames = &layer;
    info.enabledExtensionCount = 2;
    info.ppEnabledExtensionNames = extensions;
    auto result = vk::createInstance(&info, nullptr, &graphics->instance);
    if (result == vk::Result::eErrorExtensionNotPresent ||
        result == vk::Result::eErrorIncompatibleDriver) {
      std::puts("[vma] SKIPPED: mandatory instance prerequisite unavailable");
      return 41;
    }
    if (result != vk::Result::eSuccess) {
      Check(false, "instance creation");
      return 1;
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(graphics->instance);
    if (graphics->instance.createDebugUtilsMessengerEXT(
            &debug, nullptr, &messenger) != vk::Result::eSuccess) {
      Check(false, "validation messenger");
      return 1;
    }
    if (graphics->instance.enumeratePhysicalDevices(&count, nullptr) !=
        vk::Result::eSuccess) {
      Check(false, "device enumeration count");
      return 1;
    }
    std::vector<vk::PhysicalDevice> devices(count);
    if (graphics->instance.enumeratePhysicalDevices(&count, devices.data()) !=
        vk::Result::eSuccess) {
      Check(false, "device enumeration data");
      return 1;
    }
    for (auto physical : devices) {
      auto props = physical.getProperties();
      std::printf(
          "[vma] device: %s vendor=%04X id=%04X API=%u.%u.%u driver=%u\n",
          props.deviceName.data(), props.vendorID, props.deviceID,
          VK_API_VERSION_MAJOR(props.apiVersion),
          VK_API_VERSION_MINOR(props.apiVersion),
          VK_API_VERSION_PATCH(props.apiVersion), props.driverVersion);
      vk::PhysicalDeviceDriverProperties driver{};
      vk::PhysicalDeviceProperties2 details{};
      details.pNext = &driver;
      physical.getProperties2(&details);
      std::printf("[vma] driver: %s / %s / id=%u\n", driver.driverName.data(),
                  driver.driverInfo.data(), uint32_t(driver.driverID));
      if (require_rx && (props.vendorID != 0x1002 ||
                         !std::strstr(props.deviceName.data(), "RX 5700 XT") ||
                         driver.driverID != vk::DriverId::eAmdProprietary))
        continue;
      if (props.apiVersion < api)
        continue;
      uint32_t queues = 0;
      physical.getQueueFamilyProperties(&queues, nullptr);
      std::vector<vk::QueueFamilyProperties> families(queues);
      physical.getQueueFamilyProperties(&queues, families.data());
      for (uint32_t i = 0; i < queues; ++i)
        if ((families[i].queueFlags &
             (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute)) ==
            (vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute)) {
          graphics->physical_device = physical;
          graphics->queue_family = i;
          break;
        }
      if (graphics->physical_device)
        break;
    }
    if (!graphics->physical_device) {
      if (require_rx) {
        Check(false, "required native RX5700XT strict device unavailable");
        return 1;
      }
      std::puts(
          "[vma] SKIPPED: no device supports requested API (no downgrade)");
      return 41;
    }
    auto props = graphics->physical_device.getProperties();
    std::printf("[vma] selected: %s; %s\n", props.deviceName.data(),
                api13 ? "strict Vulkan1.3"
                      : "Vulkan1.2 AUXILIARY, not RDNA1/1.3 acceptance");
    float priority = 1;
    vk::PhysicalDeviceVulkan12Features available{};
    vk::PhysicalDeviceFeatures2 features{};
    features.pNext = &available;
    graphics->physical_device.getFeatures2(&features);
    if (!available.timelineSemaphore) {
      std::puts("[vma] SKIPPED: timeline semaphore feature unavailable");
      return 41;
    }
    vk::PhysicalDeviceVulkan12Features enabled{};
    enabled.timelineSemaphore = true;
    vk::DeviceQueueCreateInfo queue{};
    queue.sType = vk::StructureType::eDeviceQueueCreateInfo;
    queue.queueFamilyIndex = graphics->queue_family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    vk::DeviceCreateInfo device{};
    device.sType = vk::StructureType::eDeviceCreateInfo;
    device.pNext = &enabled;
    device.queueCreateInfoCount = 1;
    device.pQueueCreateInfos = &queue;
    if (graphics->physical_device.createDevice(
            &device, nullptr, &graphics->device) != vk::Result::eSuccess) {
      Check(false, "device creation");
      return 1;
    }
    VULKAN_HPP_DEFAULT_DISPATCHER.init(graphics->device);
    graphics->queue = graphics->device.getQueue(graphics->queue_family, 0);
    real_create = VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateBuffer;
    real_destroy = VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyBuffer;
    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = get;
    functions.vkGetDeviceProcAddr =
        VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;
    functions.vkCreateBuffer = Create;
    functions.vkDestroyBuffer = Destroy;
    VmaAllocatorCreateInfo allocator{};
    allocator.instance = graphics->instance;
    allocator.physicalDevice = graphics->physical_device;
    allocator.device = graphics->device;
    allocator.vulkanApiVersion = api;
    allocator.pVulkanFunctions = &functions;
    if (vmaCreateAllocator(&allocator, &graphics->allocator) != VK_SUCCESS) {
      Check(false, "VMA initialization");
      return 1;
    }
    return 0;
  }
};
void Cases(GraphicContext &graphics) {
  auto allocations = [&] {
    VmaTotalStatistics s{};
    vmaCalculateStatistics(graphics.allocator, &s);
    return s.total.statistics.allocationCount;
  };
  for (bool zero : {false, true})
    for (bool uv : {false, true})
      for (int fail_at : {0, 1, 2}) {
        if (fail_at == 2 && !uv)
          continue;
        const auto before = allocations();
        const int destroyed_before = destroyed;
        int calls = 0;
        const auto plan = *B::CheckedExecutionLayout(zero ? 0 : 3, zero ? 0 : 1,
                                                     64, 64, uv, 64);
        {
          auto owned = B::PrepareBufferOwnership<Buffer>(
              plan, [&](uint64_t size, bool is_uv) {
                inject = ++calls == fail_at;
                vk::Result error;
                auto buffer = Buffer::TryCreate(
                    graphics, nullptr, MemoryUsage::Upload,
                    vk::BufferUsageFlagBits::eStorageBuffer |
                        vk::BufferUsageFlagBits::eVertexBuffer |
                        (is_uv ? vk::BufferUsageFlags{}
                               : vk::BufferUsageFlagBits::eIndexBuffer),
                    size, error);
                inject = false;
                return B::BufferAttempt<Buffer>{std::move(buffer),
                                                static_cast<VkResult>(error)};
              });
          if (fail_at)
            Check(!owned && !owned.geometry && !owned.uv &&
                      owned.error == VK_ERROR_OUT_OF_DEVICE_MEMORY,
                  "real VMA refusal is recoverable and atomic");
          else {
            Check(bool(owned) && owned.geometry->Handle() != nullptr,
                  "real Buffer success");
            for (auto *b : {owned.geometry.get(), owned.uv.get()})
              if (b) {
                Check(b->Mapped().size() == b->Size(),
                      "mapped span covers resource");
                std::fill(b->Mapped().begin(), b->Mapped().end(), 0x53);
                b->Flush(0, b->Size());
              }
          }
          if (fail_at)
            Check(allocations() == before &&
                      destroyed - destroyed_before == (fail_at == 2 ? 1 : 0),
                  "real partial allocation released once before return");
        }
        Check(allocations() == before,
              "no live VMA allocation after ownership scope");
      }
  vk::Result error;
  const auto attempts = refused + created;
  auto invalid =
      Buffer::TryCreate(graphics, nullptr, MemoryUsage::Upload,
                        vk::BufferUsageFlagBits::eStorageBuffer, 0, error);
  Check(!invalid && error == vk::Result::eErrorInitializationFailed,
        "zero allocation rejects before driver");
  invalid = Buffer::TryCreate(graphics, nullptr, MemoryUsage::Upload,
                              vk::BufferUsageFlagBits::eShaderDeviceAddress,
                              1840, error);
  Check(!invalid && error == vk::Result::eErrorFeatureNotPresent &&
            refused + created == attempts,
        "BDA outside recoverable subset rejects before driver");
  Check(created == destroyed && refused == 18,
        "all successful native handles destroyed once; six failures exhaust "
        "three real VMA attempts");
  std::printf("[vma] created=%d destroyed=%d injected-refusals=%d\n", created,
              destroyed, refused);
}
void DescriptorCases(GraphicContext &graphics) {
  auto &dispatch = VULKAN_HPP_DEFAULT_DISPATCHER;
  real_layout = dispatch.vkCreateDescriptorSetLayout;
  real_destroy_layout = dispatch.vkDestroyDescriptorSetLayout;
  real_pool = dispatch.vkCreateDescriptorPool;
  real_destroy_pool = dispatch.vkDestroyDescriptorPool;
  real_sets = dispatch.vkAllocateDescriptorSets;
  real_update = dispatch.vkUpdateDescriptorSets;
  dispatch.vkCreateDescriptorSetLayout = Layout;
  dispatch.vkDestroyDescriptorSetLayout = DestroyLayout;
  dispatch.vkCreateDescriptorPool = Pool;
  dispatch.vkDestroyDescriptorPool = DestroyPool;
  dispatch.vkAllocateDescriptorSets = Sets;
  dispatch.vkUpdateDescriptorSets = Update;
  const auto before_created = created;
  for (bool zero : {false, true})
    for (bool uv : {false, true})
      for (int fail : {0, 1, 2, 3}) {
        descriptor_fail = fail;
        descriptor_count = uv ? 2 : 1;
        const auto plan = *B::CheckedExecutionLayout(zero ? 0 : 3, zero ? 0 : 1,
                                                     64, 64, uv, 64);
        auto buffers = B::PrepareBufferOwnership<Buffer>(
            plan, [&](uint64_t size, bool is_uv) {
              vk::Result error;
              auto b = Buffer::TryCreate(
                  graphics, nullptr, MemoryUsage::Upload,
                  vk::BufferUsageFlagBits::eStorageBuffer |
                      vk::BufferUsageFlagBits::eVertexBuffer |
                      (is_uv ? vk::BufferUsageFlags{}
                             : vk::BufferUsageFlagBits::eIndexBuffer),
                  size, error);
              return B::BufferAttempt<Buffer>{std::move(b), VkResult(error)};
            });
        Check(bool(buffers), "descriptor prerequisite real buffers");
        if (!buffers)
          continue;
        auto invalid_plan = plan;
        ++invalid_plan.index_offset;
        vk::Result invalid_error;
        B::DescriptorFailure invalid_failure;
        auto invalid = B::ComputeDescriptors::TryCreate(
            graphics.device, invalid_plan, buffers, invalid_error,
            invalid_failure);
        Check(!invalid &&
                  invalid_error == vk::Result::eErrorInitializationFailed &&
                  invalid_failure == B::DescriptorFailure::InvalidInput &&
                  layouts == 0 && pools == 0,
              "malformed layout rejected before Vulkan setup");
        expected_buffers[0] = buffers.geometry->Handle();
        expected_buffers[1] =
            buffers.uv ? VkBuffer(buffers.uv->Handle()) : VK_NULL_HANDLE;
        const auto destroyed_before = destroyed;
        const auto updates_before = updates;
        {
          auto resources = B::AttachDescriptors<Buffer, B::ComputeDescriptors>(
              std::move(buffers), [&](const auto &local, vk::Result &error,
                                      B::DescriptorFailure &failure) {
                return B::ComputeDescriptors::TryCreate(graphics.device, plan,
                                                        local, error, failure);
              });
          Check(!buffers.geometry && !buffers.uv,
                "buffer source ownership moved");
          if (fail) {
            const VkResult expected[] = {
                VK_SUCCESS, VK_ERROR_OUT_OF_HOST_MEMORY,
                VK_ERROR_OUT_OF_DEVICE_MEMORY, VK_ERROR_OUT_OF_POOL_MEMORY};
            const B::DescriptorFailure stages[] = {
                B::DescriptorFailure::None, B::DescriptorFailure::Layout,
                B::DescriptorFailure::Pool, B::DescriptorFailure::Set};
            Check(!resources && !resources.buffers.geometry &&
                      !resources.buffers.uv && !resources.descriptors &&
                      resources.error == expected[fail] &&
                      resources.failure == stages[fail],
                  "precise descriptor error, no usable partial resources");
            Check(destroyed - destroyed_before == descriptor_count &&
                      layouts == 0 && pools == 0 && updates == updates_before,
                  "partial setup rollback before return, no update");
          } else {
            Check(bool(resources) && resources.descriptors->Set() &&
                      resources.descriptors->Layout() && layouts == 1 &&
                      pools == 1 && destroyed == destroyed_before &&
                      updates == updates_before + 1,
                  "successful setup owns all resources, one update");
            auto moved = std::move(resources);
            Check(bool(moved) && !resources, "complete owner move");
          }
        }
        VmaTotalStatistics stats{};
        vmaCalculateStatistics(graphics.allocator, &stats);
        Check(layouts == 0 && pools == 0 &&
                  destroyed - destroyed_before == descriptor_count &&
                  stats.total.statistics.allocationCount == 0,
              "exact-once complete descriptor/buffer cleanup");
      }
  descriptor_fail = 0;
  int calls = 0;
  B::PreparedBuffers<Buffer> empty;
  empty.error = VK_SUCCESS;
  auto invalid = B::AttachDescriptors<Buffer, B::ComputeDescriptors>(
      std::move(empty),
      [&](const auto &, vk::Result &, B::DescriptorFailure &) {
        ++calls;
        return std::unique_ptr<B::ComputeDescriptors>{};
      });
  Check(
      !invalid && invalid.error == VK_ERROR_INITIALIZATION_FAILED && calls == 0,
      "invalid buffers fail before descriptor factory even with success code");
  dispatch.vkCreateDescriptorSetLayout = real_layout;
  dispatch.vkDestroyDescriptorSetLayout = real_destroy_layout;
  dispatch.vkCreateDescriptorPool = real_pool;
  dispatch.vkDestroyDescriptorPool = real_destroy_pool;
  dispatch.vkAllocateDescriptorSets = real_sets;
  dispatch.vkUpdateDescriptorSets = real_update;
  Check(created == destroyed && created - before_created == 24 && updates == 4,
        "16 real descriptor scenarios:24 buffers/four successful updates");
  std::printf("[vma] descriptors:16 scenarios,24 buffers,four updates; live "
              "layouts=%d pools=%d\n",
              layouts, pools);
}
#include "MeshResourceCompletion.h"
} // namespace
int main(int argc, char **argv) {
  if ((argc != 2 && argc != 3) || (std::strcmp(argv[1], "--vulkan13") &&
                                   std::strcmp(argv[1], "--vulkan12")))
    return 1;
  if (argc == 3 && std::strcmp(argv[2], "--require-rx5700xt"))
    return 1;
  const bool api13 = std::strcmp(argv[1], "--vulkan13") == 0;
  std::filesystem::path executable;
#ifdef _WIN32
  std::wstring buffer(32768, L'\0');
  const auto length =
      GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
  if (!length || length >= buffer.size()) {
    Check(false, "executable path discovery");
    return 1;
  }
  buffer.resize(length);
  executable = std::filesystem::path(buffer);
#else
  std::error_code error;
  executable = std::filesystem::read_symlink("/proc/self/exe", error);
  if (error)
    executable = std::filesystem::absolute(argv[0], error);
  if (error) {
    Check(false, "executable path discovery");
    return 1;
  }
#endif
  const auto shaders = executable.parent_path() / "gpu_mesh_proto_shaders";
  if (!ValidateCompletionModules(shaders, api13))
    return 1;
  int result;
  {
    Runtime runtime;
    result = runtime.Init(api13, argc == 3);
    if (!result) {
      Cases(*runtime.graphics);
      DescriptorCases(*runtime.graphics);
      CompletionCase(*runtime.graphics, shaders, api13, "frontend", false,
                     false, false);
      CompletionCase(*runtime.graphics, shaders, api13, "frontend_permuted",
                     false, false, true);
      CompletionCase(*runtime.graphics, shaders, api13, "frontend_zero", true,
                     false, false);
      CompletionCase(*runtime.graphics, shaders, api13, "frontend_uv", false,
                     true, false);
      CompletionCase(*runtime.graphics, shaders, api13, "frontend_uv_zero",
                     true, true, false);
      Check(created == destroyed && created == 44 && refused == 18,
            "all allocation, descriptor and dispatch resources retired");
      std::printf("[vma] total: created=%d destroyed=%d refusals=%d\n", created,
                  destroyed, refused);
    }
  }
  if (failures || errors || warnings)
    result = 1;
  std::printf("[vma] RESULT: %s failures=%d validation-errors=%d "
              "validation-warnings=%d including cleanup\n",
              result == 41 ? "SKIPPED"
              : result     ? "FAIL"
                           : "PASS",
              failures, errors.load(), warnings.load());
  return result;
}
