# S-067 — Dedicated unsubmitted compute descriptors

Input `511a96f3`, branch `feature/rdna1-mesh-fallback`; clean source and 14
matching recursive submodules. Continued D-033 after the S-066 allocation gate.

`ComputeDescriptors::TryCreate` creates a dedicated layout, one-set pool and
storage-buffer set. Geometry is binding1 and optional UV binding2, offset0 and
the complete guarded 1840/544-byte ranges. There is no payload binding0.
Canonical plan, actual buffer sizes/handles and UV presence must agree before
any Vulkan setup. Exact errors identify layout/pool/set or host-object failure.
Vulkan descriptor update is void; no invented success return or exception gate.

`AttachDescriptors` owns the partial buffers privately, destroys descriptor
setup before buffers on failure, and publishes a complete owner on success.
Owner destruction releases pool/layout before buffers. Move construction works;
move assignment is deliberately deleted to prevent member-wise replacement
from destroying buffers before their existing descriptor owner. Production
`PrepareResources` composes the successful compiler bundle with the real Buffer
boundary and this descriptor owner. No renderer call site and no GPU submission.
The existing shared `DescriptorHeap` and its native failure/reuse paths are
unchanged. These resources remain **unsubmitted only**.

## Verification

- Complete pre-build dirty snapshots `p2-s067-pre-build-01` and `-02` under
  `_Build/p0-s012/`; final code is snapshot02. Full diff, untracked raw source,
  hashes, cache/submodule/toolchain provenance retained. Logs `_Build/p2-s067/`.
- Both two-job builds PASS, including actual out-of-line resource/descriptor
  implementation and full emulator link, and compile-only boundary consumer.
  No PGO/IPO or memory retry. Existing upstream warnings retained.
- Actual Intel Iris Xe API1.2.172 auxiliary real Buffer/VMA/Vulkan tests: PASS.
  All 16 zero/nonzero, UV on/off, success/layout/pool/set failure combinations
  tested. Test-local Vulkan dispatcher refusal wrappers forward successful
  calls and destruction to the real driver. Precise host-memory/device-memory/
  pool-memory errors remain distinguishable, and no partial resources escape.
- 24 native buffers for descriptor scenarios all destroyed once; four complete
  descriptor updates. Independent callback inspection checks every binding,
  type, stage, handle, range and pool count. Failed setup performs no update.
  Layout/pool live counts and VMA allocation counts return to zero before failed
  return and after success-owner destruction. Malformed plan rejects before
  Vulkan setup; malformed empty buffers with success status reject before factory.
- S-066 real allocation cases also PASS:8 created/destroyed,18 refusals.
  **Zero core/synchronization validation errors and warnings through cleanup.**
- Serial focused49-case CTest: **36 PASS /13 SKIPPED /0 FAIL**,7.71s.
  Process-local retained validation-layer path. Strict1.3 SKIPPED on Intel1.2;
  existing CPU/offline/SPIR-V/Intel draw and full-buffer/image regressions PASS.

Six categories: compilation PASS; unit PASS; shader/SPIR-V regressions PASS;
Vulkan integration PASS Intel1.2 auxiliary, strict1.3 SKIPPED; new RDNA1 hardware
NOT RUN; Astro Bot NOT RUN. No physical OOM or noncoherent guarantee, GPU execution
using these descriptor owners, completion retention or production activation claim.
Prior S-064 native isolated bundle evidence remains separate.

Next bounded task: scheduler admission and retention before first resource use,
through the final compute/graphics use in that same recording. Reject inactive,
draining/closed and mismatched-tick registration without publishing usable views;
prove delayed exact-once cleanup, no reuse, abandonment and shutdown behavior.
Do not add per-draw host waits. Actual scheduler/native tests are distinct from
the controlled CPU completion model and standalone Intel allocation evidence.
