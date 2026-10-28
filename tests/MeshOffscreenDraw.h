// P4.2 standalone offscreen indexed draw. Uses the P4.1 compute pipeline and
// output contract; no emulator code or guest shaders. Included in the test TU.
constexpr uint32_t kDrawSize = 64;
constexpr size_t kDrawBytes = kDrawSize * kDrawSize * 4;

// Pixel centers never lie on this triangle's edges: screen vertices are
// (16,16), (48,16), (16,47.5). Integer edge math avoids tolerance/driver guesses.
bool TrianglePixel(uint32_t x, uint32_t y) {
	return x >= 16 && y >= 16 &&
	       63 * (2 * static_cast<int>(x) + 1 - 32) +
	       64 * (2 * static_cast<int>(y) + 1 - 32) < 4032;
}
size_t PixelMismatches(const uint8_t* rgba, bool triangle) {
	size_t mismatches = 0;
	for (uint32_t y = 0; y < kDrawSize; ++y) {
		for (uint32_t x = 0; x < kDrawSize; ++x) {
			const size_t at = (y * kDrawSize + x) * 4;
			const uint8_t color = triangle && TrianglePixel(x, y) ? 255 : 0;
			if (rgba[at] != color || rgba[at + 1] != color ||
			    rgba[at + 2] != color || rgba[at + 3] != 255) { ++mismatches; }
		}
	}
	return mismatches;
}
int CheckDrawOracle() {
	std::vector<uint8_t> pixels(kDrawBytes, 0);
	size_t covered = 0;
	for (uint32_t y = 0; y < kDrawSize; ++y) {
		for (uint32_t x = 0; x < kDrawSize; ++x) {
			const size_t at = (y * kDrawSize + x) * 4;
			if (TrianglePixel(x, y)) {
				std::fill_n(pixels.data() + at, 3, 255);
				++covered;
			}
			pixels[(y * kDrawSize + x) * 4 + 3] = 255;
		}
	}
	Expect(covered == 496, "offscreen oracle has independently counted 496 white pixels");
	Expect(PixelMismatches(pixels.data(), true) == 0, "complete reference image accepted");
	Expect(PixelMismatches(pixels.data(), false) == covered, "draw cannot pass clear-only oracle");
	pixels[(20 * kDrawSize + 20) * 4] = 0;
	Expect(PixelMismatches(pixels.data(), true) == 1, "one corrupted interior channel rejected");
	pixels[(20 * kDrawSize + 20) * 4] = 255;
	pixels[3] = 0;
	Expect(PixelMismatches(pixels.data(), true) == 1, "background alpha corruption rejected");
	std::fill(pixels.begin(), pixels.end(), 255);
	Expect(PixelMismatches(pixels.data(), true) == kDrawSize * kDrawSize - covered,
	       "all-white image rejected");
	std::fill(pixels.begin(), pixels.end(), 0);
	for (size_t i = 3; i < pixels.size(); i += 4) { pixels[i] = 255; }
	Expect(PixelMismatches(pixels.data(), true) == covered, "missing draw rejected");
	Expect(PixelMismatches(pixels.data(), false) == 0, "clear-only image accepted");
	PrintLine1("draw-oracle: %s (496 covered pixels, missing draw and channel/alpha corruption checks)",
	           g_failures == 0 ? "PASS" : "FAIL");
	return g_failures == 0 ? 0 : 1;
}

std::filesystem::path DrawModulePath(bool vulkan13, const char* stage) {
	return g_shader_directory / (std::string("mesh_draw_") + stage + (vulkan13 ? "_vk13.spv" : "_vk12.spv"));
}
bool ValidateDrawShaders() {
	for (bool api13: {true, false}) {
		for (const char* stage: {"vert", "frag"}) {
			const auto words = LoadSpirvFile(DrawModulePath(api13, stage), stage);
			const auto env = api13 ? SPV_ENV_VULKAN_1_3 : SPV_ENV_VULKAN_1_2;
			if (!ValidateSpirv(words, env, stage)) { return false; }
			spvtools::SpirvTools tools(env);
			std::string text;
			Expect(tools.Disassemble(words, &text), "draw shader disassembly");
			Expect(text.find(std::strcmp(stage, "vert") == 0 ? "Vertex" : "Fragment") != std::string::npos,
			       "draw shader execution model");
		}
	}
	return g_failures == 0;
}

struct DrawScopeExit {
	std::function<void()> cleanup;
	~DrawScopeExit() { cleanup(); }
};

bool RunOffscreenDraw(VkDevice device, PFN_vkGetDeviceProcAddr proc,
                      const VkPhysicalDeviceMemoryProperties& memory, bool api13,
                      VkQueue queue, VkCommandBuffer cmd, VkFence fence,
                      VkPipeline compute, VkPipelineLayout compute_layout, VkDescriptorSet set,
                      GpuBuffer& output, GpuBuffer& payload, const std::filesystem::path& image_path) {
	// Resolve every entry point, including teardown, before creating any draw objects.
#define DRAW_PROC(name) auto name = reinterpret_cast<PFN_vk##name>(proc(device, "vk" #name)); \
	if (name == nullptr) { Expect(false, "draw entry point vk" #name); return false; }
	DRAW_PROC(CreateImage) DRAW_PROC(DestroyImage) DRAW_PROC(GetImageMemoryRequirements)
	DRAW_PROC(AllocateMemory) DRAW_PROC(FreeMemory) DRAW_PROC(BindImageMemory)
	DRAW_PROC(CreateImageView) DRAW_PROC(DestroyImageView)
	DRAW_PROC(CreateRenderPass) DRAW_PROC(DestroyRenderPass)
	DRAW_PROC(CreateFramebuffer) DRAW_PROC(DestroyFramebuffer)
	DRAW_PROC(CreateShaderModule) DRAW_PROC(DestroyShaderModule)
	DRAW_PROC(CreatePipelineLayout) DRAW_PROC(DestroyPipelineLayout)
	DRAW_PROC(CreateGraphicsPipelines) DRAW_PROC(DestroyPipeline)
	DRAW_PROC(DestroyBuffer) DRAW_PROC(UnmapMemory) DRAW_PROC(DeviceWaitIdle)
	DRAW_PROC(ResetCommandBuffer) DRAW_PROC(BeginCommandBuffer) DRAW_PROC(EndCommandBuffer)
	DRAW_PROC(CmdPipelineBarrier) DRAW_PROC(CmdBindPipeline) DRAW_PROC(CmdBindDescriptorSets)
	DRAW_PROC(CmdPushConstants) DRAW_PROC(CmdDispatch) DRAW_PROC(CmdBeginRenderPass)
	DRAW_PROC(CmdEndRenderPass) DRAW_PROC(CmdBindVertexBuffers) DRAW_PROC(CmdBindIndexBuffer)
	DRAW_PROC(CmdDrawIndexed) DRAW_PROC(CmdCopyImageToBuffer) DRAW_PROC(QueueSubmit)
	DRAW_PROC(WaitForFences) DRAW_PROC(ResetFences)
	DRAW_PROC(FlushMappedMemoryRanges) DRAW_PROC(InvalidateMappedMemoryRanges)
#undef DRAW_PROC
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory image_memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkRenderPass pass = VK_NULL_HANDLE;
	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;
	VkPipeline graphics = VK_NULL_HANDLE;
	VkShaderModule modules[2] = {};
	GpuBuffer readback;
	DrawScopeExit scope{[&]() {
		CheckVk(DeviceWaitIdle(device), "draw idle before cleanup");
		if (graphics) { DestroyPipeline(device, graphics, nullptr); }
		if (layout) { DestroyPipelineLayout(device, layout, nullptr); }
		for (auto module: modules) { if (module) { DestroyShaderModule(device, module, nullptr); } }
		if (framebuffer) { DestroyFramebuffer(device, framebuffer, nullptr); }
		if (pass) { DestroyRenderPass(device, pass, nullptr); }
		if (view) { DestroyImageView(device, view, nullptr); }
		if (image) { DestroyImage(device, image, nullptr); }
		if (image_memory) { FreeMemory(device, image_memory, nullptr); }
		if (readback.mapped) { UnmapMemory(device, readback.memory); }
		if (readback.buffer) { DestroyBuffer(device, readback.buffer, nullptr); }
		if (readback.memory) { FreeMemory(device, readback.memory, nullptr); }
	}};
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
	image_info.extent = {kDrawSize, kDrawSize, 1};
	image_info.mipLevels = image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (!CheckVk(CreateImage(device, &image_info, nullptr, &image), "offscreen image creation")) { return false; }
	VkMemoryRequirements requirements;
	GetImageMemoryRequirements(device, image, &requirements);
	const uint32_t type = PickMemoryType(memory, requirements.memoryTypeBits, 0, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
	if (type == UINT32_MAX) { Expect(false, "offscreen image memory type"); return false; }
	VkMemoryAllocateInfo allocation = {};
	allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation.allocationSize = requirements.size;
	allocation.memoryTypeIndex = type;
	if (!CheckVk(AllocateMemory(device, &allocation, nullptr, &image_memory), "offscreen image allocation") ||
	    !CheckVk(BindImageMemory(device, image, image_memory, 0), "offscreen image bind")) { return false; }
	VkImageViewCreateInfo view_info = {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = image_info.format;
	view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
	if (!CheckVk(CreateImageView(device, &view_info, nullptr, &view), "offscreen view")) { return false; }
	VkAttachmentDescription attachment = {};
	attachment.format = image_info.format;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	// Explicit image barriers outside the pass handle both transitions.
	attachment.initialLayout = attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference reference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &reference;
	VkRenderPassCreateInfo pass_info = {};
	pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	pass_info.attachmentCount = pass_info.subpassCount = 1;
	pass_info.pAttachments = &attachment;
	pass_info.pSubpasses = &subpass;
	if (!CheckVk(CreateRenderPass(device, &pass_info, nullptr, &pass), "offscreen render pass")) { return false; }
	VkFramebufferCreateInfo fb = {};
	fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb.renderPass = pass;
	fb.attachmentCount = 1;
	fb.pAttachments = &view;
	fb.width = fb.height = kDrawSize;
	fb.layers = 1;
	if (!CheckVk(CreateFramebuffer(device, &fb, nullptr, &framebuffer), "offscreen framebuffer")) { return false; }
	VkPipelineShaderStageCreateInfo stages[2] = {};
	for (size_t i = 0; i < 2; ++i) {
		const auto words = LoadSpirvFile(DrawModulePath(api13, i == 0 ? "vert" : "frag"), "draw-runtime");
		if (words.empty()) { return false; }
		VkShaderModuleCreateInfo sm = {};
		sm.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		sm.codeSize = words.size() * 4;
		sm.pCode = words.data();
		if (!CheckVk(CreateShaderModule(device, &sm, nullptr, &modules[i]), "draw shader creation")) { return false; }
		stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[i].stage = i == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[i].module = modules[i];
		stages[i].pName = "main";
	}
	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	if (!CheckVk(CreatePipelineLayout(device, &layout_info, nullptr, &layout), "draw pipeline layout")) { return false; }
	VkVertexInputBindingDescription binding = {0, 16, VK_VERTEX_INPUT_RATE_VERTEX};
	VkVertexInputAttributeDescription attribute = {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0};
	VkPipelineVertexInputStateCreateInfo vertex = {};
	vertex.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex.vertexBindingDescriptionCount = vertex.vertexAttributeDescriptionCount = 1;
	vertex.pVertexBindingDescriptions = &binding;
	vertex.pVertexAttributeDescriptions = &attribute;
	VkPipelineInputAssemblyStateCreateInfo assembly = {};
	assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkViewport viewport = {0, 0, float(kDrawSize), float(kDrawSize), 0, 1};
	VkRect2D scissor = {{0, 0}, {kDrawSize, kDrawSize}};
	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = vp.scissorCount = 1;
	vp.pViewports = &viewport;
	vp.pScissors = &scissor;
	VkPipelineRasterizationStateCreateInfo raster = {};
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	raster.lineWidth = 1;
	VkPipelineMultisampleStateCreateInfo ms = {};
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	VkPipelineColorBlendAttachmentState color = {};
	color.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo blend = {};
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments = &color;
	VkGraphicsPipelineCreateInfo graphics_info = {};
	graphics_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_info.stageCount = 2;
	graphics_info.pStages = stages;
	graphics_info.pVertexInputState = &vertex;
	graphics_info.pInputAssemblyState = &assembly;
	graphics_info.pViewportState = &vp;
	graphics_info.pRasterizationState = &raster;
	graphics_info.pMultisampleState = &ms;
	graphics_info.pColorBlendState = &blend;
	graphics_info.layout = layout;
	graphics_info.renderPass = pass;
	if (!CheckVk(CreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphics_info, nullptr, &graphics), "graphics pipeline")) { return false; }
	if (AllocateMappedBuffer(device, proc, memory, kDrawBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
	                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                         readback, "image-readback") != 0) { return false; }
	// Two sequential GPU submissions: clear-only control, then freshly dispatched
	// geometry consumed directly by the draw in the SAME submission (no CPU gate).
	for (bool draw: {false, true}) {
		Scenario scenario;
		scenario.name = "offscreen-triangle";
		scenario.vertex_count = 3;
		scenario.primitive_count = 1;
		scenario.initial.Init();
		scenario.payload_positions.assign(kPositionWords, 0);
		scenario.payload_indices.assign(kIndexWords, 0);
		const float positions[12] = {-0.5f,-0.5f,0,1, 0.5f,-0.5f,0,1, -0.5f,0.484375f,0,1};
		std::memcpy(scenario.payload_positions.data(), positions, sizeof(positions));
		scenario.payload_indices[0] = 2; scenario.payload_indices[1] = 0; scenario.payload_indices[2] = 1;
		FinalizeScenario(scenario);
		std::memcpy(output.mapped, scenario.initial.words.data(), kTotalBytes);
		std::memcpy(payload.mapped, scenario.payload_positions.data(), kPositionWords * 4);
		std::memcpy(static_cast<uint8_t*>(payload.mapped) + kPositionWords * 4,
		            scenario.payload_indices.data(), kIndexWords * 4);
		std::memset(readback.mapped, 0xA5, kDrawBytes);
		for (auto* buffer: {&output, &payload, &readback}) {
			if (buffer->noncoherent) {
				VkMappedMemoryRange range = {};
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.memory = buffer->memory;
				range.size = VK_WHOLE_SIZE;
				if (!CheckVk(FlushMappedMemoryRanges(device, 1, &range), "draw flush host writes")) { return false; }
			}
		}
		if (!CheckVk(ResetCommandBuffer(cmd, 0), "draw command reset")) { return false; }
		VkCommandBufferBeginInfo begin = {};
		begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (!CheckVk(BeginCommandBuffer(cmd, &begin), "draw command begin")) { return false; }
		VkMemoryBarrier upload = {};
		upload.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		upload.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		upload.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
		CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT,
		                   VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
		                   0, 1, &upload, 0, nullptr, 0, nullptr);
		if (draw) {
			CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute);
			CmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, compute_layout, 0, 1, &set, 0, nullptr);
			const uint32_t counts[2] = {3, 1};
			CmdPushConstants(cmd, compute_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(counts), counts);
			CmdDispatch(cmd, 1, 1, 1);
			VkBufferMemoryBarrier geometry = {};
			geometry.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			geometry.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			geometry.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
			geometry.srcQueueFamilyIndex = geometry.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			geometry.buffer = output.buffer;
			geometry.size = kTotalBytes;
			CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			                   0, 0, nullptr, 1, &geometry, 0, nullptr);
		}
		VkImageMemoryBarrier image_barrier = {};
		image_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		image_barrier.oldLayout = draw ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		image_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier.srcAccessMask = draw ? VK_ACCESS_TRANSFER_READ_BIT : 0;
		image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_barrier.srcQueueFamilyIndex = image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		image_barrier.image = image;
		image_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		CmdPipelineBarrier(cmd, draw ? VK_PIPELINE_STAGE_TRANSFER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &image_barrier);
		VkClearValue clear = {};
		clear.color.float32[3] = 1;
		VkRenderPassBeginInfo render = {};
		render.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render.renderPass = pass;
		render.framebuffer = framebuffer;
		render.renderArea = scissor;
		render.clearValueCount = 1;
		render.pClearValues = &clear;
		CmdBeginRenderPass(cmd, &render, VK_SUBPASS_CONTENTS_INLINE);
		if (draw) {
			CmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics);
			const VkDeviceSize offset = kPositionsOffsetBytes;
			CmdBindVertexBuffers(cmd, 0, 1, &output.buffer, &offset);
			CmdBindIndexBuffer(cmd, output.buffer, kIndicesOffsetBytes, VK_INDEX_TYPE_UINT32);
			CmdDrawIndexed(cmd, 3, 1, 0, 0, 0);
		}
		CmdEndRenderPass(cmd);
		image_barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		image_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		                   0, 0, nullptr, 0, nullptr, 1, &image_barrier);
		VkBufferImageCopy copy = {};
		copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		copy.imageExtent = image_info.extent;
		CmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &copy);
		VkMemoryBarrier host = {};
		host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
		CmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		                   VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
		if (!CheckVk(EndCommandBuffer(cmd), "draw command end")) { return false; }
		VkSubmitInfo submit = {};
		submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit.commandBufferCount = 1;
		submit.pCommandBuffers = &cmd;
		if (!CheckVk(QueueSubmit(queue, 1, &submit, fence), "draw queue submit") ||
		    !CheckVk(WaitForFences(device, 1, &fence, VK_TRUE, 10000000000ull), "draw fence within 10s") ||
		    !CheckVk(ResetFences(device, 1, &fence), "draw fence reset")) { return false; }
		for (auto* buffer: {&output, &readback}) {
			if (buffer->noncoherent) {
				VkMappedMemoryRange range = {};
				range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				range.memory = buffer->memory;
				range.size = VK_WHOLE_SIZE;
				if (!CheckVk(InvalidateMappedMemoryRanges(device, 1, &range), "draw invalidate readback")) { return false; }
			}
		}
		MeshOutputBlob actual;
		std::memcpy(actual.words.data(), output.mapped, kTotalBytes);
		CompareBlobWords(actual, draw ? scenario.expected : scenario.initial, "draw exact full output blob including guards");
		const size_t mismatches = PixelMismatches(static_cast<const uint8_t*>(readback.mapped), draw);
		PrintLine1("offscreen %s: 64x64 RGBA pixels mismatched=%zu/4096; full blob checked",
		           draw ? "compute-indexed-triangle" : "clear-only-control", mismatches);
		Expect(mismatches == 0, "all 4096 offscreen RGBA pixels match");
		if (draw && !image_path.empty()) {
			std::ofstream file(image_path, std::ios::binary | std::ios::trunc);
			file << "P6\n64 64\n255\n";
			for (size_t i = 0; i < kDrawBytes; i += 4) {
				file.write(static_cast<const char*>(readback.mapped) + i, 3);
			}
			file.close();
			Expect(!file.fail(), "offscreen PPM artifact saved");
			PrintLine1("offscreen-image: %s", PathText(image_path).c_str());
		}
		if (g_failures != 0 || g_validation_errors != 0) { return false; }
	}
	return true; // scope cleanup runs before caller's final validation summary
}
