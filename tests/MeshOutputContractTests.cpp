// P2.1 RDNA1 mesh-fallback research (docs/RDNA1_MESH_FALLBACK_PLAN.md): the host-side
// output-buffer contract test for the planned synthetic compute prototype
// (docs/rdna1-mesh-architecture.md, "smallest independent compute prototype").
//
// The layout constants, mesh-output types and write/read helpers now live in the shared
// tests/MeshOutputContract.h (promoted in S-013 so the P4.1 GPU prototype embeds the exact
// same numbers). This file keeps the same nine cases: exact offsets/alignment, capacity
// arithmetic, zero output, the three-position/three-index prototype case, exact-capacity and
// overflow writes, and non-sticky overflow recovery. CPU/offline only; it exercises no
// Vulkan device, dispatch or guest shader translation.
//
// GPU visibility requires a separately successful P4.1 prototype run:
// explicit dependencies/dispatch/fence/invalidate, not this host test.

#include "MeshOutputContract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

using namespace Kyty::MeshOutput;

namespace {

int g_failures = 0;

void Expect(bool condition, const char* what) {
	if (!condition) {
		std::printf("MeshOutputContractTests: failed: %s\n", what);
		g_failures++;
	}
}

// Guard-row integrity only; written regions legitimately hold data.
void CheckGuards(const MeshOutputBlob& blob, const char* what) {
	Expect(GuardsIntact(blob), what);
}

void CheckOnlyStatusChanged(const MeshOutputBlob& before, const MeshOutputBlob& after,
                            const char* what) {
	for (size_t i = 0; i < before.words.size(); ++i) {
		if (i != kStatusWord && before.words[i] != after.words[i]) {
			std::printf("MeshOutputContractTests: changed word %zu: %08X -> %08X\n", i,
			            before.words[i], after.words[i]);
			Expect(false, what);
			return;
		}
	}
}

// The planned three-position/three-index prototype payload: three float4
// positions and one triangle (indices 0,1,2), all synthetic public values.
std::array<uint32_t, 12> MakePrototypePositions() {
	return PrototypePositions();
}

// ---------------------------------------------------------------------------
// Cases
// ---------------------------------------------------------------------------

void TestLayoutContract() {
	// Byte-level offsets that a SPIR-V module would hardcode.
	Expect(kPositionsOffsetBytes == 0, "positions at byte 0");
	Expect(kIndicesOffsetBytes == 1032, "indices region at byte 1032");
	Expect(kCountsOffsetBytes == 1808, "counts region at byte 1808");
	Expect(kStatusOffsetBytes == 1828, "status word at byte 1828");
	Expect(kTotalBytes == 1840, "blob is exactly 1840 bytes");
	// Alignment.
	Expect(kPositionsOffsetBytes % 16 == 0, "positions 16-byte aligned");
	Expect(kIndicesOffsetBytes % 4 == 0, "indices 4-byte aligned");
	Expect(kCountsOffsetBytes % 4 == 0, "counts 4-byte aligned");
	Expect(kStatusOffsetBytes % 4 == 0, "status is 4-byte aligned");
	Expect(kTotalBytes % 16 == 0, "blob size is a 16-byte multiple");
	// Capacity arithmetic.
	Expect(kMaxIndices == kMaxPrimitives * kIndicesPerPrimitive, "indices capacity arithmetic");
	Expect(kMaxVertices * 16 == 1024, "positions region spans 1024 bytes");
	Expect(kMaxIndices * 4 == 768, "indices region spans 768 bytes");
	Expect(size_t{kMaxVertices} * 16 + kGuardRowWords * 4 == kIndicesOffsetBytes,
	       "indices region follows the positions region and its guard row");
	Expect(kIndicesOffsetBytes + kMaxIndices * 4 + kGuardRowWords * 4 == kCountsOffsetBytes,
	       "counts region follows the indices region and its guard row");
	Expect(kCountsOffsetBytes + kCountWords * 4 + kGuardRowWords * 4 == kStatusOffsetBytes,
	       "status follows the counts region and its guard row");
	Expect(kStatusOffsetBytes + kStatusWords * 4 + kGuardRowWords * 4 == kTotalBytes,
	       "the final guard row ends the blob");
	// Regions and guard rows are disjoint and in bounds.
	Expect(kGuardWordIndexes.front() < kGuardWordIndexes.back(), "guard rows ordered");
	for (size_t w: kGuardWordIndexes) {
		Expect(w < kTotalWords, "guard word in bounds");
	}
	std::printf("layout: positions=0..1024 indices=1032..1800 counts=1808..1820 status=%zu total=%zu\n",
	            kStatusOffsetBytes, kTotalBytes);
}

void TestGuardInit() {
	MeshOutputBlob blob;
	blob.Init();
	for (size_t w: kGuardWordIndexes) {
		Expect(blob.words[w] == kGuardWord, "guard word holds the sentinel");
	}
	Expect(RegionUnwritten(blob, 0, kPositionWords), "positions region zero-initialized");
	Expect(RegionUnwritten(blob, kIndicesStartWord, kIndexWords), "indices region zero-initialized");
	Expect(RegionUnwritten(blob, kCountsStartWord, kCountWords), "counts region zero-initialized");
	Expect(ReadOutput(blob).status == kStatusPending, "initialized output is Pending, not Ok");
	std::printf("guard-init: %zu guard words hold the sentinel\n", kGuardWordIndexes.size());
}

void TestZeroOutput() {
	MeshOutputBlob blob;
	blob.Init();
	const MeshOutputWrite write = {0, nullptr, 0, nullptr};
	const uint32_t        status = WriteOutput(blob, write);
	Expect(status == kStatusOk, "zero output succeeds");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOk, "committed status is Ok");
	Expect(read.vertex_count == 0, "zero vertex count");
	Expect(read.primitive_count == 0, "zero primitive count");
	Expect(blob.words[kCountsStartWord + 2] == 0, "reserved count word stays zero");
	Expect(RegionUnwritten(blob, 0, kPositionWords), "zero output writes no position");
	Expect(RegionUnwritten(blob, kIndicesStartWord, kIndexWords), "zero output writes no index");
	CheckGuards(blob, "zero output leaves the guard rows intact");
	std::printf("zero-output: status=%u vertex_count=%u primitive_count=%u\n", read.status,
	            read.vertex_count, read.primitive_count);
}

void TestPrototypeTriangle() {
	MeshOutputBlob blob;
	blob.Init();
	const std::array<uint32_t, 12> positions = MakePrototypePositions();
	const std::array<uint32_t, 3>  indices   = {0, 1, 2};
	const MeshOutputWrite          write     = {3, positions.data(), 1, indices.data()};

	const uint32_t status = WriteOutput(blob, write);
	Expect(status == kStatusOk, "prototype triangle succeeds");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOk, "committed status is Ok");
	Expect(read.vertex_count == 3, "vertex_count is 3");
	Expect(read.primitive_count == 1, "primitive_count is 1");
	Expect(blob.words[kCountsStartWord + 2] == 0, "reserved count word stays zero");

	// Exact positions; everything past the written record stays zero.
	Expect(std::memcmp(&blob.words[0], positions.data(), positions.size() * sizeof(uint32_t)) == 0,
	       "the three float4 positions landed at the exact contract offset");
	Expect(RegionUnwritten(blob, positions.size(), kPositionWords - positions.size()),
	       "no position beyond the three vertices");

	// Exact indices at the exact region offset; everything past them stays zero.
	Expect(std::memcmp(&blob.words[kIndicesStartWord], indices.data(),
	                  indices.size() * sizeof(uint32_t)) == 0,
	       "the three uint32 indices landed at the exact contract offset");
	Expect(RegionUnwritten(blob, kIndicesStartWord + indices.size(),
	                       kIndexWords - indices.size()),
	       "no index beyond the three indices");

	CheckGuards(blob, "guards intact after the prototype triangle");
	std::printf("prototype: vertex_count=%u primitive_count=%u indices=0,1,2\n", read.vertex_count,
	            read.primitive_count);
}

void TestExactCapacity() {
	// A full-capacity write must succeed and reach the last word of each region
	// without touching its guard row.
	MeshOutputBlob blob;
	blob.Init();

	std::array<uint32_t, kMaxVertices * 4> positions;
	for (size_t i = 0; i < positions.size(); ++i) {
		positions[i] = uint32_t(0x10000000u + i);
	}
	std::array<uint32_t, kMaxIndices> indices;
	for (size_t i = 0; i < indices.size(); ++i) {
		indices[i] = uint32_t(i % kMaxVertices);
	}

	const MeshOutputWrite write = {kMaxVertices, positions.data(), kMaxPrimitives, indices.data()};
	const uint32_t        status = WriteOutput(blob, write);
	Expect(status == kStatusOk, "exact-capacity write succeeds");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOk, "exact-capacity write commits stored Ok");
	Expect(read.vertex_count == kMaxVertices, "full vertex count");
	Expect(read.primitive_count == kMaxPrimitives, "full primitive count");

	Expect(std::memcmp(&blob.words[0], positions.data(), positions.size() * sizeof(uint32_t)) == 0,
	       "all position slots written exactly");
	Expect(std::memcmp(&blob.words[kIndicesStartWord], indices.data(),
	                  indices.size() * sizeof(uint32_t)) == 0,
	       "all index slots written exactly");
	CheckGuards(blob, "guards intact after exact-capacity write");
	std::printf("exact-capacity: vertices=%u primitives=%u indices=%u\n", read.vertex_count,
	            read.primitive_count, kMaxIndices);
}

void TestOverflowVertices() {
	// One position past capacity: reject with the explicit overflow status and
	// write nothing but the status word.
	MeshOutputBlob blob;
	InitOverflowSnapshot(blob);
	const MeshOutputBlob before = blob;
	std::array<uint32_t, (kMaxVertices + 1) * 4> positions;
	positions.fill(0x55555555u);
	const std::array<uint32_t, 3> indices = {0, 1, 2};
	const MeshOutputWrite write    = {kMaxVertices + 1, positions.data(), 1, indices.data()};

	const uint32_t status = WriteOutput(blob, write);
	Expect(status == kStatusOverflowVertices, "vertex overflow reports its status");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOverflowVertices, "committed status is the overflow status");
	CheckOnlyStatusChanged(before, blob, "vertex overflow preserves every non-status word");
	CheckGuards(blob, "guards intact after vertex overflow");
	std::printf("overflow-vertices: status=%u requested=%u capacity=%u\n", read.status,
	            kMaxVertices + 1, kMaxVertices);
}

void TestOverflowIndices() {
	// One primitive (three indices) past capacity: reject with its own status and
	// write nothing but the status word. The valid-capacity positions of this
	// request stay unwritten too.
	MeshOutputBlob blob;
	InitOverflowSnapshot(blob);
	const MeshOutputBlob before = blob;
	const std::array<uint32_t, 12> positions = MakePrototypePositions();
	std::array<uint32_t, (kMaxPrimitives + 1) * kIndicesPerPrimitive> indices;
	indices.fill(0x55555555u);
	const MeshOutputWrite write = {3, positions.data(), kMaxPrimitives + 1, indices.data()};

	const uint32_t status = WriteOutput(blob, write);
	Expect(status == kStatusOverflowIndices, "index overflow reports its status");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOverflowIndices, "committed status is the overflow status");
	CheckOnlyStatusChanged(before, blob, "index overflow preserves every non-status word");
	CheckGuards(blob, "guards intact after index overflow");
	std::printf("overflow-indices: status=%u requested_primitives=%u capacity=%u\n", read.status,
	            kMaxPrimitives + 1, kMaxPrimitives);
}

void TestOverflowCombined() {
	// Both capacities exceeded: the vertex check runs first, so the status is the
	// vertex overflow status and nothing beside the status word is written.
	MeshOutputBlob blob;
	InitOverflowSnapshot(blob);
	const MeshOutputBlob before = blob;
	const std::array<uint32_t, (kMaxVertices + 1) * 4> positions{};
	const std::array<uint32_t, (kMaxPrimitives + 1) * kIndicesPerPrimitive> indices{};
	const MeshOutputWrite write = {
		kMaxVertices + 1, positions.data(), kMaxPrimitives + 1, indices.data()};

	const uint32_t status = WriteOutput(blob, write);
	Expect(status == kStatusOverflowVertices, "combined overflow reports the vertex status first");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOverflowVertices, "committed status is the vertex overflow status");
	CheckOnlyStatusChanged(before, blob, "combined overflow preserves every non-status word");
	CheckGuards(blob, "guards intact after combined overflow");
	std::printf("overflow-combined: status=%u\n", read.status);
}

void TestOverflowIgnoresFullCapacity() {
	// The writer must still accept the maximum itself after a rejected
	// over-capacity request: capacity checks are comparisons, not sticky state.
	MeshOutputBlob blob;
	InitOverflowSnapshot(blob);
	const MeshOutputBlob before = blob;
	const std::array<uint32_t, 12> overflow_positions = {};
	const MeshOutputWrite overflow = {kMaxVertices + 1, overflow_positions.data(), 0, nullptr};
	Expect(WriteOutput(blob, overflow) == kStatusOverflowVertices, "rejected request");
	Expect(ReadOutput(blob).status == kStatusOverflowVertices, "rejected request commits stored error");
	CheckOnlyStatusChanged(before, blob, "rejected request preserves every non-status word before retry");

	std::array<uint32_t, kMaxVertices * 4> positions;
	positions.fill(0x12345678u);
	std::array<uint32_t, kMaxIndices> indices;
	indices.fill(0x2468ACE0u);
	const MeshOutputWrite exact = {kMaxVertices, positions.data(), kMaxPrimitives, indices.data()};
	const uint32_t        status = WriteOutput(blob, exact);
	Expect(status == kStatusOk, "maximum-capacity request still succeeds afterwards");
	const MeshOutputRead read = ReadOutput(blob);
	Expect(read.status == kStatusOk, "overflow recovery commits stored Ok over the previous error");
	Expect(blob.words[kCountsStartWord + 2] == 0, "recovery clears the reserved count word");
	Expect(read.vertex_count == kMaxVertices && read.primitive_count == kMaxPrimitives,
	       "maximum counts after the rejected request");
	Expect(std::memcmp(&blob.words[0], positions.data(), positions.size() * sizeof(uint32_t)) == 0,
	       "positions written after the rejected request");
	Expect(std::memcmp(&blob.words[kIndicesStartWord], indices.data(), kMaxIndices * sizeof(uint32_t)) == 0,
	       "indices written after the rejected request");
	CheckGuards(blob, "guards intact after the rejected-then-maximum sequence");
	std::printf("overflow-recovery: status=%u vertices=%u primitives=%u\n", read.status,
	            read.vertex_count, read.primitive_count);
}

} // namespace

int main() {
	TestLayoutContract();
	TestGuardInit();
	TestZeroOutput();
	TestPrototypeTriangle();
	TestExactCapacity();
	TestOverflowVertices();
	TestOverflowIndices();
	TestOverflowCombined();
	TestOverflowIgnoresFullCapacity();
	if (g_failures != 0) {
		std::printf("MeshOutputContractTests: failed: %d check(s)\n", g_failures);
		return 1;
	}
	std::printf("MeshOutputContractTests: ok\n");
	return 0;
}
