# S-060 — Atomic isolated compile bundle

Input7c793311 clean branch,14 matching recursive submodules. Implements D-035
only; no renderer activation, Vulkan command recording or native cache mutation.

## Implementation

Production-owned `graphics/host_gpu/renderer/meshFallbackBundle.h` composes the
actual policy, raw-input/private-clone compiler adapter, reflected vertex bridge
and canonical key. Derives specialization exclusively from the admitted Draw;
rejects a second caller specialization. Native preference/disabled/unsupported
policy outcomes cannot produce a fallback bundle. Stable diagnostic and policy
decision retained; every rejection publishes empty modules/interface/key and zero
ABI sizes. Successful result owns compute/vertex/fragment words, interface ID,
geometry/UV byte sizes and complete canonical key.

Caller still supplies complete canonical source/frontend-options identity, not a
hash; empty identity rejects. Bundle domain/version, length-prefixed source ID
and exact full compute/bridge/fragment modules additionally enter the existing
backend/context/ABI key. Request specialization pointer is consumed synchronously
and never retained. Source IR remains caller-owned and unmodified. Result is a
compile product, explicitly NOT authorization to record a draw.

Tests extend existing real mesh frontend fixtures (constant, triangle, permuted,
zero, UV and UV-zero). Independent public authored plain/float4 parameter fragments
validate via SPIR-V; actual production native pixel emission remains covered
separately by existing pairing tests, not claimed as a new bundle runtime test.
Checks include exact adapter-module equivalence, bridge validation, ABI sizes,
repeat determinism/source preservation, canonical-source/capacity/full-fragment
key separation; indexed/native/API1.2/duplicate specialization/empty identity/
missing or valid unsupported z-reader fragment/wave mismatch/wrong stage reject
atomically with repeat-stable diagnostic and recovery. Package staging updated
for both production policy/bundle header dependencies; syntax checked, no new
package built.

## Actual verification and provenance

First edit command failed decoding UTF-8 using platform default; original attempt
retained. Initial snapshot with only an untracked header failed reverse-patch
check on empty tracked diff, no claim of a valid build snapshot from that attempt.
Correct complete snapshots -02 and final -03 retained under
`_Build/p0-s012/p3-s060-pre-build-*/`; final code build uses -03. Builds2 jobs PASS,
no memory retry/PGO/IPO; existing upstream warnings. Logs/binary hash/module hashes
under ignored `_Build/p3-s060/`. Final43-case serial suite32 PASS/11 strict1.3
SKIPPED/0 FAIL(6.40s).44 existing generated modules byte-identical to S-053.
Diff and PowerShell staging-script syntax checks PASS. No emulator build requested
or renderer call-site added; header compiled through frontend CPU test target.

Six categories: Compilation scoped PASS; Unit/offline bundle regressions PASS;
Shader compilation/SPIR-V PASS,44 modules unchanged; existing Intel1.2 Vulkan
regressions PASS and strict1.3 SKIPPED; new bundle GPU/RDNA1 NOT RUN (S-059 native
standalone acceptance retained separately); Astro Bot NOT RUN. No transfer/push.

## Next single local task

Expose independently proven static output counts/capacities from lowering in the
bundle and derive a pure execution plan: exact geometry/UV allocation/binding/
vertex-index ranges and required consumer dependencies for triangle and zero.
A successful compile alone must never imply a fixed three-index draw: zero-output
programs are already admitted. Gate independent3-vertex/1-primitive and0/0 plans,
unsupported or unproven count rejection, overflow/offset/range safety, complete ABI
agreement and no partial plan. Preserve source/key separation; focused CPU/SPIR-V
regressions/provenance. Do not record Vulkan commands or activate renderer yet.
Actual scheduler allocation/descriptors/lifetime and command integration follow
as separate bounded tasks, with batched native verification under D-028.
