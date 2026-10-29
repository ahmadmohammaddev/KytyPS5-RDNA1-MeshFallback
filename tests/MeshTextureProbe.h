// P4.3 authored synthetic texture/UV fixture, included in the existing test TU.
// UV layout (words): guards 0/1, 64 float2 slots 2..129, guards 130/131,
// count 132, status 133 (Pending=FFFFFFFF, Ok=0, overflow=1), guards 134/135.
// 544 bytes; vertex binding offset 8/stride 8. Original P4.1 blob is unchanged.
constexpr size_t kUvWords = 136;
constexpr size_t kUvBytes = kUvWords * 4;
using UvBlob = std::array<uint32_t, kUvWords>;
constexpr std::array<size_t, 6> kUvGuards = {0, 1, 130, 131, 134, 135};
static_assert(kUvBytes == 544 && 2 * sizeof(uint32_t) == 8);
static_assert(2 + 64 * 2 == 130 && 134 + 2 == kUvWords);
UvBlob InitUv() {
  UvBlob value;
  value.fill(0xA5A5A5A5u);
  for (auto i : kUvGuards) {
    value[i] = kGuardWord;
  }
  value[132] = 0;
  value[133] = kStatusPending;
  return value;
}
void WriteUv(UvBlob &value, uint32_t count) {
  if (count > 64) {
    value[133] = 1;
    return;
  }
  for (uint32_t i = 0; i < count; ++i) {
    const float uv[2] = {0.25f + (i % 3 == 1 ? 1.0f : 0.0f),
                         0.25f + (i % 3 == 2 ? 1.0f : 0.0f)};
    std::memcpy(value.data() + 2 + i * 2, uv, sizeof(uv));
  }
  value[132] = count;
  value[133] = kStatusOk;
}
constexpr std::array<std::array<uint8_t, 4>, 4> kTextureColors = {
    {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}, {255, 255, 0, 255}}};
static_assert(sizeof(kTextureColors) == 16);
std::array<uint8_t, 4> TexturePixel(uint32_t x, uint32_t y, bool draw) {
  if (!draw || !TrianglePixel(x, y)) {
    return {0, 0, 0, 255};
  }
  // UVs .25..1.25; nearest 2x2 sampling with clamp-to-edge. The .5
  // boundaries fall at screen x=24 and y=23.875, away from pixel centers.
  return kTextureColors[(y >= 24 ? 2 : 0) + (x >= 24 ? 1 : 0)];
}
size_t TextureMismatches(const uint8_t *rgba, bool draw) {
  size_t bad = 0;
  for (uint32_t y = 0; y < 64; ++y) {
    for (uint32_t x = 0; x < 64; ++x) {
      const auto expected = TexturePixel(x, y, draw);
      if (std::memcmp(rgba + (y * 64 + x) * 4, expected.data(), 4) != 0) {
        ++bad;
      }
    }
  }
  return bad;
}
std::filesystem::path TextureModulePath(bool api13, const char *stage) {
  return g_shader_directory / (std::string("mesh_texture_") + stage +
                               (api13 ? "_vk13.spv" : "_vk12.spv"));
}
bool ValidateTextureShaders() {
  for (bool api13 : {true, false}) {
    for (const char *stage : {"comp", "vert", "frag"}) {
      const auto words = LoadSpirvFile(TextureModulePath(api13, stage), stage);
      const auto env = api13 ? SPV_ENV_VULKAN_1_3 : SPV_ENV_VULKAN_1_2;
      if (!ValidateSpirv(words, env, stage)) {
        return false;
      }
      spvtools::SpirvTools tools(env);
      std::string text;
      Expect(tools.Disassemble(words, &text), "texture shader disassembly");
      const char *model = std::strcmp(stage, "comp") == 0   ? "GLCompute"
                          : std::strcmp(stage, "vert") == 0 ? "Vertex"
                                                            : "Fragment";
      Expect(text.find(std::string("OpEntryPoint ") + model) !=
                 std::string::npos,
             "texture execution model");
    }
  }
  return g_failures == 0;
}
int CheckTextureFixtures() {
  const auto initial = InitUv();
  for (uint32_t count : {0u, 3u, 64u}) {
    auto value = initial;
    WriteUv(value, count);
    Expect(value[132] == count && value[133] == 0,
           "UV accepted count and stored Ok");
    constexpr uint32_t uv_bits[6] = {0x3E800000u, 0x3E800000u, 0x3FA00000u,
                                     0x3E800000u, 0x3E800000u, 0x3FA00000u};
    for (size_t i = 0; i < count * 2; ++i) {
      Expect(value[2 + i] == uv_bits[i % 6], "UV reference uses exact .25/1.25 float bits");
    }
    for (auto i : kUvGuards) {
      Expect(value[i] == kGuardWord, "UV guard preserved");
    }
    for (size_t i = 2 + count * 2; i < 130; ++i) {
      Expect(value[i] == initial[i], "UV unused slot preserved");
    }
  }
  auto rejected = initial;
  WriteUv(rejected, 65);
  Expect(rejected[133] == 1, "UV overflow status");
  for (size_t i = 0; i < kUvWords; ++i) {
    if (i != 133) {
      Expect(rejected[i] == initial[i], "UV rejection writes only status");
    }
  }
  WriteUv(rejected, 3);
  auto valid = initial;
  WriteUv(valid, 3);
  Expect(rejected == valid, "UV overflow recovery is not sticky");
  std::vector<uint8_t> rgba(kDrawBytes);
  std::array<size_t, 4> colors = {};
  for (uint32_t y = 0; y < 64; ++y) {
    for (uint32_t x = 0; x < 64; ++x) {
      const auto color = TexturePixel(x, y, true);
      std::memcpy(rgba.data() + (y * 64 + x) * 4, color.data(), 4);
      if (TrianglePixel(x, y)) {
        ++colors[(y >= 24 ? 2 : 0) + (x >= 24 ? 1 : 0)];
      }
    }
  }
  Expect(TextureMismatches(rgba.data(), true) == 0,
         "textured reference accepted");
  for (auto count : colors) {
    Expect(count > 0, "all four texels visible");
  }
  rgba[(20 * 64 + 20) * 4 + 3] = 0;
  Expect(TextureMismatches(rgba.data(), true) == 1,
         "textured alpha corruption rejected");
  for (size_t i = 0; i < rgba.size(); i += 4) {
    std::fill_n(rgba.data() + i, 4, 255);
  }
  Expect(TextureMismatches(rgba.data(), true) == 4096,
         "untextured all-white output rejected");
  PrintLine1("texture-fixtures: %s (UV 544 bytes, capacities/recovery/guards, "
             "four colors %zu/%zu/%zu/%zu)",
             g_failures == 0 ? "PASS" : "FAIL", colors[0], colors[1], colors[2],
             colors[3]);
  return g_failures == 0 ? 0 : 1;
}

struct TextureProbe {
  VkDevice device;
  PFN_vkGetDeviceProcAddr proc;
  const VkPhysicalDeviceMemoryProperties &memory;
  GpuBuffer uv, staging;
  bool borrowed_uv = false;
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory image_memory = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkDescriptorSetLayout uv_layout = VK_NULL_HANDLE,
                        sampled_layout = VK_NULL_HANDLE;
  VkDescriptorPool pool = VK_NULL_HANDLE;
  VkDescriptorSet uv_set = VK_NULL_HANDLE, sampled_set = VK_NULL_HANDLE;
  VkPipelineLayout compute_layout = VK_NULL_HANDLE;
  VkPipeline compute = VK_NULL_HANDLE;
  VkShaderModule module = VK_NULL_HANDLE;
  PFN_vkCreateImage CreateImage = nullptr;
  PFN_vkDestroyImage DestroyImage = nullptr;
  PFN_vkGetImageMemoryRequirements GetImageMemoryRequirements = nullptr;
  PFN_vkAllocateMemory AllocateMemory = nullptr;
  PFN_vkFreeMemory FreeMemory = nullptr;
  PFN_vkBindImageMemory BindImageMemory = nullptr;
  PFN_vkCreateImageView CreateImageView = nullptr;
  PFN_vkDestroyImageView DestroyImageView = nullptr;
  PFN_vkCreateSampler CreateSampler = nullptr;
  PFN_vkDestroySampler DestroySampler = nullptr;
  PFN_vkCreateDescriptorSetLayout CreateDescriptorSetLayout = nullptr;
  PFN_vkDestroyDescriptorSetLayout DestroyDescriptorSetLayout = nullptr;
  PFN_vkCreateDescriptorPool CreateDescriptorPool = nullptr;
  PFN_vkDestroyDescriptorPool DestroyDescriptorPool = nullptr;
  PFN_vkAllocateDescriptorSets AllocateDescriptorSets = nullptr;
  PFN_vkUpdateDescriptorSets UpdateDescriptorSets = nullptr;
  PFN_vkCreateShaderModule CreateShaderModule = nullptr;
  PFN_vkDestroyShaderModule DestroyShaderModule = nullptr;
  PFN_vkCreatePipelineLayout CreatePipelineLayout = nullptr;
  PFN_vkDestroyPipelineLayout DestroyPipelineLayout = nullptr;
  PFN_vkCreateComputePipelines CreateComputePipelines = nullptr;
  PFN_vkDestroyPipeline DestroyPipeline = nullptr;
  PFN_vkUnmapMemory UnmapMemory = nullptr;
  PFN_vkDestroyBuffer DestroyBuffer = nullptr;
  PFN_vkDeviceWaitIdle DeviceWaitIdle = nullptr;
  PFN_vkFlushMappedMemoryRanges FlushMappedMemoryRanges = nullptr;
  PFN_vkInvalidateMappedMemoryRanges InvalidateMappedMemoryRanges = nullptr;
  PFN_vkCmdBindPipeline CmdBindPipeline = nullptr;
  PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets = nullptr;
  PFN_vkCmdPushConstants CmdPushConstants = nullptr;
  PFN_vkCmdDispatch CmdDispatch = nullptr;
  PFN_vkCmdPipelineBarrier CmdPipelineBarrier = nullptr;
  PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage = nullptr;
  PFN_vkResetCommandBuffer ResetCommandBuffer = nullptr;
  PFN_vkBeginCommandBuffer BeginCommandBuffer = nullptr;
  PFN_vkEndCommandBuffer EndCommandBuffer = nullptr;
  PFN_vkQueueSubmit QueueSubmit = nullptr;
  PFN_vkWaitForFences WaitForFences = nullptr;
  PFN_vkResetFences ResetFences = nullptr;
  TextureProbe(VkDevice d, PFN_vkGetDeviceProcAddr p,
               const VkPhysicalDeviceMemoryProperties &m)
      : device(d), proc(p), memory(m) {}
  TextureProbe(const TextureProbe&) = delete;
  TextureProbe& operator=(const TextureProbe&) = delete;
  ~TextureProbe() {
    if (uv.buffer || image) {
      CheckVk(DeviceWaitIdle(device), "texture idle before cleanup");
    }
    if (compute) {
      DestroyPipeline(device, compute, nullptr);
    }
    if (compute_layout) {
      DestroyPipelineLayout(device, compute_layout, nullptr);
    }
    if (module) {
      DestroyShaderModule(device, module, nullptr);
    }
    if (pool) {
      DestroyDescriptorPool(device, pool, nullptr);
    }
    if (uv_layout) {
      DestroyDescriptorSetLayout(device, uv_layout, nullptr);
    }
    if (sampled_layout) {
      DestroyDescriptorSetLayout(device, sampled_layout, nullptr);
    }
    if (sampler) {
      DestroySampler(device, sampler, nullptr);
    }
    if (view) {
      DestroyImageView(device, view, nullptr);
    }
    if (image) {
      DestroyImage(device, image, nullptr);
    }
    if (image_memory) {
      FreeMemory(device, image_memory, nullptr);
    }
    for (auto *b : {&uv, &staging}) {
      if (b == &uv && borrowed_uv) continue;
      if (b->mapped) {
        UnmapMemory(device, b->memory);
      }
      if (b->buffer) {
        DestroyBuffer(device, b->buffer, nullptr);
      }
      if (b->memory) {
        FreeMemory(device, b->memory, nullptr);
      }
    }
  }
  bool Initialize(bool api13, GpuBuffer* generated_uv = nullptr) {
    borrowed_uv = generated_uv != nullptr;
    CreateImage =
        reinterpret_cast<PFN_vkCreateImage>(proc(device, "vkCreateImage"));
    if (!CreateImage) {
      Expect(false, "texture vkCreateImage resolves");
      return false;
    }
    DestroyImage =
        reinterpret_cast<PFN_vkDestroyImage>(proc(device, "vkDestroyImage"));
    if (!DestroyImage) {
      Expect(false, "texture vkDestroyImage resolves");
      return false;
    }
    GetImageMemoryRequirements =
        reinterpret_cast<PFN_vkGetImageMemoryRequirements>(
            proc(device, "vkGetImageMemoryRequirements"));
    if (!GetImageMemoryRequirements) {
      Expect(false, "texture vkGetImageMemoryRequirements resolves");
      return false;
    }
    AllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(
        proc(device, "vkAllocateMemory"));
    if (!AllocateMemory) {
      Expect(false, "texture vkAllocateMemory resolves");
      return false;
    }
    FreeMemory =
        reinterpret_cast<PFN_vkFreeMemory>(proc(device, "vkFreeMemory"));
    if (!FreeMemory) {
      Expect(false, "texture vkFreeMemory resolves");
      return false;
    }
    BindImageMemory = reinterpret_cast<PFN_vkBindImageMemory>(
        proc(device, "vkBindImageMemory"));
    if (!BindImageMemory) {
      Expect(false, "texture vkBindImageMemory resolves");
      return false;
    }
    CreateImageView = reinterpret_cast<PFN_vkCreateImageView>(
        proc(device, "vkCreateImageView"));
    if (!CreateImageView) {
      Expect(false, "texture vkCreateImageView resolves");
      return false;
    }
    DestroyImageView = reinterpret_cast<PFN_vkDestroyImageView>(
        proc(device, "vkDestroyImageView"));
    if (!DestroyImageView) {
      Expect(false, "texture vkDestroyImageView resolves");
      return false;
    }
    CreateSampler =
        reinterpret_cast<PFN_vkCreateSampler>(proc(device, "vkCreateSampler"));
    if (!CreateSampler) {
      Expect(false, "texture vkCreateSampler resolves");
      return false;
    }
    DestroySampler = reinterpret_cast<PFN_vkDestroySampler>(
        proc(device, "vkDestroySampler"));
    if (!DestroySampler) {
      Expect(false, "texture vkDestroySampler resolves");
      return false;
    }
    CreateDescriptorSetLayout =
        reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(
            proc(device, "vkCreateDescriptorSetLayout"));
    if (!CreateDescriptorSetLayout) {
      Expect(false, "texture vkCreateDescriptorSetLayout resolves");
      return false;
    }
    DestroyDescriptorSetLayout =
        reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(
            proc(device, "vkDestroyDescriptorSetLayout"));
    if (!DestroyDescriptorSetLayout) {
      Expect(false, "texture vkDestroyDescriptorSetLayout resolves");
      return false;
    }
    CreateDescriptorPool = reinterpret_cast<PFN_vkCreateDescriptorPool>(
        proc(device, "vkCreateDescriptorPool"));
    if (!CreateDescriptorPool) {
      Expect(false, "texture vkCreateDescriptorPool resolves");
      return false;
    }
    DestroyDescriptorPool = reinterpret_cast<PFN_vkDestroyDescriptorPool>(
        proc(device, "vkDestroyDescriptorPool"));
    if (!DestroyDescriptorPool) {
      Expect(false, "texture vkDestroyDescriptorPool resolves");
      return false;
    }
    AllocateDescriptorSets = reinterpret_cast<PFN_vkAllocateDescriptorSets>(
        proc(device, "vkAllocateDescriptorSets"));
    if (!AllocateDescriptorSets) {
      Expect(false, "texture vkAllocateDescriptorSets resolves");
      return false;
    }
    UpdateDescriptorSets = reinterpret_cast<PFN_vkUpdateDescriptorSets>(
        proc(device, "vkUpdateDescriptorSets"));
    if (!UpdateDescriptorSets) {
      Expect(false, "texture vkUpdateDescriptorSets resolves");
      return false;
    }
    CreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(
        proc(device, "vkCreateShaderModule"));
    if (!CreateShaderModule) {
      Expect(false, "texture vkCreateShaderModule resolves");
      return false;
    }
    DestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(
        proc(device, "vkDestroyShaderModule"));
    if (!DestroyShaderModule) {
      Expect(false, "texture vkDestroyShaderModule resolves");
      return false;
    }
    CreatePipelineLayout = reinterpret_cast<PFN_vkCreatePipelineLayout>(
        proc(device, "vkCreatePipelineLayout"));
    if (!CreatePipelineLayout) {
      Expect(false, "texture vkCreatePipelineLayout resolves");
      return false;
    }
    DestroyPipelineLayout = reinterpret_cast<PFN_vkDestroyPipelineLayout>(
        proc(device, "vkDestroyPipelineLayout"));
    if (!DestroyPipelineLayout) {
      Expect(false, "texture vkDestroyPipelineLayout resolves");
      return false;
    }
    CreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(
        proc(device, "vkCreateComputePipelines"));
    if (!CreateComputePipelines) {
      Expect(false, "texture vkCreateComputePipelines resolves");
      return false;
    }
    DestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(
        proc(device, "vkDestroyPipeline"));
    if (!DestroyPipeline) {
      Expect(false, "texture vkDestroyPipeline resolves");
      return false;
    }
    UnmapMemory =
        reinterpret_cast<PFN_vkUnmapMemory>(proc(device, "vkUnmapMemory"));
    if (!UnmapMemory) {
      Expect(false, "texture vkUnmapMemory resolves");
      return false;
    }
    DestroyBuffer =
        reinterpret_cast<PFN_vkDestroyBuffer>(proc(device, "vkDestroyBuffer"));
    if (!DestroyBuffer) {
      Expect(false, "texture vkDestroyBuffer resolves");
      return false;
    }
    DeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
        proc(device, "vkDeviceWaitIdle"));
    if (!DeviceWaitIdle) {
      Expect(false, "texture vkDeviceWaitIdle resolves");
      return false;
    }
    FlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(
        proc(device, "vkFlushMappedMemoryRanges"));
    if (!FlushMappedMemoryRanges) {
      Expect(false, "texture vkFlushMappedMemoryRanges resolves");
      return false;
    }
    InvalidateMappedMemoryRanges =
        reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(
            proc(device, "vkInvalidateMappedMemoryRanges"));
    if (!InvalidateMappedMemoryRanges) {
      Expect(false, "texture vkInvalidateMappedMemoryRanges resolves");
      return false;
    }
    CmdBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(
        proc(device, "vkCmdBindPipeline"));
    if (!CmdBindPipeline) {
      Expect(false, "texture vkCmdBindPipeline resolves");
      return false;
    }
    CmdBindDescriptorSets = reinterpret_cast<PFN_vkCmdBindDescriptorSets>(
        proc(device, "vkCmdBindDescriptorSets"));
    if (!CmdBindDescriptorSets) {
      Expect(false, "texture vkCmdBindDescriptorSets resolves");
      return false;
    }
    CmdPushConstants = reinterpret_cast<PFN_vkCmdPushConstants>(
        proc(device, "vkCmdPushConstants"));
    if (!CmdPushConstants) {
      Expect(false, "texture vkCmdPushConstants resolves");
      return false;
    }
    CmdDispatch =
        reinterpret_cast<PFN_vkCmdDispatch>(proc(device, "vkCmdDispatch"));
    if (!CmdDispatch) {
      Expect(false, "texture vkCmdDispatch resolves");
      return false;
    }
    CmdPipelineBarrier = reinterpret_cast<PFN_vkCmdPipelineBarrier>(
        proc(device, "vkCmdPipelineBarrier"));
    if (!CmdPipelineBarrier) {
      Expect(false, "texture vkCmdPipelineBarrier resolves");
      return false;
    }
    CmdCopyBufferToImage = reinterpret_cast<PFN_vkCmdCopyBufferToImage>(
        proc(device, "vkCmdCopyBufferToImage"));
    if (!CmdCopyBufferToImage) {
      Expect(false, "texture vkCmdCopyBufferToImage resolves");
      return false;
    }
    ResetCommandBuffer = reinterpret_cast<PFN_vkResetCommandBuffer>(
        proc(device, "vkResetCommandBuffer"));
    if (!ResetCommandBuffer) {
      Expect(false, "texture vkResetCommandBuffer resolves");
      return false;
    }
    BeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(
        proc(device, "vkBeginCommandBuffer"));
    if (!BeginCommandBuffer) {
      Expect(false, "texture vkBeginCommandBuffer resolves");
      return false;
    }
    EndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(
        proc(device, "vkEndCommandBuffer"));
    if (!EndCommandBuffer) {
      Expect(false, "texture vkEndCommandBuffer resolves");
      return false;
    }
    QueueSubmit =
        reinterpret_cast<PFN_vkQueueSubmit>(proc(device, "vkQueueSubmit"));
    if (!QueueSubmit) {
      Expect(false, "texture vkQueueSubmit resolves");
      return false;
    }
    WaitForFences =
        reinterpret_cast<PFN_vkWaitForFences>(proc(device, "vkWaitForFences"));
    if (!WaitForFences) {
      Expect(false, "texture vkWaitForFences resolves");
      return false;
    }
    ResetFences =
        reinterpret_cast<PFN_vkResetFences>(proc(device, "vkResetFences"));
    if (!ResetFences) {
      Expect(false, "texture vkResetFences resolves");
      return false;
    }
    // Adopt only after cleanup entry points have all been resolved, so a failed
    // lookup cannot leave a borrowed live buffer with an incomplete destructor.
    if (generated_uv) uv = *generated_uv;
    if ((!borrowed_uv && AllocateMappedBuffer(device, proc, memory, kUvBytes,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, uv,
                             "uv-output") != 0) ||
        AllocateMappedBuffer(device, proc, memory, 16,
                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging,
                             "texture-upload") != 0) {
      return false;
    }
    VkImageCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType = VK_IMAGE_TYPE_2D;
    ci.format = VK_FORMAT_R8G8B8A8_UNORM;
    ci.extent = {2, 2, 1};
    ci.mipLevels = ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!CheckVk(CreateImage(device, &ci, nullptr, &image), "texture image")) {
      return false;
    }
    VkMemoryRequirements req = {};
    GetImageMemoryRequirements(device, image, &req);
    const auto type =
        PickMemoryType(memory, req.memoryTypeBits, 0,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
    if (type == UINT32_MAX) {
      Expect(false, "texture image memory type");
      return false;
    }
    VkMemoryAllocateInfo ai = {};
    ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    ai.memoryTypeIndex = type;
    if (!CheckVk(AllocateMemory(device, &ai, nullptr, &image_memory),
                 "texture allocate") ||
        !CheckVk(BindImageMemory(device, image, image_memory, 0),
                 "texture bind")) {
      return false;
    }
    VkImageViewCreateInfo vi = {};
    vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image = image;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = ci.format;
    vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    if (!CheckVk(CreateImageView(device, &vi, nullptr, &view),
                 "texture view")) {
      return false;
    }
    VkSamplerCreateInfo si = {};
    si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    si.magFilter = si.minFilter = VK_FILTER_NEAREST;
    si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU = si.addressModeV = si.addressModeW =
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    if (!CheckVk(CreateSampler(device, &si, nullptr, &sampler),
                 "texture sampler")) {
      return false;
    }
    VkDescriptorSetLayoutBinding binding = {
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
        nullptr};
    VkDescriptorSetLayoutCreateInfo li = {};
    li.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    li.bindingCount = 1;
    li.pBindings = &binding;
    if (!CheckVk(CreateDescriptorSetLayout(device, &li, nullptr, &uv_layout),
                 "UV descriptor layout")) {
      return false;
    }
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    if (!CheckVk(
            CreateDescriptorSetLayout(device, &li, nullptr, &sampled_layout),
            "sampled descriptor layout")) {
      return false;
    }
    VkDescriptorPoolSize sizes[2] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};
    VkDescriptorPoolCreateInfo pi = {};
    pi.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pi.maxSets = 2;
    pi.poolSizeCount = 2;
    pi.pPoolSizes = sizes;
    if (!CheckVk(CreateDescriptorPool(device, &pi, nullptr, &pool),
                 "texture descriptor pool")) {
      return false;
    }
    VkDescriptorSetLayout layouts[2] = {uv_layout, sampled_layout};
    VkDescriptorSet sets[2] = {};
    VkDescriptorSetAllocateInfo di = {};
    di.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    di.descriptorPool = pool;
    di.descriptorSetCount = 2;
    di.pSetLayouts = layouts;
    if (!CheckVk(AllocateDescriptorSets(device, &di, sets),
                 "texture descriptor sets")) {
      return false;
    }
    uv_set = sets[0];
    sampled_set = sets[1];
    VkDescriptorBufferInfo buffer = {uv.buffer, 0, kUvBytes};
    VkDescriptorImageInfo tex = {sampler, view,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = uv_set;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[0].pBufferInfo = &buffer;
    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = sampled_set;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = &tex;
    UpdateDescriptorSets(device, 2, writes, 0, nullptr);
    // The IR pipeline owns UV generation in borrowed mode. Do not even create
    // the authored UV pipeline: its dispatch would hide translation mistakes.
    if (borrowed_uv) return true;
    const auto words =
        LoadSpirvFile(TextureModulePath(api13, "comp"), "UV generator");
    if (words.empty()) {
      return false;
    }
    VkShaderModuleCreateInfo mi = {};
    mi.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    mi.codeSize = words.size() * 4;
    mi.pCode = words.data();
    if (!CheckVk(CreateShaderModule(device, &mi, nullptr, &module),
                 "UV module")) {
      return false;
    }
    VkPushConstantRange range = {VK_SHADER_STAGE_COMPUTE_BIT, 0, 4};
    VkPipelineLayoutCreateInfo pli = {};
    pli.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &uv_layout;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &range;
    if (!CheckVk(CreatePipelineLayout(device, &pli, nullptr, &compute_layout),
                 "UV pipeline layout")) {
      return false;
    }
    VkComputePipelineCreateInfo pci = {};
    pci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pci.stage.module = module;
    pci.stage.pName = "main";
    pci.layout = compute_layout;
    return CheckVk(CreateComputePipelines(device, VK_NULL_HANDLE, 1, &pci,
                                          nullptr, &compute),
                   "UV compute pipeline");
  }
  bool Upload() {
    const auto initial = InitUv();
    std::memcpy(uv.mapped, initial.data(), kUvBytes);
    std::memcpy(staging.mapped, kTextureColors.data(), 16);
    for (auto *b : {&uv, &staging}) {
      if (b->noncoherent) {
        VkMappedMemoryRange range = {};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = b->memory;
        range.size = VK_WHOLE_SIZE;
        if (!CheckVk(FlushMappedMemoryRanges(device, 1, &range),
                     "UV/texture host flush")) {
          return false;
        }
      }
    }
    return true;
  }
  void Record(VkCommandBuffer cmd) {
    if (!borrowed_uv) {
      CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute);
      CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_layout,
                            0, 1, &uv_set, 0, nullptr);
      const uint32_t count = 3;
      CmdPushConstants(cmd, compute_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4,
                       &count);
      CmdDispatch(cmd, 1, 1, 1);
    }
    VkBufferMemoryBarrier uv_barrier = {};
    uv_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    uv_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    uv_barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    uv_barrier.srcQueueFamilyIndex = uv_barrier.dstQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED;
    uv_barrier.buffer = uv.buffer;
    uv_barrier.size = kUvBytes;
    CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1,
                       &uv_barrier, 0, nullptr);
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex =
        VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
    VkBufferImageCopy copy = {};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {2, 2, 1};
    CmdCopyBufferToImage(cmd, staging.buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
  }
  bool CheckBoundaries(VkCommandBuffer cmd, VkQueue queue, VkFence fence) {
    auto expected = InitUv();
    std::memcpy(uv.mapped, expected.data(), kUvBytes);
    if (uv.noncoherent) {
      VkMappedMemoryRange r = {};
      r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      r.memory = uv.memory;
      r.size = VK_WHOLE_SIZE;
      if (!CheckVk(FlushMappedMemoryRanges(device, 1, &r),
                   "UV boundary initial flush")) {
        return false;
      }
    }
    for (uint32_t count : {0u, 64u, 65u, 3u}) {
      if (!CheckVk(ResetCommandBuffer(cmd, 0), "UV boundary reset")) {
        return false;
      }
      VkCommandBufferBeginInfo begin = {};
      begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      if (!CheckVk(BeginCommandBuffer(cmd, &begin), "UV boundary begin")) {
        return false;
      }
      VkMemoryBarrier memory_barrier = {};
      memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
      memory_barrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      memory_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      CmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_HOST_BIT |
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                         &memory_barrier, 0, nullptr, 0, nullptr);
      CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute);
      CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_layout,
                            0, 1, &uv_set, 0, nullptr);
      CmdPushConstants(cmd, compute_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4,
                       &count);
      CmdDispatch(cmd, 1, 1, 1);
      memory_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      memory_barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
      CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &memory_barrier, 0,
                         nullptr, 0, nullptr);
      if (!CheckVk(EndCommandBuffer(cmd), "UV boundary end")) {
        return false;
      }
      VkSubmitInfo submit = {};
      submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit.commandBufferCount = 1;
      submit.pCommandBuffers = &cmd;
      if (!CheckVk(QueueSubmit(queue, 1, &submit, fence),
                   "UV boundary submit") ||
          !CheckVk(WaitForFences(device, 1, &fence, VK_TRUE, 10000000000ull),
                   "UV boundary fence") ||
          !CheckVk(ResetFences(device, 1, &fence), "UV boundary fence reset")) {
        return false;
      }
      if (uv.noncoherent) {
        VkMappedMemoryRange r = {};
        r.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        r.memory = uv.memory;
        r.size = VK_WHOLE_SIZE;
        if (!CheckVk(InvalidateMappedMemoryRanges(device, 1, &r),
                     "UV boundary invalidate")) {
          return false;
        }
      }
      WriteUv(expected, count);
      Expect(std::memcmp(uv.mapped, expected.data(), kUvBytes) == 0,
             "UV GPU capacity/overflow/recovery full snapshot");
      PrintLine1("UV boundary count=%u: %s (all 136 words, status=%u)", count,
                 g_failures == 0 ? "PASS" : "FAIL", expected[133]);
      if (g_failures || g_validation_errors) {
        return false;
      }
    }
    return true;
  }
  bool Check(bool draw, uint32_t count = 3) {
    if (uv.noncoherent) {
      VkMappedMemoryRange range = {};
      range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      range.memory = uv.memory;
      range.size = VK_WHOLE_SIZE;
      if (!CheckVk(InvalidateMappedMemoryRanges(device, 1, &range),
                   "UV readback invalidate")) {
        return false;
      }
    }
    auto expected = InitUv();
    if (draw) {
      WriteUv(expected, count);
    }
    Expect(std::memcmp(uv.mapped, expected.data(), kUvBytes) == 0,
           "entire UV blob/count/status/guards/unused slots match");
    Expect(std::memcmp(staging.mapped, kTextureColors.data(), 16) == 0,
           "authored texture upload preserved");
    PrintLine1("UV full-buffer: %s (136 words including count/status/guards)",
               g_failures == 0 ? "PASS" : "FAIL");
    return g_failures == 0;
  }
};
