// P2.1 RDNA1 mesh-fallback research (docs/RDNA1_MESH_FALLBACK_PLAN.md): the host-side
// output-buffer contract for the planned synthetic compute prototype
// (docs/rdna1-mesh-architecture.md, "smallest independent compute prototype").
//
// The contract is one fixed-capacity blob with four regions - positions (float4), indices
// (uint32), counts (vertex/primitive/reserved) and a status word written last - separated by
// guard rows. Every region starts at a constant offset with a constant capacity so a GPU
// prototype can reproduce the layout exactly.
//
// Tested here, isolated from production rendering: exact offsets/alignment, capacity
// arithmetic, zero output, the three-position/three-index prototype case, exact-capacity and
// overflow writes. On insufficient capacity the writer must reject with an explicit error
// status and must not write out of bounds: nothing but the status word may change. This is a
// CPU/offline test; it exercises no Vulkan device, dispatch or guest shader translation and
// proves nothing about PS5 mesh shaders or game compatibility.

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

int g_failures = 0;

void Expect(bool condition, const char* what) {
	if (!condition) {
		std::printf("MeshOutputContractTests: failed: %s\n", what);
		g_failures++;
	}
}

// ---------------------------------------------------------------------------
// The layout contract. All values are host-mirrored constants of the planned
// std430 storage-buffer layout; a GPU prototype must embed the same numbers.
// ---------------------------------------------------------------------------

// One uint32 guard-row sentinel. Guard rows sit between (and after) the written
// regions so any out-of-bounds or aliased write is detectable by value.
constexpr uint32_t kGuardWord = 0xC0DEFEEDu;

constexpr uint32_t kMaxVertices        = 64u;  // float4 position slots
constexpr uint32_t kMaxPrimitives      = 64u;  // primitive records
constexpr uint32_t kIndicesPerPrimitive = 3u;  // uint32 indices per primitive record
constexpr uint32_t kMaxIndices         = kMaxPrimitives * kIndicesPerPrimitive; // 192

// Pending distinguishes initialized storage from a completed write, even for
// zero output. A final status is written last; readers must check it before
// trusting counts. This host test does not establish GPU memory visibility:
// P4.1 must use explicit synchronization and fenced readback, not status polling.
constexpr uint32_t kStatusPending          = 0xFFFFFFFFu;
constexpr uint32_t kStatusOk               = 0u;
constexpr uint32_t kStatusOverflowVertices = 1u;
constexpr uint32_t kStatusOverflowIndices  = 2u;
static_assert(kStatusPending != kStatusOk && kStatusPending != kStatusOverflowVertices &&
              kStatusPending != kStatusOverflowIndices, "Pending is not a completed status");

// Region sizes in uint32 words.
constexpr size_t kPositionWords = size_t{kMaxVertices} * 4u;                  // 256
constexpr size_t kIndexWords    = size_t{kMaxIndices};                        // 192
constexpr size_t kCountWords    = 3u;  // vertex_count, primitive_count, reserved
constexpr size_t kStatusWords   = 1u;
constexpr size_t kGuardRowWords = 2u;  // every region boundary has one guard row

// Region boundaries in uint32 words; the guard rows follow every region:
// [positions][G][indices][G][counts][G][status][G]
constexpr size_t kGuardAfterPositionsWord = kPositionWords;                     // 256
constexpr size_t kIndicesStartWord        = kGuardAfterPositionsWord + kGuardRowWords; // 258
constexpr size_t kGuardAfterIndicesWord   = kIndicesStartWord + kIndexWords;    // 450
constexpr size_t kCountsStartWord         = kGuardAfterIndicesWord + kGuardRowWords;   // 452
constexpr size_t kGuardAfterCountsWord    = kCountsStartWord + kCountWords;     // 455
constexpr size_t kStatusWord              = kGuardAfterCountsWord + kGuardRowWords;    // 457
constexpr size_t kTotalWords              = kStatusWord + kStatusWords + kGuardRowWords; // 460

// Byte offsets of the same regions, the numbers a SPIR-V module would hardcode.
constexpr size_t kPositionsOffsetBytes = 0u;
constexpr size_t kIndicesOffsetBytes   = kIndicesStartWord * 4u;  // 1032
constexpr size_t kCountsOffsetBytes    = kCountsStartWord * 4u;   // 1808
constexpr size_t kStatusOffsetBytes    = kStatusWord * 4u;        // 1828
constexpr size_t kTotalBytes           = kTotalWords * 4u;        // 1840

// The eight guard-row words checked against the sentinel.
constexpr std::array<size_t, 8> kGuardWordIndexes = {
	kGuardAfterPositionsWord, kGuardAfterPositionsWord + 1,
	kGuardAfterIndicesWord, kGuardAfterIndicesWord + 1,
	kGuardAfterCountsWord, kGuardAfterCountsWord + 1,
	kStatusWord + 1, kStatusWord + 2,
};

// Static layout arithmetic: the exact identities the contract depends on. If one
// of these changes, every hardcoded GPU-side offset must be reviewed with it.
static_assert(kMaxIndices == kMaxPrimitives * kIndicesPerPrimitive, "capacity arithmetic");
static_assert(kPositionWords == kMaxVertices * 4, "positions capacity arithmetic");
static_assert(kIndicesStartWord == 258, "indices region word offset");
constexpr size_t checked_guard_positions_word = kGuardAfterPositionsWord;
static_assert(checked_guard_positions_word == 256, "guard row after positions");
static_assert(kIndicesOffsetBytes == 1032, "indices region byte offset");
static_assert(kCountsOffsetBytes == 1808, "counts region byte offset");
static_assert(kStatusOffsetBytes == 1828, "status byte offset");
static_assert(kTotalBytes == 1840, "total blob size");
static_assert(kTotalBytes % 16 == 0, "16-byte sized blob");
static_assert(kPositionsOffsetBytes % 16 == 0, "float4 positions are 16-byte aligned");
static_assert(kIndicesOffsetBytes % 4 == 0, "uint32 indices are 4-byte aligned");
static_assert(kCountsOffsetBytes % 4 == 0, "counts are 4-byte aligned");
static_assert(kStatusOffsetBytes % 4 == 0, "status is 4-byte aligned");
static_assert(kIndicesOffsetBytes == kMaxVertices * 16 + kGuardRowWords * 4,
	"indices follow the positions guard row");
static_assert(kCountsStartWord == 452, "counts region word offset");
static_assert(kGuardAfterCountsWord == 455, "guard row after counts");
static_assert(kStatusWord == 457, "status word offset");

// One output blob. Zero-initialized except the guard rows and Pending status.
struct MeshOutputBlob {
	std::array<uint32_t, kTotalWords> words;

	void Init() {
		words.fill(0);
		words[kStatusWord] = kStatusPending;
		for (size_t w: kGuardWordIndexes) {
			words[w] = kGuardWord;
		}
	}
};

// Host mirror of the planned compute-shader write sequence: positions, then
// indices, then counts, then the Ok status word. Initialize before first use;
// a rejected request preserves existing data, including on reuse. Sequential
// retries are supported; concurrent readers/writers are outside this host test.
//
// Insufficient capacity must never write: the blob keeps everything except the
// status word, which records the specific overflow. Vertex overflow is checked
// first, so a combined overflow reports OverflowVertices deterministically.
struct MeshOutputWrite {
	uint32_t       vertex_count;
	const uint32_t* positions;   // vertex_count * 4 uint32, bitwise float4 payload
	uint32_t       primitive_count;
	const uint32_t* indices;     // primitive_count * kIndicesPerPrimitive uint32
};

uint32_t WriteOutput(MeshOutputBlob& blob, const MeshOutputWrite& write) {
	if (write.vertex_count > kMaxVertices) {
		blob.words[kStatusWord] = kStatusOverflowVertices;
		return kStatusOverflowVertices;
	}
	if (write.primitive_count > kMaxPrimitives) {
		blob.words[kStatusWord] = kStatusOverflowIndices;
		return kStatusOverflowIndices;
	}
	const size_t position_words = size_t{write.vertex_count} * 4u;
	for (size_t i = 0; i < position_words; ++i) {
		blob.words[i] = write.positions[i];
	}
	const size_t index_words = size_t{write.primitive_count} * kIndicesPerPrimitive;
	for (size_t i = 0; i < index_words; ++i) {
		blob.words[kIndicesStartWord + i] = write.indices[i];
	}
	blob.words[kCountsStartWord + 0] = write.vertex_count;
	blob.words[kCountsStartWord + 1] = write.primitive_count;
	blob.words[kCountsStartWord + 2] = 0;
	blob.words[kStatusWord]          = kStatusOk; // written last
	return kStatusOk;
}

// Host inspection helper. Callers must check status before trusting counts;
// counts may contain preserved data when status is Pending or an error.
struct MeshOutputRead {
	uint32_t status;
	uint32_t vertex_count;
	uint32_t primitive_count;
};

MeshOutputRead ReadOutput(const MeshOutputBlob& blob) {
	return {
		blob.words[kStatusWord],
		blob.words[kCountsStartWord + 0],
		blob.words[kCountsStartWord + 1],
	};
}

// ---------------------------------------------------------------------------
// Checks
// ---------------------------------------------------------------------------

bool GuardsIntact(const MeshOutputBlob& blob) {
	for (size_t w: kGuardWordIndexes) {
		if (blob.words[w] != kGuardWord) {
			return false;
		}
	}
	return true;
}

bool RegionUnwritten(const MeshOutputBlob& blob, size_t start_word, size_t word_count) {
	for (size_t i = start_word; i < start_word + word_count; ++i) {
		if (blob.words[i] != 0) {
			return false;
		}
	}
	return true;
}

// Guard-row integrity only; written regions legitimately hold data.
void CheckGuards(const MeshOutputBlob& blob, const char* what) {
	Expect(GuardsIntact(blob), what);
}

// Give every non-status word a nonzero value before rejected requests, so even
// unintended zero stores are observable. Preserve the actual guard sentinels.
void InitOverflowSnapshot(MeshOutputBlob& blob) {
	blob.Init();
	for (size_t i = 0; i < blob.words.size(); ++i) {
		if (i != kStatusWord) {
			blob.words[i] = 0xA5000000u + uint32_t(i);
		}
	}
	for (size_t w: kGuardWordIndexes) {
		blob.words[w] = kGuardWord;
	}
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
	return {
		0x3F800000u, 0x40000000u, 0x00000000u, 0x3F800000u, // ( 1.0,  2.0,  0.0, 1.0)
		0xC0000000u, 0x3FC00000u, 0x00000000u, 0x3F800000u, // (-2.0,  1.5,  0.0, 1.0)
		0x3F000000u, 0xC0400000u, 0x40000000u, 0x3F800000u, // ( 0.5, -3.0,  2.0, 1.0)
	};
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
	Expect(kStatusOffsetBytes % 4 == 0, "status 4-byte aligned");
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
