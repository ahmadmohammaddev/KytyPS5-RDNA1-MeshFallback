// P4.1 RDNA1 mesh-fallback research (docs/RDNA1_MESH_FALLBACK_PLAN.md): a synthetic
// Vulkan compute prototype of the hardened mesh-output contract in
// tests/MeshOutputContract.h (host checker: tests/MeshOutputContractTests.cpp).
//
// Mode --validate-spirv: offline SPIR-V validation only (no Vulkan device). The shader
// tests/shaders/mesh_output_proto.comp is compiled at build time twice by the pinned
// glslang: once for --target-env vulkan1.3 (SPIR-V 1.6, validated for SPV_ENV_VULKAN_1_3,
// the plan's Vulkan-1.3 offline gate) and once for --target-env vulkan1.2 (SPIR-V 1.5,
// the runtime variant). Both modules are validated here with the pinned SPIRV-Tools
// library, plus structural shape checks so the tested file is the authored shader.
//
// Mode --run: strict Vulkan 1.3 runtime gate. Mode --run-vulkan12: explicitly
// auxiliary Vulkan 1.2 evidence. Both perform offline validation, then dispatch on a
// device discovered through the system loader (vulkan-1.dll): every physical device is
// reported, the KHRONOS validation layer enables core and synchronization validation
// (VkValidationFeaturesEXT) as a prerequisite, the fixed-layout output buffer plus the exact
// example payload are uploaded through mapped memory (explicit flush for non-coherent
// memory), the compute shader writes the blob, an explicit fence gates host readback with
// the mandatory invalidate for non-coherent memory, and every one of the 460 output words
// is compared against the host reference blob built by the shared contract header. The
// dispatched module and the physical device/driver identity are recorded. A missing
// loader, validation facility or qualifying device is a clean skip (exit 41), never a
// pass; every runtime comparison failure is a fail. Intel (Iris Xe) or software-ICD
// results are Intel/software evidence only and cannot close an RDNA1 hardware gate, and
// this rig proves nothing about PS5 mesh shaders or game compatibility.

// Driver-level guard: this test reads the environment for loader discovery only.
#define _CRT_SECURE_NO_WARNINGS

#include "MeshOutputContract.h"

#include <spirv-tools/libspirv.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <vulkan/vulkan.h>

namespace {

using namespace Kyty::MeshOutput;

// ---------------------------------------------------------------------------
// Small shared helpers
// ---------------------------------------------------------------------------

int g_failures = 0;
std::atomic<int> g_validation_errors{0};
std::atomic<int> g_validation_warnings{0};
constexpr int kSkip = 41; // All unavailable-environment paths use CTest's registered code.
std::filesystem::path g_shader_directory;

std::string PathText(const std::filesystem::path& path) {
	const auto utf8 = path.u8string();
	return std::string(utf8.begin(), utf8.end());
}

bool LocateShaders(const char* executable) {
#ifdef _WIN32
	std::wstring buffer(32768, L'\0');
	const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
	if (length == 0 || length >= buffer.size()) { return false; }
	buffer.resize(length);
	const std::filesystem::path executable_path(buffer);
#else
	std::error_code error;
	auto executable_path = std::filesystem::read_symlink("/proc/self/exe", error);
	if (error) { executable_path = std::filesystem::absolute(executable, error); }
	if (error) { return false; }
#endif
	g_shader_directory = executable_path.parent_path() / "gpu_mesh_proto_shaders";
	return true;
}

std::filesystem::path ModulePath(bool vulkan13) {
	return g_shader_directory / (vulkan13 ? "mesh_output_proto_vk13.spv" : "mesh_output_proto_vk12.spv");
}

void PrintLine1(const char* format, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, format);
	std::vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	std::printf("[p41] %s\n", buffer);
}

void Expect(bool condition, const char* what) {
	if (!condition) {
		PrintLine1("FAILED: %s", what);
		g_failures++;
	}
}

bool CheckVk(VkResult result, const char* what) {
	if (result != VK_SUCCESS) {
		PrintLine1("API result %d: %s", static_cast<int>(result), what);
		Expect(false, what);
		return false;
	}
	return true;
}

std::string ApiString(uint32_t version) {
	return std::to_string(version >> 22) + "." +
	       std::to_string((version >> 12) & 0x3FF) + "." + std::to_string(version & 0xFFF);
}

VKAPI_ATTR VkBool32 VKAPI_CALL OnValidationMessage(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT,
	const VkDebugUtilsMessengerCallbackDataEXT* data, void*) {
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		const int count = ++g_validation_errors;
		if (count <= 12 && data != nullptr) {
			PrintLine1("validation-error: %s",
			           data->pMessage != nullptr ? data->pMessage : "(no text)");
		}
	} else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		const int count = ++g_validation_warnings;
		if (count <= 12 && data != nullptr) {
			PrintLine1("validation-warning: %s",
			           data->pMessage != nullptr ? data->pMessage : "(no text)");
		}
	}
	return VK_FALSE;
}

const char* DeviceTypeString(VkPhysicalDeviceType type) {
	switch (type) {
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED_GPU";
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "DISCRETE_GPU";
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VIRTUAL_GPU";
	case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU (software ICD)";
	default: return "OTHER";
	}
}

// ---------------------------------------------------------------------------
// Offline SPIR-V gates (shader compilation happened at build time via the pinned
// glslang; modules are resolved beside the executable, independent of working directory)
// ---------------------------------------------------------------------------

std::vector<uint32_t> LoadSpirvFile(const std::filesystem::path& path, const char* label) {
	std::ifstream file(path, std::ios::binary);
	if (!file) {
		Expect(false, "SPIR-V module file opens");
		PrintLine1("module-missing: %s (%s)", label, PathText(path).c_str());
		return {};
	}
	std::vector<char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	if ((bytes.size() % sizeof(uint32_t)) != 0) {
		Expect(false, "SPIR-V module size is a whole word count");
		return {};
	}
	std::vector<uint32_t> binary(bytes.size() / sizeof(uint32_t));
	std::memcpy(binary.data(), bytes.data(), bytes.size());
	if (binary.size() < 5 || binary[0] != 0x07230203u) {
		Expect(false, "SPIR-V module magic word");
		return {};
	}
	PrintLine1("module-loaded: %s SPIR-V %u.%u (%zu words)", label,
	           (binary[1] >> 16) & 0x7Fu, (binary[1] >> 8) & 0xFFu, binary.size());
	return binary;
}

bool ValidateSpirv(const std::vector<uint32_t>& binary, spv_target_env env, const char* label) {
	if (binary.empty()) {
		Expect(false, label);
		return false;
	}
	spvtools::SpirvTools tools(env);
	tools.SetMessageConsumer([](spv_message_level_t level, const char*, const spv_position_t&,
	                            const char* message) {
		if (level == SPV_MSG_ERROR || level == SPV_MSG_INTERNAL_ERROR) {
			PrintLine1("validator: %s", message);
		}
	});
	const bool valid = tools.Validate(binary);
	PrintLine1("spirv-validation: %s %s", label, valid ? "PASS" : "FAIL");
	Expect(valid, label);
	return valid;
}

void CheckModuleShape(const std::vector<uint32_t>& binary, spv_target_env env, const char* label) {
	if (binary.empty()) {
		return;
	}
	spvtools::SpirvTools tools(env);
	std::string text;
	Expect(tools.Disassemble(binary, &text,
	                        SPV_BINARY_TO_TEXT_OPTION_NO_HEADER | SPV_BINARY_TO_TEXT_OPTION_FRIENDLY_NAMES),
	       "SPIR-V disassembly succeeds");
	const std::string name(label);
	Expect(text.find("GLCompute") != std::string::npos,
	       (name + " uses the GLCompute execution model").c_str());
	Expect(text.find("LocalSize 1 1 1") != std::string::npos,
	       (name + " uses a 1x1x1 workgroup").c_str());
	Expect(text.find("Binding 0") != std::string::npos, (name + " declares binding 0").c_str());
	Expect(text.find("Binding 1") != std::string::npos, (name + " declares binding 1").c_str());
	Expect(text.find("PushConstant") != std::string::npos,
	       (name + " declares push constants").c_str());
}

bool RunOfflineValidation() {
	PrintLine1("shader-directory: %s", PathText(g_shader_directory).c_str());
	const std::vector<uint32_t> vk13 = LoadSpirvFile(ModulePath(true), "vulkan1.3");
	const std::vector<uint32_t> vk12 = LoadSpirvFile(ModulePath(false), "vulkan1.2");
	Expect(!vk13.empty(), "vulkan1.3 module present");
	Expect(!vk12.empty(), "vulkan1.2 module present");
	CheckModuleShape(vk13, SPV_ENV_VULKAN_1_3, "vulkan1.3");
	CheckModuleShape(vk12, SPV_ENV_VULKAN_1_2, "vulkan1.2");
	bool ok = ValidateSpirv(vk13, SPV_ENV_VULKAN_1_3, "vulkan1.3 module (SPV_ENV_VULKAN_1_3)");
	if (!ValidateSpirv(vk12, SPV_ENV_VULKAN_1_2, "vulkan1.2 module (SPV_ENV_VULKAN_1_2)")) {
		ok = false;
	}
	ok = ok && g_failures == 0;
	PrintLine1("spirv-validation: summary %s", ok ? "PASS (validation and shape, both modules)" : "FAIL");
	return ok;
}

// ---------------------------------------------------------------------------
// The contracted scenarios mirrored from the host test; expectations are computed
// with the same authoritative header functions the host test uses.
// ---------------------------------------------------------------------------

struct Scenario {
	std::string          name;
	uint32_t             vertex_count    = 0;
	uint32_t             primitive_count = 0;
	bool                 dispatch        = true;
	bool                 upload_output   = true;
	std::vector<uint32_t> payload_positions; // kPositionWords (256) words
	std::vector<uint32_t> payload_indices;   // kIndexWords (192) words
	MeshOutputBlob       initial;
	MeshOutputBlob       expected;
};

std::vector<uint32_t> ScriptPositions(const std::array<uint32_t, 12>& prefix, uint32_t fill,
                                      size_t keep) {
	std::vector<uint32_t> words(static_cast<size_t>(kPositionWords));
	for (size_t i = 0; i < words.size(); ++i) {
		if (i < keep) {
			words[i] = (i < prefix.size()) ? prefix[i] : 0u;
		} else {
			words[i] = fill;
		}
	}
	return words;
}

std::vector<uint32_t> ScriptIndices(const std::array<uint32_t, 3>& prefix, uint32_t fill,
                                    size_t keep) {
	std::vector<uint32_t> words(static_cast<size_t>(kIndexWords));
	for (size_t i = 0; i < words.size(); ++i) {
		if (i < keep) {
			words[i] = (i < prefix.size()) ? prefix[i] : 0u;
		} else {
			words[i] = fill;
		}
	}
	return words;
}

void FinalizeScenario(Scenario& scenario) {
	scenario.expected = scenario.initial;
	if (!scenario.dispatch) {
		return; // An upload without a dispatch must retain Pending, not commit Ok.
	}
	const MeshOutputWrite write = {scenario.vertex_count, scenario.payload_positions.data(),
	                               scenario.primitive_count, scenario.payload_indices.data()};
	(void)WriteOutput(scenario.expected, write);
}

std::vector<Scenario> MakeScenarios() {
	std::vector<Scenario> scenarios;
	scenarios.reserve(9);

	// The initialized blob check: guards, Pending status, everything else zero, and no
	// dispatch at all; still exercises the full upload/submit/fence/readback path with
	// its mandatory non-coherent actions so the identity of that path is proven too.
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "pending-init-upload";
		s.dispatch        = false;
		s.vertex_count    = 0;
		s.primitive_count = 0;
		s.payload_positions.assign(static_cast<size_t>(kPositionWords), 0u);
		s.payload_indices.assign(static_cast<size_t>(kIndexWords), 0u);
		s.initial.Init();
		FinalizeScenario(s);
	}
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "zero-output";
		s.vertex_count    = 0;
		s.primitive_count = 0;
		s.payload_positions.assign(static_cast<size_t>(kPositionWords), 0u);
		s.payload_indices.assign(static_cast<size_t>(kIndexWords), 0u);
		s.initial.Init();
		FinalizeScenario(s);
	}
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "prototype-triangle";
		s.vertex_count    = 3;
		s.primitive_count = 1;
		s.payload_positions = ScriptPositions(PrototypePositions(), 0u, 12);
		s.payload_indices   = ScriptIndices({0u, 1u, 2u}, 0u, 3);
		s.initial.Init();
		FinalizeScenario(s);
	}
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "exact-capacity";
		s.vertex_count    = kMaxVertices;
		s.primitive_count = kMaxPrimitives;
		s.payload_positions.assign(static_cast<size_t>(kPositionWords), 0u);
		for (size_t i = 0; i < s.payload_positions.size(); ++i) {
			s.payload_positions[i] = uint32_t(0x10000000u + i);
		}
		s.payload_indices.assign(static_cast<size_t>(kIndexWords), 0u);
		for (size_t i = 0; i < s.payload_indices.size(); ++i) {
			s.payload_indices[i] = uint32_t(i % static_cast<size_t>(kMaxVertices));
		}
		s.initial.Init();
		FinalizeScenario(s);
	}
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "overflow-vertices";
		s.vertex_count    = kMaxVertices + 1;
		s.primitive_count = 1;
		s.payload_positions.assign(static_cast<size_t>(kPositionWords), 0x55555555u);
		s.payload_indices = ScriptIndices({0u, 1u, 2u}, 0x55555555u, 3);
		InitOverflowSnapshot(s.initial);
		FinalizeScenario(s);
	}
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "overflow-indices";
		s.vertex_count    = 3;
		s.primitive_count = kMaxPrimitives + 1;
		s.payload_positions = ScriptPositions(PrototypePositions(), 0x55555555u, 12);
		s.payload_indices.assign(static_cast<size_t>(kIndexWords), 0x55555555u);
		InitOverflowSnapshot(s.initial);
		FinalizeScenario(s);
	}
	// Actual same-buffer recovery: this dispatch reuses the preceding GPU output.
	{
		const MeshOutputBlob initial = scenarios.back().expected;
		Scenario& s = scenarios.emplace_back();
		s.name = "index-overflow-recovery";
		s.upload_output = false;
		s.vertex_count = kMaxVertices;
		s.primitive_count = kMaxPrimitives;
		s.payload_positions.assign(kPositionWords, 0x12345678u);
		s.payload_indices.assign(kIndexWords, 0x2468ACE0u);
		s.initial = initial;
		FinalizeScenario(s);
	}
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "overflow-combined";
		s.vertex_count    = kMaxVertices + 1;
		s.primitive_count = kMaxPrimitives + 1;
		s.payload_positions.assign(static_cast<size_t>(kPositionWords), 0x55555555u);
		s.payload_indices.assign(static_cast<size_t>(kIndexWords), 0x55555555u);
		InitOverflowSnapshot(s.initial);
		FinalizeScenario(s);
	}
	// Reuse the preceding combined-overflow GPU result (vertex error has priority).
	{
		Scenario& s = scenarios.emplace_back();
		s.name            = "overflow-recovery";
		s.upload_output   = false;
		s.vertex_count    = kMaxVertices;
		s.primitive_count = kMaxPrimitives;
		s.payload_positions.assign(static_cast<size_t>(kPositionWords), 0x12345678u);
		s.payload_indices.assign(static_cast<size_t>(kIndexWords), 0x2468ACE0u);
		s.initial = scenarios[scenarios.size() - 2].expected;
		FinalizeScenario(s);
	}
	return scenarios;
}

void CompareBlobWords(const MeshOutputBlob& actual, const MeshOutputBlob& expected,
                      const char* what) {
	size_t mismatches = 0;
	for (size_t i = 0; i < kTotalWords; ++i) {
		if (actual.words[i] != expected.words[i]) {
			if (mismatches < 8) {
				PrintLine1("  word %zu: got %08X expected %08X", i,
			               static_cast<unsigned>(actual.words[i]),
			               static_cast<unsigned>(expected.words[i]));
			}
			++mismatches;
		}
	}
	if (mismatches != 0) {
		PrintLine1("  %zu of %zu words differ", mismatches, kTotalWords);
	}
	Expect(mismatches == 0, what);
}

// ---------------------------------------------------------------------------
// Dynamic Vulkan loader plumbing (no statically linked loader; no SDK)
// ---------------------------------------------------------------------------

#ifdef _WIN32
void* LoadLoaderLibrary() {
	return reinterpret_cast<void*>(LoadLibraryExW(L"vulkan-1.dll", nullptr,
	                                              LOAD_LIBRARY_SEARCH_SYSTEM32 |
	                                                  LOAD_LIBRARY_SEARCH_DEFAULT_DIRS));
}
void* GetLoaderProcAddress(void* library, const char* name) {
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(library), name));
}
void UnloadLoaderLibrary(void* library) { FreeLibrary(static_cast<HMODULE>(library)); }
#else
void* LoadLoaderLibrary() { return dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL); }
void* GetLoaderProcAddress(void* library, const char* name) { return dlsym(library, name); }
void UnloadLoaderLibrary(void* library) { dlclose(library); }
#endif


struct GpuBuffer {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	void* mapped = nullptr;
	uint32_t memory_type = 0;
	VkDeviceSize size = 0;
	VkDeviceSize allocation_size = 0;
	bool noncoherent = false;
};

// Host-visible allocation preference: both buffers are host-visible mapped; the output
// prefers device-local memory (non-coherence is detected from the chosen type and it
// drives the mandatory flush/invalidate whenever required), and the input payload
// prefers plain coherent mapping. The chosen type is recorded for the experiment evidence.
uint32_t PickMemoryType(const VkPhysicalDeviceMemoryProperties& properties,
                        uint32_t allowed_types,
                        VkMemoryPropertyFlags must_have, VkMemoryPropertyFlags prefer,
                        bool* noncoherent) {
	uint32_t best = UINT32_MAX;
	int best_score = -1;
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
		if ((allowed_types & (1u << i)) == 0) {
			continue;
		}
		const VkMemoryPropertyFlags flags = properties.memoryTypes[i].propertyFlags;
		if ((flags & must_have) != must_have) {
			continue;
		}
		int score = 0;
		if ((flags & prefer) == prefer) {
			score = 100;
		} else {
			score = ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0 ? 2 : 0) +
			        ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0 ? 1 : 0);
		}
		if (score > best_score) {
			best_score = score;
			best = i;
		}
	}
	if (best != UINT32_MAX && noncoherent != nullptr) {
		// There is no "not host coherent" flag bit; non-coherence is the absence
		// of VK_MEMORY_PROPERTY_HOST_COHERENT_BIT on a host-visible mapped type.
		*noncoherent =
		    (properties.memoryTypes[best].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0;
	}
	return best;
}

int AllocateMappedBuffer(VkDevice device, PFN_vkGetDeviceProcAddr get_device_proc,
                         const VkPhysicalDeviceMemoryProperties& properties, VkDeviceSize size,
                         VkBufferUsageFlags usage, VkMemoryPropertyFlags must_have,
                         VkMemoryPropertyFlags prefer, GpuBuffer& out, const char* label) {
	auto dproc = [&](const char* name) { return get_device_proc(device, name); };
	auto create_buffer = reinterpret_cast<PFN_vkCreateBuffer>(dproc("vkCreateBuffer"));
	auto get_requirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(
	    dproc("vkGetBufferMemoryRequirements"));
	auto allocate_memory = reinterpret_cast<PFN_vkAllocateMemory>(dproc("vkAllocateMemory"));
	auto bind_buffer = reinterpret_cast<PFN_vkBindBufferMemory>(dproc("vkBindBufferMemory"));
	auto map_memory = reinterpret_cast<PFN_vkMapMemory>(dproc("vkMapMemory"));
	if (create_buffer == nullptr || get_requirements == nullptr || allocate_memory == nullptr ||
	    bind_buffer == nullptr || map_memory == nullptr) {
		Expect(false, "required buffer entry points resolve");
		return 1;
	}
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (create_buffer == nullptr ||
	    create_buffer(device, &buffer_info, nullptr, &out.buffer) != VK_SUCCESS) {
		PrintLine1("buffer: %s create failed", label);
		Expect(false, label);
		return 1;
	}
	VkMemoryRequirements requirements = {};
	get_requirements(device, out.buffer, &requirements);
	const uint32_t type_index = PickMemoryType(properties, requirements.memoryTypeBits,
	                                           must_have, prefer, &out.noncoherent);
	if (type_index == UINT32_MAX || (requirements.memoryTypeBits & (1u << type_index)) == 0) {
		PrintLine1("buffer: %s no usable memory type (required mask 0x%X)", label,
		           requirements.memoryTypeBits);
		Expect(false, label);
		return 1;
	}
	out.memory_type = type_index;
	VkMemoryAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = requirements.size;
	allocate_info.memoryTypeIndex = type_index;
	auto free_memory = reinterpret_cast<PFN_vkFreeMemory>(dproc("vkFreeMemory"));
	auto destroy_buffer = reinterpret_cast<PFN_vkDestroyBuffer>(dproc("vkDestroyBuffer"));
	(void)free_memory;
	(void)destroy_buffer;
	if (allocate_memory(device, &allocate_info, nullptr, &out.memory) != VK_SUCCESS ||
	    bind_buffer(device, out.buffer, out.memory, 0) != VK_SUCCESS ||
	    map_memory(device, out.memory, 0, VK_WHOLE_SIZE, 0, &out.mapped) != VK_SUCCESS ||
	    out.mapped == nullptr) {
		PrintLine1("buffer: %s allocate/bind/map failed (type %u)", label, type_index);
		Expect(false, label);
		return 1;
	}
	out.size = size;
	out.allocation_size = requirements.size;
	PrintLine1("buffer: %s size=%llu type=%u noncoherent=%d", label,
	           static_cast<unsigned long long>(size), type_index, out.noncoherent ? 1 : 0);
	return 0;
}

int CheckHostFixtures() {
	const auto scenarios = MakeScenarios();
	Expect(scenarios.size() == 9, "exactly nine runtime scenarios");
	Expect(!scenarios.front().dispatch &&
	       scenarios.front().expected.words == scenarios.front().initial.words &&
	       scenarios.front().expected.words[kStatusWord] == kStatusPending,
	       "no-dispatch scenario preserves every initialized word and Pending");
	size_t recoveries = 0;
	for (size_t i = 0; i < scenarios.size(); ++i) {
		const auto& scenario = scenarios[i];
		Expect(scenario.payload_positions.size() == kPositionWords &&
		       scenario.payload_indices.size() == kIndexWords, "complete bounded payload fixtures");
		if (!scenario.upload_output) {
			++recoveries;
			Expect(i > 0 && scenario.initial.words == scenarios[i - 1].expected.words,
			       "recovery starts from immediately preceding GPU scenario");
			Expect((scenario.initial.words[kStatusWord] == kStatusOverflowVertices ||
			        scenario.initial.words[kStatusWord] == kStatusOverflowIndices) &&
			       scenario.expected.words[kStatusWord] == kStatusOk,
			       "recovery changes a committed overflow error to Ok");
		}
	}
	Expect(recoveries == 2, "both overflow status values have a same-buffer recovery");
	VkPhysicalDeviceMemoryProperties properties = {};
	properties.memoryTypeCount = 3;
	properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	properties.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	properties.memoryTypes[2].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	bool noncoherent = false;
	Expect(PickMemoryType(properties, 0x2u, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &noncoherent) == 1 && noncoherent,
	       "excluded preferred memory type cannot hide a compatible noncoherent type");
	Expect(PickMemoryType(properties, 0x4u, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr) == UINT32_MAX,
	       "no compatible host-visible memory type is rejected");
	PrintLine1("RESULT: %s (host scenario and memory-selection regressions)", g_failures == 0 ? "PASS" : "FAIL");
	return g_failures == 0 ? 0 : 1;
}

#include "MeshOffscreenDraw.h"

// Core-and-synchronization-validated dispatch at the explicitly requested API level.
// A single registered skip code covers missing runtime prerequisites, never API failures.
int RunPrototype(uint32_t required_api, bool draw = false, const std::filesystem::path& image_path = {}, bool textured = false, const char* ir_variant = nullptr) {
	const bool ir_uv = ir_variant && (std::strcmp(ir_variant, "uv") == 0 || std::strcmp(ir_variant, "uv_zero") == 0);
	void* loader = LoadLoaderLibrary();
	if (loader == nullptr) {
		PrintLine1("SKIP: the vulkan-1 system loader cannot be loaded; the dispatch part cannot "
		           "run here (offline SPIR-V validation is a separate registered check)");
		return 41;
	}
	auto* get_instance_proc_addr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
	    GetLoaderProcAddress(loader, "vkGetInstanceProcAddr"));
	if (get_instance_proc_addr == nullptr) {
		Expect(false, "loaded Vulkan library exports vkGetInstanceProcAddr");
		UnloadLoaderLibrary(loader);
		return 1;
	}
	auto iproc = [&](const char* name) {
		return reinterpret_cast<void*>(get_instance_proc_addr(nullptr, name));
	};
	auto vCreateInstance2 = reinterpret_cast<PFN_vkCreateInstance>(iproc("vkCreateInstance"));
	auto enumerate_instance_layer_properties =
	    reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties>(
	        iproc("vkEnumerateInstanceLayerProperties"));
	auto enumerate_instance_extension_properties =
	    reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties>(
	        iproc("vkEnumerateInstanceExtensionProperties"));
	auto enumerate_instance_version = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
	    iproc("vkEnumerateInstanceVersion"));
	uint32_t loader_api = VK_API_VERSION_1_0;
	if (enumerate_instance_version != nullptr && enumerate_instance_version(&loader_api) != VK_SUCCESS) {
		Expect(false, "loader API version query succeeds");
		UnloadLoaderLibrary(loader);
		return 1;
	}
	PrintLine1("runtime requested=%s loader=%s", ApiString(required_api).c_str(), ApiString(loader_api).c_str());
	if (loader_api < required_api) {
		PrintLine1("SKIP: loader does not support the requested Vulkan API");
		UnloadLoaderLibrary(loader);
		return kSkip;
	}
	if (vCreateInstance2 == nullptr || enumerate_instance_layer_properties == nullptr ||
	    enumerate_instance_extension_properties == nullptr) {
		Expect(false, "required global Vulkan entry points resolve");
		UnloadLoaderLibrary(loader);
		return 1;
	}
	PrintLine1("implicit-layer environment: VK_IMPLICIT_LAYER_PATH=%s VK_ADD_IMPLICIT_LAYER_PATH=%s",
	           std::getenv("VK_IMPLICIT_LAYER_PATH") ? std::getenv("VK_IMPLICIT_LAYER_PATH") : "<unset>",
	           std::getenv("VK_ADD_IMPLICIT_LAYER_PATH") ? std::getenv("VK_ADD_IMPLICIT_LAYER_PATH") : "<unset>");
	const char* env_layer_path = std::getenv("VK_LAYER_PATH");
	const char* env_layer_env = std::getenv("VK_ADD_LAYER_PATH");
	const char* env_layers = std::getenv("VK_INSTANCE_LAYERS");
	const char* env_driver_files = std::getenv("VK_DRIVER_FILES");
	const char* env_icd_names = std::getenv("VK_ICD_FILENAMES");
	PrintLine1(
	    "loader environment: VK_LAYER_PATH=%s VK_ADD_LAYER_PATH=%s VK_INSTANCE_LAYERS=%s "
	    "VK_DRIVER_FILES=%s VK_ICD_FILENAMES=%s",
	    env_layer_path != nullptr ? env_layer_path : "<unset>",
	    env_layer_env != nullptr ? env_layer_env : "<unset>",
	    env_layers != nullptr ? env_layers : "<unset>",
	    env_driver_files != nullptr ? env_driver_files : "<unset>",
	    env_icd_names != nullptr ? env_icd_names : "<unset>");

	std::vector<VkLayerProperties> layers;
	uint32_t layer_count = 0;
	if (!CheckVk(enumerate_instance_layer_properties(&layer_count, nullptr), "instance layer count")) {
		UnloadLoaderLibrary(loader);
		return 1;
	}
	if (layer_count > 0) {
		layers.resize(layer_count);
		if (!CheckVk(enumerate_instance_layer_properties(&layer_count, layers.data()), "instance layer list")) {
			UnloadLoaderLibrary(loader);
			return 1;
		}
		layers.resize(layer_count);
	}
	for (const VkLayerProperties& layer: layers) {
		PrintLine1("instance-layer: %s v%u (%s)", layer.layerName, layer.implementationVersion,
		           layer.description);
	}
	const bool have_khronos_validation = std::any_of(
	    layers.begin(), layers.end(), [](const VkLayerProperties& layer) {
		    return std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0;
		    });
	if (!have_khronos_validation) {
		PrintLine1("SKIP: VK_LAYER_KHRONOS_validation is not installed; the core + "
		           "synchronization validation gate cannot be met");
		UnloadLoaderLibrary(loader);
		return 41;
	}

	std::vector<VkExtensionProperties> instance_extensions;
	uint32_t instance_extension_count = 0;
	if (!CheckVk(enumerate_instance_extension_properties(nullptr, &instance_extension_count, nullptr),
	             "global instance extension count")) {
		UnloadLoaderLibrary(loader);
		return 1;
	}
	if (instance_extension_count > 0) {
		instance_extensions.resize(instance_extension_count);
		if (!CheckVk(enumerate_instance_extension_properties(nullptr, &instance_extension_count,
		                                                   instance_extensions.data()), "global instance extension list")) {
			UnloadLoaderLibrary(loader);
			return 1;
		}
		instance_extensions.resize(instance_extension_count);
	}
	// A NULL layerName only lists loader/implicit-layer extensions; the validation
	// layer brokers VK_EXT_validation_features, so its own extension list is also
	// queried (explicit layers need the layerName argument) and merged without
	// duplicates.
	{
		const char* khronos_layer_name = "VK_LAYER_KHRONOS_validation";
		uint32_t layer_extension_count = 0;
		if (!CheckVk(enumerate_instance_extension_properties(khronos_layer_name, &layer_extension_count,
		                                                    nullptr), "validation layer extension count")) {
			UnloadLoaderLibrary(loader);
			return 1;
		}
		if (layer_extension_count > 0) {
			std::vector<VkExtensionProperties> layer_extensions(layer_extension_count);
			if (!CheckVk(enumerate_instance_extension_properties(khronos_layer_name, &layer_extension_count,
			                                                   layer_extensions.data()), "validation layer extension list")) {
				UnloadLoaderLibrary(loader);
				return 1;
			}
			layer_extensions.resize(layer_extension_count);
			for (const VkExtensionProperties& extension: layer_extensions) {
				const bool already = std::any_of(
				    instance_extensions.begin(), instance_extensions.end(),
				    [&](const VkExtensionProperties& existing) {
					    return std::strcmp(existing.extensionName, extension.extensionName) == 0;
				    });
				if (!already) {
					instance_extensions.push_back(extension);
				}
			}
		}
	}
	const bool have_debug_utils = std::any_of(
	    instance_extensions.begin(), instance_extensions.end(),
	    [](const VkExtensionProperties& extension) {
		    return std::strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
		    });
	const bool have_validation_features = std::any_of(
	    instance_extensions.begin(), instance_extensions.end(),
	    [](const VkExtensionProperties& extension) {
		    return std::strcmp(extension.extensionName, "VK_EXT_validation_features") == 0;
		    });
	PrintLine1("instance-extensions: %zu available (debug_utils=%d validation_features=%d)",
	           instance_extensions.size(), have_debug_utils ? 1 : 0,
	           have_validation_features ? 1 : 0);
	if (!have_debug_utils || !have_validation_features) {
		PrintLine1("SKIP: core/synchronization validation and a debug error collector are required");
		UnloadLoaderLibrary(loader);
		return kSkip;
	}

	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	VkValidationFeaturesEXT validation_features = {};
	VkValidationFeatureEnableEXT sync_validation[1] = {
	    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};
	VkDebugUtilsMessengerCreateInfoEXT messenger_info = {};
	messenger_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messenger_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messenger_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	                             VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messenger_info.pfnUserCallback = OnValidationMessage;
	{
		VkApplicationInfo application_info = {};
		application_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		application_info.pApplicationName = "Kyty-RDNA1-MeshComputeProto";
		application_info.apiVersion = required_api;
		VkInstanceCreateInfo instance_info = {};
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pApplicationInfo = &application_info;
		const char* khronos_layer = "VK_LAYER_KHRONOS_validation";
		instance_info.enabledLayerCount = 1;
		instance_info.ppEnabledLayerNames = &khronos_layer;
		std::vector<const char*> enabled_extensions;
		if (have_debug_utils) {
			enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		if (have_validation_features) {
			enabled_extensions.push_back("VK_EXT_validation_features");
		}
		instance_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
		instance_info.ppEnabledExtensionNames = enabled_extensions.data();
		if (have_validation_features) {
			validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
			validation_features.enabledValidationFeatureCount = 1;
			validation_features.pEnabledValidationFeatures = sync_validation;
			validation_features.pNext = &messenger_info; // capture instance creation/destruction too
			instance_info.pNext = &validation_features;
			PrintLine1(
			    "validation: synchronous-validation features enabled on the validation layer");
		} else {
			PrintLine1("validation: core validation only; VK_EXT_validation_features absent");
		}
		if (vCreateInstance2(&instance_info, nullptr, &instance) != VK_SUCCESS) {
			Expect(false, "instance creation succeeds");
			UnloadLoaderLibrary(loader);
			return 1;
		}
		auto create_messenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
		    reinterpret_cast<void*>(get_instance_proc_addr(
		        instance, "vkCreateDebugUtilsMessengerEXT")));
		if (have_debug_utils && create_messenger != nullptr &&
		    create_messenger(instance, &messenger_info, nullptr, &debug_messenger) == VK_SUCCESS) {
			PrintLine1("validation: debug-utils messenger installed");
		} else {
			Expect(false, "required debug-utils messenger creation succeeds");
			auto destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
			    get_instance_proc_addr(instance, "vkDestroyInstance"));
			if (destroy_instance != nullptr) { destroy_instance(instance, nullptr); }
			UnloadLoaderLibrary(loader);
			return 1;
		}
	}

	// Physical-device enumeration and selection: every device the loader exposes is
	// printed with its full identity (name, type, vendor/device, API and driver
	// version, conformance version, driver name), and the first device with a
	// non-empty compute-capable queue family is selected deterministically. Any
	// result on this runner machine is Intel, software-ICD or other vendor
	// evidence only; it cannot establish an RDNA1 hardware gate.
	VkPhysicalDevice selected                      = VK_NULL_HANDLE;
	uint32_t selected_index                        = 0;
	uint32_t selected_family                       = UINT32_MAX;
	VkPhysicalDeviceProperties selected_properties = {};
	VkPhysicalDeviceDriverProperties selected_driver = {};
	VkPhysicalDeviceMemoryProperties selected_memory = {};
	auto vDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(
	    reinterpret_cast<void*>(get_instance_proc_addr(instance, "vkDestroyInstance")));
	auto vEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(
	    reinterpret_cast<void*>(get_instance_proc_addr(instance, "vkEnumeratePhysicalDevices")));
	auto vGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(
	    reinterpret_cast<void*>(get_instance_proc_addr(instance, "vkGetPhysicalDeviceProperties")));
	auto vGetPhysicalDeviceQueueFamilyProperties =
	    reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(
	        reinterpret_cast<void*>(get_instance_proc_addr(
	            instance, "vkGetPhysicalDeviceQueueFamilyProperties")));
	auto vGetPhysicalDeviceMemoryProperties =
	    reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(
	        reinterpret_cast<void*>(get_instance_proc_addr(
	            instance, "vkGetPhysicalDeviceMemoryProperties")));
	auto vGetPhysicalDeviceProperties2 =
	    reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(
	        reinterpret_cast<void*>(get_instance_proc_addr(
	            instance, "vkGetPhysicalDeviceProperties2")));
	// Every teardown until the logical device exists goes through this closure: it
	// releases the debug messenger FIRST (with it installed, destroying the
	// instance directly is a validated spec violation), then the instance, then the
	// loader library. cleanup_all (further down) handles the post-device teardown.
	auto close_instance = [&]() {
		auto vDestroyDebugUtilsMessengerEXT3 =
		    reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
		        reinterpret_cast<void*>(get_instance_proc_addr(
		            instance, "vkDestroyDebugUtilsMessengerEXT")));
		if (debug_messenger != VK_NULL_HANDLE && vDestroyDebugUtilsMessengerEXT3 != nullptr) {
			vDestroyDebugUtilsMessengerEXT3(instance, debug_messenger, nullptr);
			debug_messenger = VK_NULL_HANDLE;
		}
		if (instance != VK_NULL_HANDLE && vDestroyInstance != nullptr) {
			vDestroyInstance(instance, nullptr);
		}
		UnloadLoaderLibrary(loader);
	};
	if (vDestroyInstance == nullptr || vEnumeratePhysicalDevices == nullptr ||
	    vGetPhysicalDeviceProperties == nullptr ||
	    vGetPhysicalDeviceQueueFamilyProperties == nullptr ||
	    vGetPhysicalDeviceMemoryProperties == nullptr) {
		Expect(false, "required physical-device entry points resolve");
		close_instance();
		return 1;
	}
	uint32_t device_count = 0;
	if (vEnumeratePhysicalDevices(instance, &device_count, nullptr) != VK_SUCCESS) {
		Expect(false, "physical-device count query succeeds");
		close_instance();
		return 1;
	}
	if (device_count == 0) {
		PrintLine1("SKIP: no physical devices were found (device_count=%u)", device_count);
		close_instance();
		return kSkip;
	}
	std::vector<VkPhysicalDevice> devices(device_count);
	if (vEnumeratePhysicalDevices(instance, &device_count, devices.data()) != VK_SUCCESS) {
		Expect(false, "physical device enumeration succeeds");
		close_instance();
		return 1;
	}
	PrintLine1("physical-devices: %u enumerated", device_count);
	for (size_t i = 0; i < devices.size(); ++i) {
		VkPhysicalDeviceProperties device_properties = {};
		vGetPhysicalDeviceProperties(devices[i], &device_properties);
		uint32_t family_count = 0;
		vGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, nullptr);
		std::vector<VkQueueFamilyProperties> families(family_count);
		if (family_count > 0) {
			vGetPhysicalDeviceQueueFamilyProperties(devices[i], &family_count, families.data());
		}
		uint32_t compute_family = UINT32_MAX;
		for (uint32_t j = 0; j < family_count; ++j) {
			if ((families[j].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && families[j].queueCount > 0 &&
			    (!draw || (families[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)) {
				compute_family = j;
				break;
			}
		}
		VkPhysicalDeviceDriverProperties driver_chip = {};
		driver_chip.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
		std::string driver_label;
		if (vGetPhysicalDeviceProperties2 != nullptr && device_properties.apiVersion >= VK_API_VERSION_1_2) {
			VkPhysicalDeviceProperties2 properties2 = {};
			properties2.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			properties2.properties = device_properties;
			properties2.pNext      = &driver_chip;
			vGetPhysicalDeviceProperties2(devices[i], &properties2);
			if (driver_chip.driverID != 0) {
				driver_label =
				    std::string(driver_chip.driverName) + " (ID " +
				    std::to_string(driver_chip.driverID) + ", conform=" +
				    std::to_string(driver_chip.conformanceVersion.major) + "." +
				    std::to_string(driver_chip.conformanceVersion.minor) + "." +
				    std::to_string(driver_chip.conformanceVersion.patch) + ", " +
				    driver_chip.driverInfo + ")";
			}
		}
		if (driver_label.empty()) {
			driver_label = "<driver name unavailable>";
		}
		PrintLine1(
		    "[%zu] %s type=%s vendor=0x%04X device=0x%04X api=%s driverVer=0x%08X "
		    "driver=%s compute-queue=%s",
		    i, device_properties.deviceName, DeviceTypeString(device_properties.deviceType),
		    device_properties.vendorID, device_properties.deviceID,
		    ApiString(device_properties.apiVersion).c_str(), device_properties.driverVersion,
		    driver_label.c_str(),
		    compute_family != UINT32_MAX ? "yes" : "no");
		if (selected == VK_NULL_HANDLE && compute_family != UINT32_MAX &&
		    device_properties.apiVersion >= required_api) {
			selected            = devices[i];
			selected_index      = static_cast<uint32_t>(i);
			selected_family     = compute_family;
			selected_properties = device_properties;
			selected_driver     = driver_chip;
			vGetPhysicalDeviceMemoryProperties(selected, &selected_memory);
		}
	}
	if (selected == VK_NULL_HANDLE) {
		PrintLine1("SKIP: no compute device supports requested Vulkan %s", ApiString(required_api).c_str());
		close_instance();
		return kSkip;
	}
	PrintLine1("selected: #%u %s compute-queue-family=%u memory-heap-count=%u",
	           selected_index, selected_properties.deviceName, selected_family,
	           selected_memory.memoryHeapCount);
	if (selected_driver.driverID == 0) {
		PrintLine1("selected-driver: <unavailable>");
	} else {
		PrintLine1("selected-driver: %s", selected_driver.driverName);
	}
	if (draw) {
		auto format_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(
		    get_instance_proc_addr(instance, "vkGetPhysicalDeviceFormatProperties"));
		if (format_properties == nullptr) { Expect(false, "format query entry point"); close_instance(); return 1; }
		VkFormatProperties format = {};
		format_properties(selected, VK_FORMAT_R8G8B8A8_UNORM, &format);
		const auto needed = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
		                    (textured ? VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT : 0);
		if ((format.optimalTilingFeatures & needed) != needed) {
			PrintLine1("SKIP: offscreen RGBA8 color/transfer format unavailable"); close_instance(); return kSkip;
		}
		format_properties(selected, VK_FORMAT_R32G32B32A32_SFLOAT, &format);
		if ((format.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) == 0) {
			PrintLine1("SKIP: float4 vertex format unavailable"); close_instance(); return kSkip;
		}
		if (textured) {
			format_properties(selected, VK_FORMAT_R32G32_SFLOAT, &format);
			if ((format.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) == 0) {
				PrintLine1("SKIP: float2 UV vertex format unavailable"); close_instance(); return kSkip;
			}
		}
		PrintLine1("draw-mode: conventional indexed offscreen triangle, graphics+compute queue, API %s", ApiString(required_api).c_str());
	}
	// Never downgrade the requested experiment. The auxiliary 1.2 run is an
	// explicit CLI/CTest case, even on a device that also supports Vulkan 1.3.
	const bool  device_13_capable = required_api >= VK_API_VERSION_1_3;
	const char* module_label      = device_13_capable ? "vulkan1.3" : "vulkan1.2";
	const auto module_path = ir_variant ? g_shader_directory / (std::string("mesh_ir_") + ir_variant + (device_13_capable ? "_vk13.spv" : "_vk12.spv")) : ModulePath(device_13_capable);
	PrintLine1("dispatch-module: %s (%s)", module_label,
	           device_13_capable ? "strict Vulkan 1.3 runtime gate"
	                             : "explicit auxiliary Vulkan 1.2 run; not Vulkan 1.3 evidence");
	const std::vector<uint32_t> module_words = LoadSpirvFile(module_path, ir_variant ? ir_variant : module_label);
	if (ir_variant && !ValidateSpirv(module_words, device_13_capable ? SPV_ENV_VULKAN_1_3 : SPV_ENV_VULKAN_1_2, "IR-generated module")) { close_instance(); return 1; }
	if (module_words.empty()) {
		close_instance();
		return 1;
	}
	// Logical device with exactly one compute queue, and the shader module built
	// from the validated SPIR-V words read back from the record's artifacts.
	auto vCreateDevice =
	    reinterpret_cast<PFN_vkCreateDevice>(get_instance_proc_addr(instance, "vkCreateDevice"));
	auto get_device_proc = reinterpret_cast<PFN_vkGetDeviceProcAddr>(
	    get_instance_proc_addr(instance, "vkGetDeviceProcAddr"));
	if (vCreateDevice == nullptr) {
		Expect(false, "required vkCreateDevice lookup succeeds for the live instance");
		close_instance();
		return 1;
	}
	if (get_device_proc == nullptr) {
		Expect(false, "required vkGetDeviceProcAddr lookup succeeds");
		close_instance();
		return 1;
	}
	VkDeviceQueueCreateInfo queue_info = {};
	queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.queueFamilyIndex = selected_family;
	queue_info.queueCount       = 1;
	const float queue_priority  = 1.0f;
	queue_info.pQueuePriorities = &queue_priority;
	VkDeviceCreateInfo device_info  = {};
	device_info.sType               = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos    = &queue_info;
	VkDevice device = VK_NULL_HANDLE;
	if (vCreateDevice(selected, &device_info, nullptr, &device) != VK_SUCCESS) {
		Expect(false, "logical device creation succeeds");
		close_instance();
		return 1;
	}
	auto dp = [&](const char* name) {
		return reinterpret_cast<void*>(get_device_proc(device, name));
	};
	VkShaderModuleCreateInfo module_info = {};
	module_info.sType   = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = module_words.size() * sizeof(uint32_t);
	module_info.pCode    = module_words.data();
	VkShaderModule shader_module = VK_NULL_HANDLE;
	auto vCreateShaderModule =
	    reinterpret_cast<PFN_vkCreateShaderModule>(dp("vkCreateShaderModule"));
	auto vDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(dp("vkDestroyDevice"));
	if (vCreateShaderModule == nullptr ||
	    vCreateShaderModule(device, &module_info, nullptr, &shader_module) != VK_SUCCESS) {
		Expect(false, "shader module creation succeeds");
		if (vDestroyDevice != nullptr) {
			vDestroyDevice(device, nullptr);
		}
		close_instance();
		return 1;
	}

	// Pipeline and descriptors. Binding 0 is the read-only payload (256 position
	// words + 192 index words in one buffer), binding 1 is the write-only output
	// blob (460 words); generated UV mode adds binding 2 (136 words).
	// The push-constant block carries vertex_count /
	// primitive_count at offset 0. One descriptor set is allocated and updated
	// once; the per-scenario variation rides in buffer contents and push
	// constants only.
	VkDescriptorSetLayout set_layout     = VK_NULL_HANDLE;
	VkPipelineLayout pipeline_layout     = VK_NULL_HANDLE;
	VkPipeline pipeline                  = VK_NULL_HANDLE;
	VkDescriptorPool descriptor_pool     = VK_NULL_HANDLE;
	VkDescriptorSet descriptor_set       = VK_NULL_HANDLE;
	VkCommandPool command_pool           = VK_NULL_HANDLE;
	VkCommandBuffer command_buffer       = VK_NULL_HANDLE;
	VkFence submit_fence                 = VK_NULL_HANDLE;
	VkQueue queue                        = VK_NULL_HANDLE;
	GpuBuffer payload_buffer;
	GpuBuffer output_buffer;
	GpuBuffer uv_buffer;
	auto release_buffer = [&](GpuBuffer& buffer) {
		if (buffer.buffer == VK_NULL_HANDLE && buffer.memory == VK_NULL_HANDLE) {
			return;
		}
		auto vUnmapMemoryD = reinterpret_cast<PFN_vkUnmapMemory>(dp("vkUnmapMemory"));
		auto vFreeMemoryD  = reinterpret_cast<PFN_vkFreeMemory>(dp("vkFreeMemory"));
		auto vDestroyBufferD =
		    reinterpret_cast<PFN_vkDestroyBuffer>(dp("vkDestroyBuffer"));
		if (buffer.mapped != nullptr && vUnmapMemoryD != nullptr) {
			vUnmapMemoryD(device, buffer.memory);
		}
		if (buffer.buffer != VK_NULL_HANDLE && vDestroyBufferD != nullptr) {
			vDestroyBufferD(device, buffer.buffer, nullptr);
		}
		if (buffer.memory != VK_NULL_HANDLE && vFreeMemoryD != nullptr) {
			vFreeMemoryD(device, buffer.memory, nullptr);
		}
		buffer = GpuBuffer{};
	};
	auto cleanup_all = [&]() {
		// A timeout/failure must not destroy resources while a submission is pending.
		auto wait_idle = reinterpret_cast<PFN_vkDeviceWaitIdle>(dp("vkDeviceWaitIdle"));
		Expect(wait_idle != nullptr, "vkDeviceWaitIdle resolves for cleanup");
		if (wait_idle != nullptr) {
			Expect(wait_idle(device) == VK_SUCCESS, "device idle before resource cleanup");
		}
		// Destruction order is bottom-up: set, pool, pipeline, layouts, the
		// commands/fence, then the buffers, then the device itself. Every handle
		// is null-guarded so partially created state cleans up.
		if (descriptor_set != VK_NULL_HANDLE) {
			(void)descriptor_set; // freed together with the descriptor pool
		}
		auto vDestroyDescriptorPoolD =
		    reinterpret_cast<PFN_vkDestroyDescriptorPool>(dp("vkDestroyDescriptorPool"));
		if (descriptor_pool != VK_NULL_HANDLE && vDestroyDescriptorPoolD != nullptr) {
			vDestroyDescriptorPoolD(device, descriptor_pool, nullptr);
		}
		auto vDestroyPipelineD =
		    reinterpret_cast<PFN_vkDestroyPipeline>(dp("vkDestroyPipeline"));
		if (pipeline != VK_NULL_HANDLE && vDestroyPipelineD != nullptr) {
			vDestroyPipelineD(device, pipeline, nullptr);
		}
		auto destroy_shader = reinterpret_cast<PFN_vkDestroyShaderModule>(dp("vkDestroyShaderModule"));
		if (shader_module != VK_NULL_HANDLE && destroy_shader != nullptr) {
			destroy_shader(device, shader_module, nullptr);
		}
		auto vDestroyPipelineLayoutD =
		    reinterpret_cast<PFN_vkDestroyPipelineLayout>(dp("vkDestroyPipelineLayout"));
		if (pipeline_layout != VK_NULL_HANDLE && vDestroyPipelineLayoutD != nullptr) {
			vDestroyPipelineLayoutD(device, pipeline_layout, nullptr);
		}
		auto vDestroyDescriptorSetLayoutD =
		    reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(
		        dp("vkDestroyDescriptorSetLayout"));
		if (set_layout != VK_NULL_HANDLE && vDestroyDescriptorSetLayoutD != nullptr) {
			vDestroyDescriptorSetLayoutD(device, set_layout, nullptr);
		}
		auto vDestroyFenceD = reinterpret_cast<PFN_vkDestroyFence>(dp("vkDestroyFence"));
		if (submit_fence != VK_NULL_HANDLE && vDestroyFenceD != nullptr) {
			vDestroyFenceD(device, submit_fence, nullptr);
		}
		auto vDestroyCommandPoolD =
		    reinterpret_cast<PFN_vkDestroyCommandPool>(dp("vkDestroyCommandPool"));
		if (command_pool != VK_NULL_HANDLE && vDestroyCommandPoolD != nullptr) {
			vDestroyCommandPoolD(device, command_pool, nullptr);
		}
		release_buffer(uv_buffer);
		release_buffer(output_buffer);
		release_buffer(payload_buffer);
		auto vDestroyDeviceD = reinterpret_cast<PFN_vkDestroyDevice>(dp("vkDestroyDevice"));
		if (vDestroyDeviceD != nullptr) {
			vDestroyDeviceD(device, nullptr);
		}
		close_instance();
	};
	auto fail_and_cleanup = [&]() {
		cleanup_all();
		return 1;
	};
	auto vGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(dp("vkGetDeviceQueue"));
	if (vGetDeviceQueue == nullptr) {
		Expect(false, "vkGetDeviceQueue is exported for the logical device");
		return fail_and_cleanup();
	}
	vGetDeviceQueue(device, selected_family, 0, &queue);
	if (queue == VK_NULL_HANDLE) {
		Expect(false, "the compute queue is retrieved");
		return fail_and_cleanup();
	}

	// Buffers first: both buffers are host-visible mapped; the output prefers
	// device-local memory, and the mandatory flush/invalidate steps happen whenever
	// the chosen mapped type turns out non-coherent; the payload prefers plainly
	// coherent mapping. The chosen types are printed for the experiment record.
	if (AllocateMappedBuffer(device, get_device_proc, selected_memory,
	                         static_cast<VkDeviceSize>(kTotalBytes),
	                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | (draw ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0),
	                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                         output_buffer, "output-blob") != 0) {
		return fail_and_cleanup();
	}
	if (AllocateMappedBuffer(device, get_device_proc, selected_memory,
	                         static_cast<VkDeviceSize>(kPositionWords + kIndexWords) * 4u,
	                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
	                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                         payload_buffer, "payload-buffer") != 0) {
		return fail_and_cleanup();
	}
	Expect(output_buffer.size >= static_cast<VkDeviceSize>(kTotalBytes),
	       "output buffer covers the full 460-word blob");
	Expect(payload_buffer.size >= static_cast<VkDeviceSize>(kPositionWords + kIndexWords) * 4u,
	       "payload buffer covers the full 448-word payload");

	if (ir_uv && AllocateMappedBuffer(device, get_device_proc, selected_memory, kUvBytes,
	        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
	        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	        uv_buffer, "IR-uv-output") != 0) { return fail_and_cleanup(); }
	VkDescriptorSetLayoutBinding bindings[3] = {};
	bindings[0].binding         = 0;
	bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[1]                 = bindings[0];
	bindings[1].binding         = 1;
	bindings[2] = bindings[0]; bindings[2].binding = 2;
	VkDescriptorSetLayoutCreateInfo set_layout_info = {};
	set_layout_info.sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set_layout_info.bindingCount = ir_uv ? 3 : 2;
	set_layout_info.pBindings    = bindings;
	auto vCreateDescriptorSetLayoutD = reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(
	    dp("vkCreateDescriptorSetLayout"));
	if (vCreateDescriptorSetLayoutD == nullptr ||
	    vCreateDescriptorSetLayoutD(device, &set_layout_info, nullptr, &set_layout) !=
	        VK_SUCCESS) {
		Expect(false, "descriptor set layout creation succeeds");
		return fail_and_cleanup();
	}
	VkPushConstantRange push_range = {};
	push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	push_range.offset     = 0;
	push_range.size       = sizeof(uint32_t) * 2;
	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount    = 1;
	layout_info.pSetLayouts       = &set_layout;
	layout_info.pushConstantRangeCount = 1;
	layout_info.pPushConstantRanges    = &push_range;
	auto vCreatePipelineLayoutD =
	    reinterpret_cast<PFN_vkCreatePipelineLayout>(dp("vkCreatePipelineLayout"));
	if (vCreatePipelineLayoutD == nullptr ||
	    vCreatePipelineLayoutD(device, &layout_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
		Expect(false, "pipeline layout creation succeeds");
		return fail_and_cleanup();
	}
	VkPipelineShaderStageCreateInfo stage_info = {};
	stage_info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage_info.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
	stage_info.module = shader_module;
	stage_info.pName  = "main";
	VkComputePipelineCreateInfo pipeline_info = {};
	pipeline_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.stage  = stage_info;
	pipeline_info.layout = pipeline_layout;
	auto vCreateComputePipelinesD =
	    reinterpret_cast<PFN_vkCreateComputePipelines>(dp("vkCreateComputePipelines"));
	if (vCreateComputePipelinesD == nullptr ||
	    vCreateComputePipelinesD(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr,
	                             &pipeline) != VK_SUCCESS) {
		Expect(false, "compute pipeline creation succeeds");
		return fail_and_cleanup();
	}
	VkDescriptorPoolSize pool_size  = {};
	pool_size.type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = ir_uv ? 3 : 2;
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets       = 1;
	pool_info.poolSizeCount = 1;
	pool_info.pPoolSizes    = &pool_size;
	auto vCreateDescriptorPoolD =
	    reinterpret_cast<PFN_vkCreateDescriptorPool>(dp("vkCreateDescriptorPool"));
	if (vCreateDescriptorPoolD == nullptr ||
	    vCreateDescriptorPoolD(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
		Expect(false, "descriptor pool creation succeeds");
		return fail_and_cleanup();
	}
	VkDescriptorSetAllocateInfo set_allocate_info = {};
	set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	set_allocate_info.descriptorPool = descriptor_pool;
	set_allocate_info.descriptorSetCount = 1;
	set_allocate_info.pSetLayouts        = &set_layout;
	auto vAllocateDescriptorSetsD =
	    reinterpret_cast<PFN_vkAllocateDescriptorSets>(dp("vkAllocateDescriptorSets"));
	if (vAllocateDescriptorSetsD == nullptr ||
	    vAllocateDescriptorSetsD(device, &set_allocate_info, &descriptor_set) != VK_SUCCESS) {
		Expect(false, "descriptor set allocation succeeds");
		return fail_and_cleanup();
	}
	VkDescriptorBufferInfo payload_range = {};
	payload_range.buffer = payload_buffer.buffer;
	payload_range.offset = 0;
	payload_range.range  = static_cast<VkDeviceSize>(kPositionWords + kIndexWords) * 4u;
	VkDescriptorBufferInfo output_range = {};
	output_range.buffer = output_buffer.buffer;
	output_range.offset = 0;
	output_range.range  = static_cast<VkDeviceSize>(kTotalBytes);
	VkWriteDescriptorSet writes[3]      = {};
	writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].dstSet          = descriptor_set;
	writes[0].dstBinding      = 0;
	writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writes[0].descriptorCount = 1;
	writes[0].pBufferInfo     = &payload_range;
	writes[1]                 = writes[0];
	writes[1].dstBinding      = 1;
	writes[1].pBufferInfo     = &output_range;
	VkDescriptorBufferInfo uv_range = {uv_buffer.buffer, 0, kUvBytes};
	writes[2] = writes[0]; writes[2].dstBinding = 2; writes[2].pBufferInfo = &uv_range;
	auto vUpdateDescriptorSetsD =
	    reinterpret_cast<PFN_vkUpdateDescriptorSets>(dp("vkUpdateDescriptorSets"));
	if (vUpdateDescriptorSetsD == nullptr) {
		Expect(false, "vkUpdateDescriptorSets is exported for the logical device");
		return fail_and_cleanup();
	}
	vUpdateDescriptorSetsD(device, ir_uv ? 3 : 2, writes, 0, nullptr);
	VkCommandPoolCreateInfo command_pool_info = {};
	command_pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_info.queueFamilyIndex = selected_family;
	auto vCreateCommandPoolD =
	    reinterpret_cast<PFN_vkCreateCommandPool>(dp("vkCreateCommandPool"));
	if (vCreateCommandPoolD == nullptr ||
	    vCreateCommandPoolD(device, &command_pool_info, nullptr, &command_pool) != VK_SUCCESS) {
		Expect(false, "command pool creation succeeds");
		return fail_and_cleanup();
	}
	VkCommandBufferAllocateInfo command_allocate_info = {};
	command_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_allocate_info.commandPool = command_pool;
	command_allocate_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_allocate_info.commandBufferCount = 1;
	auto vAllocateCommandBuffersD =
	    reinterpret_cast<PFN_vkAllocateCommandBuffers>(dp("vkAllocateCommandBuffers"));
	if (vAllocateCommandBuffersD == nullptr ||
	    vAllocateCommandBuffersD(device, &command_allocate_info, &command_buffer) != VK_SUCCESS) {
		Expect(false, "command buffer allocation succeeds");
		return fail_and_cleanup();
	}
	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	auto vCreateFenceD = reinterpret_cast<PFN_vkCreateFence>(dp("vkCreateFence"));
	if (vCreateFenceD == nullptr ||
	    vCreateFenceD(device, &fence_info, nullptr, &submit_fence) != VK_SUCCESS) {
		Expect(false, "fence creation succeeds");
		return fail_and_cleanup();
	}
	auto vResetCommandBufferD =
	    reinterpret_cast<PFN_vkResetCommandBuffer>(dp("vkResetCommandBuffer"));
	auto vBeginCommandBufferD =
	    reinterpret_cast<PFN_vkBeginCommandBuffer>(dp("vkBeginCommandBuffer"));
	auto vEndCommandBufferD =
	    reinterpret_cast<PFN_vkEndCommandBuffer>(dp("vkEndCommandBuffer"));
	auto vCmdPipelineBarrierD =
	    reinterpret_cast<PFN_vkCmdPipelineBarrier>(dp("vkCmdPipelineBarrier"));
	auto vCmdBindPipelineD = reinterpret_cast<PFN_vkCmdBindPipeline>(dp("vkCmdBindPipeline"));
	auto vCmdBindDescriptorSetsD =
	    reinterpret_cast<PFN_vkCmdBindDescriptorSets>(dp("vkCmdBindDescriptorSets"));
	auto vCmdPushConstantsD =
	    reinterpret_cast<PFN_vkCmdPushConstants>(dp("vkCmdPushConstants"));
	auto vCmdDispatchD  = reinterpret_cast<PFN_vkCmdDispatch>(dp("vkCmdDispatch"));
	auto vQueueSubmitD  = reinterpret_cast<PFN_vkQueueSubmit>(dp("vkQueueSubmit"));
	auto vWaitForFencesD = reinterpret_cast<PFN_vkWaitForFences>(dp("vkWaitForFences"));
	auto vResetFencesD   = reinterpret_cast<PFN_vkResetFences>(dp("vkResetFences"));
	auto vFlushMappedMemoryRangesD =
	    reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(dp("vkFlushMappedMemoryRanges"));
	auto vInvalidateMappedMemoryRangesD =
	    reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(dp("vkInvalidateMappedMemoryRanges"));
	if (vCmdPipelineBarrierD == nullptr || vResetCommandBufferD == nullptr || vBeginCommandBufferD == nullptr ||
	    vEndCommandBufferD == nullptr || vCmdBindPipelineD == nullptr ||
	    vCmdBindDescriptorSetsD == nullptr || vCmdPushConstantsD == nullptr ||
	    vCmdDispatchD == nullptr || vQueueSubmitD == nullptr ||
	    vWaitForFencesD == nullptr || vResetFencesD == nullptr) {
		Expect(false, "every command path is exported for the logical device");
		return fail_and_cleanup();
	}
	if ((output_buffer.noncoherent || payload_buffer.noncoherent) && vFlushMappedMemoryRangesD == nullptr) {
		Expect(false, "vkFlushMappedMemoryRanges is exported for non-coherent output");
		return fail_and_cleanup();
	}
	if (output_buffer.noncoherent && vInvalidateMappedMemoryRangesD == nullptr) {
		Expect(false, "vkInvalidateMappedMemoryRanges is exported for non-coherent output");
		return fail_and_cleanup();
	}
	PrintLine1("memory: output-blob noncoherent=%d payload noncoherent=%d (host-visible "
	           "mapped buffers; explicit flush/invalidate only where required)",
	           output_buffer.noncoherent ? 1 : 0, payload_buffer.noncoherent ? 1 : 0);

	// The nine recorded scenarios run sequentially on one command buffer, one
	// submission and one fence each: initial upload (+mandatory flush), a 1x1x1
	// dispatch (or an empty submission for the pending-init case), a fenced wait,
	// the mandatory invalidate for non-coherent readback, and the exact 460-word
	// comparison against the host reference blob from the shared contract header.
	constexpr uint64_t kFenceTimeoutNs = 10ull * 1000ull * 1000ull * 1000ull;
	std::vector<Scenario> scenarios;
	if (ir_variant) {
		Scenario one; one.name = std::string("ir-") + ir_variant; one.initial.Init();
		one.vertex_count = (std::strcmp(ir_variant,"zero") == 0 || std::strcmp(ir_variant,"uv_zero") == 0 || std::strcmp(ir_variant,"frontend_zero") == 0) ? 0 : 3;
		one.primitive_count = one.vertex_count ? 1 : 0;
		const float pos[12] = {-.5f,-.5f,0,1,.5f,-.5f,0,1,-.5f,.484375f,0,1};
		one.payload_positions.assign(kPositionWords,0); std::memcpy(one.payload_positions.data(),pos,sizeof(pos));
		one.payload_indices.assign(kIndexWords,0);
		if (std::strcmp(ir_variant,"add") == 0) { one.payload_indices[0]=1; one.payload_indices[1]=1; one.payload_indices[2]=2; }
		else if (std::strcmp(ir_variant,"permuted") == 0 || std::strcmp(ir_variant,"frontend_permuted") == 0) { one.payload_indices[0]=2; one.payload_indices[1]=0; one.payload_indices[2]=1; }
		else { one.payload_indices[0]=0; one.payload_indices[1]=1; one.payload_indices[2]=2; }
		FinalizeScenario(one); scenarios.push_back(std::move(one));
	} else { scenarios = MakeScenarios(); }
	Expect(scenarios.size() == (ir_variant ? 1u : 9u), "expected scenario inventory");
	bool aborted = false;
	if (g_failures != 0 || g_validation_errors != 0) {
		return fail_and_cleanup();
	}
	// Generated UV mode initializes and checks both outputs inside the draw
	// submission itself. No standalone dispatch/readback may supply its geometry.
	for (size_t s_index = 0; s_index < scenarios.size() && !aborted && !ir_uv; ++s_index) {
		const Scenario& scenario = scenarios[s_index];
		PrintLine1("scenario %zu/%zu: %s (vertex_count=%u primitive_count=%u dispatch=%s)",
		           s_index + 1, scenarios.size(), scenario.name.c_str(),
		           scenario.vertex_count, scenario.primitive_count,
		           scenario.dispatch ? "yes" : "no");
		const auto started = std::chrono::steady_clock::now();
		if (scenario.upload_output) {
			std::memcpy(output_buffer.mapped, scenario.initial.words.data(), kTotalWords * 4u);
		} else {
			MeshOutputBlob retained;
			std::memcpy(retained.words.data(), output_buffer.mapped, kTotalBytes);
			CompareBlobWords(retained, scenario.initial, "recovery retains prior GPU-written overflow output");
			if (g_failures != 0) { return fail_and_cleanup(); }
		}
		std::vector<uint32_t> payload(static_cast<size_t>(kPositionWords + kIndexWords));
		std::copy(scenario.payload_positions.begin(), scenario.payload_positions.end(),
		          payload.begin());
		std::copy(scenario.payload_indices.begin(), scenario.payload_indices.end(),
		          payload.begin() + static_cast<std::ptrdiff_t>(kPositionWords));
		std::memcpy(payload_buffer.mapped, payload.data(),
		            (kPositionWords + kIndexWords) * 4u);
		VkMappedMemoryRange flush_ranges[2] = {};
		uint32_t flush_count = 0;
		if (output_buffer.noncoherent && scenario.upload_output) {
			flush_ranges[flush_count].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			flush_ranges[flush_count].memory = output_buffer.memory;
			flush_ranges[flush_count].size   = VK_WHOLE_SIZE;
			++flush_count;
		}
		if (payload_buffer.noncoherent) {
			flush_ranges[flush_count].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			flush_ranges[flush_count].memory = payload_buffer.memory;
			flush_ranges[flush_count].size   = VK_WHOLE_SIZE;
			++flush_count;
		}
		if (flush_count != 0) {
			if (!CheckVk(vFlushMappedMemoryRangesD(device, flush_count, flush_ranges),
			             "non-coherent host writes flushed before submission")) { return fail_and_cleanup(); }
		}
		if (!CheckVk(vResetCommandBufferD(command_buffer, 0), "command buffer reset")) { return fail_and_cleanup(); }
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (!CheckVk(vBeginCommandBufferD(command_buffer, &begin_info), "command buffer begin")) { return fail_and_cleanup(); }
		if (scenario.dispatch) {
			// The host-injected words become visible to the dispatch through an
			// explicit host-write -> compute-shader dependency; the status word,
			// guards and preserved data all ride in the mapped upload.
			VkMemoryBarrier host_barrier = {};
			host_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			host_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			host_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			vCmdPipelineBarrierD(command_buffer, VK_PIPELINE_STAGE_HOST_BIT,
			                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &host_barrier,
			                     0, nullptr, 0, nullptr);
			vCmdBindPipelineD(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			vCmdBindDescriptorSetsD(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
			                        pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
			const uint32_t push[2] = {scenario.vertex_count, scenario.primitive_count};
			vCmdPushConstantsD(command_buffer, pipeline_layout,
			                   VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), push);
			vCmdDispatchD(command_buffer, 1, 1, 1);
			VkMemoryBarrier host_read = {};
			host_read.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			host_read.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			host_read.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
			vCmdPipelineBarrierD(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			                     VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host_read, 0, nullptr, 0, nullptr);
		}
		if (!CheckVk(vEndCommandBufferD(command_buffer), "command buffer end")) { return fail_and_cleanup(); }
		VkSubmitInfo submit_info         = {};
		submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount   = 1;
		submit_info.pCommandBuffers      = &command_buffer;
		if (vQueueSubmitD(queue, 1, &submit_info, submit_fence) != VK_SUCCESS) {
			Expect(false, (scenario.name + ": queue submission succeeds").c_str());
			aborted = true;
			break;
		}
		const VkResult wait_result = vWaitForFencesD(device, 1, &submit_fence, VK_TRUE,
		                                             kFenceTimeoutNs);
		Expect(wait_result == VK_SUCCESS,
		       (scenario.name + ": fence signals within the 10 s gate").c_str());
		if (wait_result != VK_SUCCESS) {
			aborted = true;
			break;
		}
		if (!CheckVk(vResetFencesD(device, 1, &submit_fence), "fence reset for next scenario")) { return fail_and_cleanup(); }
		VkMappedMemoryRange invalidate_range = {};
		if (output_buffer.noncoherent) {
			invalidate_range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			invalidate_range.memory = output_buffer.memory;
			invalidate_range.size   = VK_WHOLE_SIZE;
			if (!CheckVk(vInvalidateMappedMemoryRangesD(device, 1, &invalidate_range),
			             "non-coherent output invalidated before readback")) { return fail_and_cleanup(); }
		}
		MeshOutputBlob actual;
		std::memcpy(actual.words.data(), output_buffer.mapped, kTotalWords * 4u);
		CompareBlobWords(actual, scenario.expected, (scenario.name + ": exact 460-word blob "
		                                             "match").c_str());
		if (g_failures != 0 || g_validation_errors != 0) { return fail_and_cleanup(); }
		const MeshOutputRead read = ReadOutput(actual);
		const double elapsed_ms =
		    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() -
		                                              started)
		        .count();
		PrintLine1("scenario %zu result: %s status=%u vertices=%u primitives=%u (%.3f ms)",
		           s_index + 1, scenario.name.c_str(), read.status, read.vertex_count,
		           read.primitive_count, elapsed_ms);
	}
	if (aborted) {
		return fail_and_cleanup();
	}
	if (draw && !ir_uv && !RunOffscreenDraw(device, get_device_proc, selected_memory, device_13_capable,
	                              queue, command_buffer, submit_fence, pipeline, pipeline_layout,
	                              descriptor_set, output_buffer, payload_buffer, textured && !image_path.empty() ? image_path.parent_path() / "untextured.ppm" : image_path, false, ir_variant ? &scenarios.front() : nullptr)) {
		return fail_and_cleanup();
	}
	if (textured && !RunOffscreenDraw(device, get_device_proc, selected_memory, device_13_capable,
	                                  queue, command_buffer, submit_fence, pipeline, pipeline_layout,
	                                  descriptor_set, output_buffer, payload_buffer, image_path, true, ir_uv ? &scenarios.front() : nullptr, ir_uv ? &uv_buffer : nullptr)) {
		return fail_and_cleanup();
	}
	// Validation and comparison results decide the run result; warnings alone do
	// not fail, but every validation ERROR does (a silenced failure is no pass).
	cleanup_all();
	PrintLine1("summary after cleanup: scenarios=%zu failures=%d validation-errors=%d validation-warnings=%d",
	           scenarios.size(), g_failures, g_validation_errors.load(), g_validation_warnings.load());
	return (g_failures == 0 && g_validation_errors == 0) ? 0 : 1;
}

} // namespace

int RunMain(int argc, char** argv) {
	if (!LocateShaders(argv[0])) {
		PrintLine1("RESULT: FAIL (cannot locate executable directory)");
		return 1;
	}
	if (argc == 3 && std::strcmp(argv[1], "--validate-module-vk13") == 0) {
		const auto binary = LoadSpirvFile(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(argv[2]))), "explicit Vulkan 1.3 fixture");
		ValidateSpirv(binary, SPV_ENV_VULKAN_1_3, "explicit Vulkan 1.3 fixture");
		CheckModuleShape(binary, SPV_ENV_VULKAN_1_3, "explicit Vulkan 1.3 fixture");
		PrintLine1("RESULT: %s (validation and shape)", g_failures == 0 ? "PASS" : "FAIL");
		return g_failures == 0 ? 0 : 1;
	}
	const std::string mode = argc >= 2 ? argv[1] : "";
	const bool ir_texture = mode == "--ir-texture" || mode == "--ir-texture-vulkan12";
	const bool ir_frontend = mode == "--frontend-draw" || mode == "--frontend-draw-vulkan12";
	const bool ir_draw = ir_frontend || mode == "--ir-draw" || mode == "--ir-draw-vulkan12";
	const bool ir_add = mode == "--ir-add" || mode == "--ir-add-vulkan12";
	const bool ir_mode = ir_texture || ir_draw || ir_add || mode == "--ir" || mode == "--ir-vulkan12";
	const std::vector<const char*> ir_variants = ir_frontend ? std::vector<const char*>{"frontend", "frontend_permuted", "frontend_zero"} : ir_texture ? std::vector<const char*>{"uv", "uv_zero"} : ir_add ? std::vector<const char*>{"add", "wrap"} : std::vector<const char*>{"triangle", "permuted", "zero"};
	const bool textured = ir_texture || mode == "--texture" || mode == "--texture-vulkan12";
	const bool draw = ir_draw || textured || mode == "--draw" || mode == "--draw-vulkan12";
	std::filesystem::path image_path;
	if (draw && argc == 4 && std::strcmp(argv[2], "--image-output") == 0) {
		image_path = std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(argv[3])));
	} else if (argc != 2) { PrintLine1("FAIL: unexpected command arguments"); return 1; }
	if (mode == "--check-texture-fixtures") { return CheckTextureFixtures(); }
	if (mode == "--validate-texture-spirv") { return ValidateTextureShaders() ? 0 : 1; }
	if (mode == "--check-draw-oracle") { return CheckDrawOracle(); }
	if (mode == "--validate-draw-spirv") { return ValidateDrawShaders() ? 0 : 1; }
	if (mode == "--check-host-fixtures") { return CheckHostFixtures(); }
	const bool validate_only = mode == "--validate-spirv";
	const bool runtime13 = mode == "--frontend-draw" || mode == "--run" || mode == "--draw" || mode == "--texture" || mode == "--ir" || mode == "--ir-add" || mode == "--ir-draw" || mode == "--ir-texture";
	const bool runtime12 = mode == "--frontend-draw-vulkan12" || mode == "--run-vulkan12" || mode == "--draw-vulkan12" || mode == "--texture-vulkan12" || mode == "--ir-vulkan12" || mode == "--ir-add-vulkan12" || mode == "--ir-draw-vulkan12" || mode == "--ir-texture-vulkan12";
	if (!validate_only && !runtime13 && !runtime12) {
		std::printf("[p41] usage: %s --validate-spirv | --run | --run-vulkan12 | --check-host-fixtures | --validate-module-vk13 FILE | --draw | --draw-vulkan12 [--image-output FILE] | --check-draw-oracle | --validate-draw-spirv | --texture | --texture-vulkan12 [--image-output FILE] | --check-texture-fixtures | --validate-texture-spirv | --ir | --ir-vulkan12 | --ir-add | --ir-add-vulkan12 | --ir-draw | --ir-draw-vulkan12 | --ir-texture | --ir-texture-vulkan12 | --frontend-draw | --frontend-draw-vulkan12 [--image-output FILE]\n", argv[0]);
		return 1;
	}
	if (!RunOfflineValidation() || (draw && !ValidateDrawShaders()) || (textured && !ValidateTextureShaders())) {
		PrintLine1("RESULT: FAIL (offline SPIR-V validation failed; the authored shader is "
		           "not a valid prototype)");
		return 1;
	}
	if (validate_only) {
		PrintLine1("RESULT: PASS (offline SPIR-V validation, both modules)");
		return 0;
	}
	if (ir_mode) {
		for (const char* variant : ir_variants) {
			const auto path = g_shader_directory / (std::string("mesh_ir_") + variant + (runtime13 ? "_vk13.spv" : "_vk12.spv"));
			const auto words = LoadSpirvFile(path, variant);
			const auto env = runtime13 ? SPV_ENV_VULKAN_1_3 : SPV_ENV_VULKAN_1_2;
			if (!ValidateSpirv(words, env, variant)) { return 1; }
			spvtools::SpirvTools validator(env); std::string disassembly;
			if (!validator.Disassemble(words, &disassembly) || disassembly.find("OpEntryPoint GLCompute") == std::string::npos ||
			    disassembly.find("LocalSize 32 1 1") == std::string::npos || disassembly.find("OpSetMeshOutputsEXT") != std::string::npos ||
			    disassembly.find("MeshShadingEXT") != std::string::npos) {
				Expect(false,"generated module execution model and shape"); return 1;
			}
		}
	}
	const uint32_t requested_api = runtime13 ? VK_API_VERSION_1_3 : VK_API_VERSION_1_2;
	int dispatch_result = 0;
	if (ir_mode) {
		for (const char* variant : ir_variants) {
			PrintLine1("IR case: %s", variant);
			auto variant_image = image_path;
			if ((ir_draw || ir_texture) && !image_path.empty()) {
				auto name = image_path.stem();
				name += std::filesystem::path(std::string("-") + variant);
				name += image_path.extension();
				variant_image = image_path.parent_path() / name;
			}
			dispatch_result = RunPrototype(requested_api, ir_draw || ir_texture, variant_image, ir_texture, variant);
			if (dispatch_result != 0 || g_failures != 0 || g_validation_errors != 0) { break; }
		}
	} else { dispatch_result = RunPrototype(requested_api, draw, image_path, textured); }
	// Setup/teardown errors must never be hidden by an otherwise legitimate skip.
	if (g_failures != 0 || g_validation_errors != 0) {
		PrintLine1("RESULT: FAIL (failures=%d validation-errors=%d including cleanup)",
		           g_failures, g_validation_errors.load());
		return 1;
	}
	if (dispatch_result == 0) {
		if (ir_mode) { PrintLine1("IR RESULT: PASS (generated modules, exact full blobs, validation including cleanup)"); }
		if (ir_texture) { PrintLine1("IR TEXTURE RESULT: PASS (generated UV and geometry, exact full buffers/RGBA, validation including cleanup)"); }
		if (textured && !ir_texture) { PrintLine1("TEXTURE RESULT: PASS (sampled texture, compute UVs/guards/boundaries, exact RGBA, validation including cleanup)"); }
		if (draw) { PrintLine1("DRAW RESULT: PASS (clear control + compute-generated indexed triangle, all pixels/full blob, validation including cleanup)"); }
		PrintLine1("RESULT: PASS (Vulkan %s%s; requested full-blob scenarios; core + synchronization validation including cleanup)",
		           runtime13 ? "1.3" : "1.2", runtime12 ? " AUXILIARY, not Vulkan 1.3 evidence" : "");
		return 0;
	}
	if (dispatch_result == kSkip) {
		PrintLine1("RESULT: SKIPPED (requested Vulkan %s runtime prerequisites unavailable; no dispatch)",
		           runtime13 ? "1.3" : "1.2 auxiliary");
		return kSkip;
	}
	PrintLine1("RESULT: FAIL (the dispatch path reported API or comparison failures)");
	return 1;
}

#ifdef _WIN32
// Windows CRT narrow argv uses the active code page, not UTF-8. Preserve Unicode
// result paths by decoding wide argv explicitly; shader lookup already uses W APIs.
int wmain(int argc, wchar_t** wide) {
	std::vector<std::string> storage;
	storage.reserve(argc);
	for (int i = 0; i < argc; ++i) {
		const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, nullptr, 0, nullptr, nullptr);
		if (bytes <= 0) { return 1; }
		std::string value(static_cast<size_t>(bytes), '\0');
		if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide[i], -1, value.data(), bytes, nullptr, nullptr)) { return 1; }
		value.pop_back();
		storage.push_back(std::move(value));
	}
	std::vector<char*> args;
	for (auto& value: storage) { args.push_back(value.data()); }
	return RunMain(argc, args.data());
}
#else
int main(int argc, char** argv) { return RunMain(argc, argv); }
#endif
