// Shared mesh-output-buffer contract for the P2.1 host test and the P4.1 synthetic
// Vulkan compute prototype (docs/RDNA1_MESH_FALLBACK_PLAN.md, S-010/S-012/S-013).
//
// The contract is one fixed-capacity blob with four regions - positions (float4), indices
// (uint32), counts (vertex/primitive/reserved) and a status word written last - separated by
// guard rows. Every region starts at a constant offset with a constant capacity so a GPU
// prototype can reproduce the layout exactly (positions 0, indices 1032, counts 1808,
// status 1828, total 1840 bytes).
//
// On insufficient capacity the writer must reject with an explicit error status and must not
// write out of bounds: nothing but the status word may change (D-017). A final status is
// written last; readers must check it before trusting counts, including for zero output.
// Pending initialization and stored-success checks must be enforced by the callers' tests;
// this header carries no temporal or concurrency guarantee.

#ifndef KYTY_MESH_OUTPUT_CONTRACT_H_
#define KYTY_MESH_OUTPUT_CONTRACT_H_

#include <array>
#include <cstddef>
#include <cstdint>

namespace Kyty { namespace MeshOutput {

inline constexpr uint32_t kGuardWord = 0xC0DEFEEDu; // one uint32 guard-row sentinel

inline constexpr uint32_t kMaxVertices         = 64u;  // float4 position slots
inline constexpr uint32_t kMaxPrimitives       = 64u;  // primitive records
inline constexpr uint32_t kIndicesPerPrimitive = 3u;   // uint32 indices per primitive record
inline constexpr uint32_t kMaxIndices          = kMaxPrimitives * kIndicesPerPrimitive; // 192

// Pending distinguishes initialized storage from a completed write, even for
// zero output. A final status is written last; readers must check it before
// trusting counts. The host tests do not establish GPU memory visibility:
// P4.1 must use explicit synchronization and fenced readback, not status polling.
inline constexpr uint32_t kStatusPending          = 0xFFFFFFFFu;
inline constexpr uint32_t kStatusOk               = 0u;
inline constexpr uint32_t kStatusOverflowVertices = 1u;
inline constexpr uint32_t kStatusOverflowIndices  = 2u;

// Region sizes in uint32 words.
inline constexpr size_t kPositionWords = size_t{kMaxVertices} * 4u;             // 256
inline constexpr size_t kIndexWords    = size_t{kMaxIndices};                   // 192
inline constexpr size_t kCountWords    = 3u;   // vertex_count, primitive_count, reserved
inline constexpr size_t kStatusWords   = 1u;
inline constexpr size_t kGuardRowWords = 2u;    // every region boundary has one guard row

// Region boundaries in uint32 words; the guard rows follow every region:
// [positions][G][indices][G][counts][G][status][G]
inline constexpr size_t kGuardAfterPositionsWord = kPositionWords;                     // 256
inline constexpr size_t kIndicesStartWord  = kGuardAfterPositionsWord + kGuardRowWords; // 258
inline constexpr size_t kGuardAfterIndicesWord   = kIndicesStartWord + kIndexWords;     // 450
inline constexpr size_t kCountsStartWord         = kGuardAfterIndicesWord + kGuardRowWords; // 452
inline constexpr size_t kGuardAfterCountsWord    = kCountsStartWord + kCountWords;      // 455
inline constexpr size_t kStatusWord        = kGuardAfterCountsWord + kGuardRowWords;    // 457
inline constexpr size_t kTotalWords        = kStatusWord + kStatusWords + kGuardRowWords; // 460

// Byte offsets of the same regions, the numbers the compute shader hardcodes.
inline constexpr size_t kPositionsOffsetBytes = 0u;
inline constexpr size_t kIndicesOffsetBytes   = kIndicesStartWord * 4u; // 1032
inline constexpr size_t kCountsOffsetBytes    = kCountsStartWord * 4u;  // 1808
inline constexpr size_t kStatusOffsetBytes    = kStatusWord * 4u;       // 1828
inline constexpr size_t kTotalBytes           = kTotalWords * 4u;       // 1840

// The eight guard-row words checked against the sentinel.
inline constexpr std::array<size_t, 8> kGuardWordIndexes = {
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
static_assert(kGuardAfterPositionsWord == 256, "guard row after positions");
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
static_assert(kStatusPending != kStatusOk && kStatusPending != kStatusOverflowVertices &&
              kStatusPending != kStatusOverflowIndices, "Pending is not a completed status");

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

// Write sequence mirrored by the P4.1 compute shader: positions, then indices, then
// counts, then the Ok status word. Initialize before first use; a rejected request
// preserves existing data, including on reuse. Sequential retries are supported;
// concurrent readers/writers are outside this host contract.
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

inline uint32_t WriteOutput(MeshOutputBlob& blob, const MeshOutputWrite& write) {
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

inline MeshOutputRead ReadOutput(const MeshOutputBlob& blob) {
	return {
		blob.words[kStatusWord],
		blob.words[kCountsStartWord + 0],
		blob.words[kCountsStartWord + 1],
	};
}

inline bool GuardsIntact(const MeshOutputBlob& blob) {
	for (size_t w: kGuardWordIndexes) {
		if (blob.words[w] != kGuardWord) {
			return false;
		}
	}
	return true;
}

inline bool RegionUnwritten(const MeshOutputBlob& blob, size_t start_word, size_t word_count) {
	for (size_t i = start_word; i < start_word + word_count; ++i) {
		if (blob.words[i] != 0) {
			return false;
		}
	}
	return true;
}

// Give every non-status word a nonzero value before rejected requests, so even
// unintended zero stores are observable. Preserve the actual guard sentinels.
inline void InitOverflowSnapshot(MeshOutputBlob& blob) {
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

// The planned three-position/three-index prototype payload: three float4
// positions and one triangle (indices 0,1,2), all synthetic public values.
inline std::array<uint32_t, 12> PrototypePositions() {
	return {
		0x3F800000u, 0x40000000u, 0x00000000u, 0x3F800000u, // ( 1.0,  2.0,  0.0, 1.0)
		0xC0000000u, 0x3FC00000u, 0x00000000u, 0x3F800000u, // (-2.0,  1.5,  0.0, 1.0)
		0x3F000000u, 0xC0400000u, 0x40000000u, 0x3F800000u, // ( 0.5, -3.0,  2.0, 1.0)
	};
}

}} // namespace Kyty::MeshOutput

#endif // KYTY_MESH_OUTPUT_CONTRACT_H_
