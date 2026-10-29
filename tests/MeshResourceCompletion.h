// SPDX-License-Identifier: GPL-2.0-only
// Included by MeshBufferVmaTests in its anonymous namespace. Real Vulkan fence
// completion adapter for the production retention helper. NOT CommandScheduler
// execution, renderer draw integration or a cross-queue lifetime proof.
bool ValidateCompletionModules(const std::filesystem::path &directory,
                               bool api13) {
  spvtools::SpirvTools validator(api13 ? SPV_ENV_VULKAN_1_3
                                       : SPV_ENV_VULKAN_1_2);
  validator.SetMessageConsumer([](spv_message_level_t, const char *,
                                  const spv_position_t &position,
                                  const char *message) {
    std::printf("[vma] SPIR-V word%zu:%s\n", position.index, message);
  });
  for (const auto *variant : {"frontend", "frontend_permuted", "frontend_zero",
                              "frontend_uv", "frontend_uv_zero"}) {
    const auto path = directory / (std::string("mesh_ir_") + variant +
                                   (api13 ? "_vk13.spv" : "_vk12.spv"));
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
      Check(false,
            "required completion module missing (no source-tree fallback)");
      return false;
    }
    const auto bytes = input.tellg();
    if (bytes < 20 || size_t(bytes) % 4) {
      Check(false, "completion module invalid size");
      return false;
    }
    std::vector<uint32_t> words(size_t(bytes) / 4);
    input.seekg(0);
    input.read(reinterpret_cast<char *>(words.data()), bytes);
    if (!input || !validator.Validate(words)) {
      Check(false, "completion module offline SPIR-V validation");
      return false;
    }
  }
  std::printf("[vma] five Vulkan%s completion modules validated offline\n",
              api13 ? "1.3" : "1.2");
  return true;
}
struct CompletionRun {
  vk::Device device;
  std::unique_ptr<Buffer> readback;
  vk::ShaderModule module{};
  vk::PipelineLayout layout{};
  vk::Pipeline pipeline{};
  vk::CommandPool pool{};
  vk::Semaphore gate{};
  vk::Fence fence{};
  bool submitted = false, signalled = false;
  ~CompletionRun() {
    if (submitted && !signalled) {
      const vk::SemaphoreSignalInfo signal{
          vk::StructureType::eSemaphoreSignalInfo, nullptr, gate, 1};
      Check(device.signalSemaphore(&signal) == vk::Result::eSuccess,
            "cleanup unblocks test gate");
    }
    if (submitted)
      Check(device.waitIdle() == vk::Result::eSuccess,
            "completion cleanup idle");
    if (fence)
      device.destroyFence(fence);
    if (pool)
      device.destroyCommandPool(pool);
    if (pipeline)
      device.destroyPipeline(pipeline);
    if (layout)
      device.destroyPipelineLayout(layout);
    if (module)
      device.destroyShaderModule(module);
    if (gate)
      device.destroySemaphore(gate);
  }
};
bool CompletionCase(GraphicContext &graphics,
                    const std::filesystem::path &shaders, bool api13,
                    const char *variant, bool zero, bool uv, bool permuted) {
  // Lifetime order is deliberate: cleanup waits before callbacks can release
  // retained resources on ANY early return after submission.
  std::vector<Common::UniqueFunction<void>> retained;
  CompletionRun run{graphics.device};
  auto ok = [](vk::Result r, const char *stage) {
    Check(r == vk::Result::eSuccess, stage);
    return r == vk::Result::eSuccess;
  };
  const auto plan =
      *B::CheckedExecutionLayout(zero ? 0 : 3, zero ? 0 : 1, 64, 64, uv, 64);
  auto buffers =
      B::PrepareBufferOwnership<Buffer>(plan, [&](uint64_t size, bool is_uv) {
        vk::Result error;
        auto b = Buffer::TryCreate(
            graphics, nullptr, MemoryUsage::Upload,
            vk::BufferUsageFlagBits::eTransferSrc |
                vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eVertexBuffer |
                (is_uv ? vk::BufferUsageFlags{}
                       : vk::BufferUsageFlagBits::eIndexBuffer),
            size, error);
        return B::BufferAttempt<Buffer>{std::move(b), VkResult(error)};
      });
  auto resources = B::AttachDescriptors<Buffer, B::ComputeDescriptors>(
      std::move(buffers),
      [&](const auto &b, vk::Result &error, B::DescriptorFailure &failure) {
        return B::ComputeDescriptors::TryCreate(graphics.device, plan, b, error,
                                                failure);
      });
  Check(bool(resources), "completion prerequisites real Buffer+descriptors");
  if (!resources)
    return false;
  vk::Result readback_error;
  const auto readback_size = uint64_t(1840 + (uv ? 544 : 0));
  run.readback = Buffer::TryCreate(graphics, nullptr, MemoryUsage::Download,
                                   vk::BufferUsageFlagBits::eTransferDst,
                                   readback_size, readback_error);
  if (!run.readback || run.readback->Mapped().size() != readback_size) {
    Check(false, "mapped Download readback allocation");
    return false;
  }
  std::fill(run.readback->Mapped().begin(), run.readback->Mapped().end(),
            uint8_t(0x5B));
  run.readback->Flush(0, readback_size);
  auto *geometry = resources.buffers.geometry.get();
  auto *uvs = resources.buffers.uv.get();
  const auto set = resources.descriptors->Set();
  const auto descriptor_layout = resources.descriptors->Layout();
  // Independent literal full-buffer oracle: no lowerer/writer output reused.
  std::array<uint32_t, 460> initial{}, expected{};
  for (size_t i : {256, 257, 450, 451, 455, 456, 458, 459})
    initial[i] = 0xC0DEFEEDu;
  initial[457] = 0xFFFFFFFFu;
  expected = initial;
  if (!zero) {
    const float positions[] = {-.5f, -.5f, 0,    1,        .5f, -.5f,
                               0,    1,    -.5f, .484375f, 0,   1};
    std::memcpy(expected.data(), positions, sizeof(positions));
    expected[258] = permuted ? 2 : 0;
    expected[259] = permuted ? 0 : 1;
    expected[260] = permuted ? 1 : 2;
    expected[452] = 3;
    expected[453] = 1;
  }
  expected[457] = 0;
  std::array<uint32_t, 136> uv_initial, uv_expected;
  uv_initial.fill(0xA5A5A5A5u);
  for (size_t i : {0, 1, 130, 131, 134, 135})
    uv_initial[i] = 0xC0DEFEEDu;
  uv_initial[132] = 0;
  uv_initial[133] = 0xFFFFFFFFu;
  uv_expected = uv_initial;
  if (!zero) {
    const float values[] = {.25f, .25f, 1.25f, .25f, .25f, 1.25f};
    std::memcpy(uv_expected.data() + 2, values, sizeof(values));
    uv_expected[132] = 3;
  }
  uv_expected[133] = 0;
  std::memcpy(geometry->Mapped().data(), initial.data(), sizeof(initial));
  geometry->Flush(0, 1840);
  if (uv) {
    std::memcpy(uvs->Mapped().data(), uv_initial.data(), sizeof(uv_initial));
    uvs->Flush(0, 544);
  }
  const auto path = shaders / (std::string("mesh_ir_") + variant +
                               (api13 ? "_vk13.spv" : "_vk12.spv"));
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  Check(bool(input), "completion module present beside executable");
  if (!input)
    return false;
  const auto bytes = input.tellg();
  Check(bytes >= 20 && size_t(bytes) % 4 == 0, "completion SPIR-V size");
  if (bytes < 20 || size_t(bytes) % 4)
    return false;
  std::vector<uint32_t> code(size_t(bytes) / 4);
  input.seekg(0);
  input.read(reinterpret_cast<char *>(code.data()), bytes);
  if (!input) {
    Check(false, "complete module read");
    return false;
  }
  vk::ShaderModuleCreateInfo module{};
  module.codeSize = size_t(bytes);
  module.pCode = code.data();
  if (!ok(graphics.device.createShaderModule(&module, nullptr, &run.module),
          "completion shader module"))
    return false;
  vk::PipelineLayoutCreateInfo layout{};
  layout.setLayoutCount = 1;
  layout.pSetLayouts = &descriptor_layout;
  if (!ok(graphics.device.createPipelineLayout(&layout, nullptr, &run.layout),
          "completion pipeline layout"))
    return false;
  vk::ComputePipelineCreateInfo pipeline{};
  pipeline.layout = run.layout;
  pipeline.stage.stage = vk::ShaderStageFlagBits::eCompute;
  pipeline.stage.module = run.module;
  pipeline.stage.pName = "main";
  if (!ok(graphics.device.createComputePipelines({}, 1, &pipeline, nullptr,
                                                 &run.pipeline),
          "completion compute pipeline"))
    return false;
  vk::CommandPoolCreateInfo pool{};
  pool.queueFamilyIndex = graphics.queue_family;
  if (!ok(graphics.device.createCommandPool(&pool, nullptr, &run.pool),
          "completion command pool"))
    return false;
  vk::CommandBufferAllocateInfo allocate{};
  allocate.commandPool = run.pool;
  allocate.commandBufferCount = 1;
  vk::CommandBuffer command;
  if (!ok(graphics.device.allocateCommandBuffers(&allocate, &command),
          "completion command buffer"))
    return false;
  vk::SemaphoreTypeCreateInfo timeline{};
  timeline.semaphoreType = vk::SemaphoreType::eTimeline;
  vk::SemaphoreCreateInfo semaphore{};
  semaphore.pNext = &timeline;
  if (!ok(graphics.device.createSemaphore(&semaphore, nullptr, &run.gate),
          "completion host gate"))
    return false;
  const vk::FenceCreateInfo fence{};
  if (!ok(graphics.device.createFence(&fence, nullptr, &run.fence),
          "completion fence"))
    return false;
  const auto before_destroyed = destroyed;
  if (!B::RetainBeforeUse(
          1, std::move(resources),
          [&](uint64_t tick, Common::UniqueFunction<void> &cleanup) {
            Check(tick == 1, "one recording completion identity");
            retained.push_back(std::move(cleanup));
            return true;
          })) {
    Check(false, "retention before recording");
    return false;
  }
  Check(!resources && retained.size() == 1 && destroyed == before_destroyed,
        "producer no longer owns retained resources");
  const vk::CommandBufferBeginInfo begin{};
  if (!ok(command.begin(&begin), "completion begin"))
    return false;
  vk::MemoryBarrier upload{};
  upload.srcAccessMask = vk::AccessFlagBits::eHostWrite;
  upload.dstAccessMask =
      vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite |
      vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite;
  command.pipelineBarrier(vk::PipelineStageFlagBits::eHost,
                          vk::PipelineStageFlagBits::eComputeShader |
                              vk::PipelineStageFlagBits::eTransfer,
                          {}, 1, &upload, 0, nullptr, 0, nullptr);
  command.bindPipeline(vk::PipelineBindPoint::eCompute, run.pipeline);
  command.bindDescriptorSets(vk::PipelineBindPoint::eCompute, run.layout, 0, 1,
                             &set, 0, nullptr);
  command.dispatch(1, 1, 1);
  vk::MemoryBarrier copy{};
  copy.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
  copy.dstAccessMask = vk::AccessFlagBits::eTransferRead;
  command.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                          vk::PipelineStageFlagBits::eTransfer, {}, 1, &copy, 0,
                          nullptr, 0, nullptr);
  const vk::BufferCopy geometry_copy{0, 0, 1840};
  command.copyBuffer(geometry->Handle(), run.readback->Handle(), 1,
                     &geometry_copy);
  if (uv) {
    const vk::BufferCopy uv_copy{0, 1840, 544};
    command.copyBuffer(uvs->Handle(), run.readback->Handle(), 1, &uv_copy);
  }
  vk::MemoryBarrier host{};
  host.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
  host.dstAccessMask = vk::AccessFlagBits::eHostRead;
  command.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                          vk::PipelineStageFlagBits::eHost, {}, 1, &host, 0,
                          nullptr, 0, nullptr);
  if (!ok(command.end(), "completion end"))
    return false;
  const uint64_t one = 1;
  vk::TimelineSemaphoreSubmitInfo wait_value{};
  wait_value.waitSemaphoreValueCount = 1;
  wait_value.pWaitSemaphoreValues = &one;
  const auto wait_stage =
      vk::PipelineStageFlags(vk::PipelineStageFlagBits::eTopOfPipe);
  vk::SubmitInfo submit{};
  submit.pNext = &wait_value;
  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = &run.gate;
  submit.pWaitDstStageMask = &wait_stage;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &command;
  if (!ok(graphics.queue.submit(1, &submit, run.fence), "completion submit"))
    return false;
  run.submitted = true;
  Check(graphics.device.getFenceStatus(run.fence) == vk::Result::eNotReady &&
            destroyed == before_destroyed,
        "host-blocked real GPU submission cannot retire resources");
  VmaTotalStatistics live{};
  vmaCalculateStatistics(graphics.allocator, &live);
  Check(live.total.statistics.allocationCount == uint32_t(uv ? 3 : 2),
        "in-flight VMA allocations remain live");
  const vk::SemaphoreSignalInfo signal{vk::StructureType::eSemaphoreSignalInfo,
                                       nullptr, run.gate, 1};
  if (!ok(graphics.device.signalSemaphore(&signal), "release GPU gate"))
    return false;
  run.signalled = true;
  if (!ok(graphics.device.waitForFences(1, &run.fence, true, 10'000'000'000ULL),
          "completion wait (test readback only)"))
    return false;
  Check(destroyed == before_destroyed,
        "fence wait alone does not free retained ownership");
  run.readback->Invalidate(0, readback_size);
  Check(std::memcmp(run.readback->Mapped().data(), expected.data(), 1840) == 0,
        "all460 geometry words exact after actual dispatch");
  if (uv) {
    Check(std::memcmp(run.readback->Mapped().data() + 1840, uv_expected.data(),
                      544) == 0,
          "all136 UV words exact after actual dispatch");
  }
  for (auto &callback : retained)
    callback();
  retained.clear();
  Check(destroyed - before_destroyed == (uv ? 2 : 1),
        "completed resources retired once at explicit callback drain");
  vmaCalculateStatistics(graphics.allocator, &live);
  Check(live.total.statistics.allocationCount == 1,
        "only test readback remains after callback drain");
  std::printf("[vma] completion:%s full buffers compared; pending retention "
              "and retirement checked\n",
              variant);
  return true;
}
