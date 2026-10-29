# S-066 — Recoverable, unsubmitted Buffer preparation

Input: `753d376e`, branch `feature/rdna1-mesh-fallback`. This closes the first
bounded prerequisite identified by S-065, not the entire renderer resource
lifetime milestone. No renderer call site, descriptor ownership, GPU commands,
submission, retirement, game execution or package is added here.

## Implementation

- `Buffer::TryCreate` returns a precise `vk::Result` and empty ownership on
  failure. Its subset is non-sparse, non-device-address scratch allocation.
  Existing public constructor and callers retain fatal final-failure behavior.
  Shared VMA allocation, host-memory operations and destruction moved from
  `streamBuffer.cpp` to `bufferAllocation.cpp`; GPU recording remains separate.
- Both entry points use the same reclaim/retry sequence: attempt, reclaim once,
  retry, relax within-budget restriction, final attempt. CPU tests exercise the
  actual shared retry helper, including distinct final error propagation.
- `PrepareBufferOwnership` validates the full checked execution layout, obtains
  geometry then optional UV privately, and publishes ownership only after all
  allocations succeed. Failure destroys partial resources before returning.
  Zero-output plans still allocate guarded ABI storage. Move ownership releases
  each successful allocation once.
- Production `PrepareBuffers` accepts a successful compiler bundle, uses real
  `Buffer::TryCreate`, requires mapped scratch, and preserves storage/vertex/index
  usage. Its out-of-line implementation is compiled and linked into the emulator;
  a compile-only test consumer checks the public boundary.

This owner is explicitly **unsubmitted**. Destruction is immediate. It must not
be used to record or submit work until descriptor ownership and last-use
completion retention are implemented. Null scheduler is supported for allocation
and host memory only; `Fill` rejects it. No per-draw wait was introduced.

## Actual evidence

Evidence: ignored `_Build/p2-s066/`; complete dirty diffs, untracked inventories,
raw changed-source copies/hashes, cache flags, toolchain and all 14 matching
submodules retained in `_Build/p0-s012/p2-s066-pre-build-01/` through `-09/`.
Final code snapshot is `-09`; later documentation does not alter built code.
Two jobs, PGO generation OFF, PGO use empty, emulator IPO OFF.

- CPU transaction/retry tests: success, zero/nonzero geometry, UV on/off,
  geometry/UV refusal, malformed plan, null success, partial handle plus error,
  move ownership and precise final error. PASS.
- Real `Buffer` + real VMA test on Intel Iris Xe, vendor `8086`, device `9A49`,
  API `1.2.172`, encoded driver `1647965`: PASS. Eight successful native buffers
  created and destroyed; 18 injected `vkCreateBuffer` refusals exercise six
  exhausted three-attempt allocations, including UV failure after geometry
  success. VMA allocation count returns to baseline before each failed return
  and after each ownership scope. Mapped-span size/write/flush checked.
  Zero validation errors and warnings **including cleanup**.
- Injection is local to this test process. Successful Vulkan creation and all
  destruction use the real driver. This is not physical memory exhaustion.
  Link-only guards abort if legacy sparse diagnostics or renderer device-loss
  reporting are unexpectedly reached; neither tested allocation nor VMA is
  stubbed. Submission broker stays disabled.
- Explicit strict Vulkan 1.3 case: SKIPPED on this Intel API 1.2 device; no
  downgrade. Missing mandatory validation layer correctly SKIPPED (41), invalid
  CLI correctly failed (1). Core and synchronization validation are mandatory.
- Serial focused CTest: **49 cases, 36 PASS / 13 SKIPPED / 0 FAIL**, 8.24 s.
  Command uses the package's 47-case selection plus `mesh_buffer_vma` and
  `mesh_buffer_vma_vk12`, process-local `VK_LAYER_PATH` pointing at retained
  validation layers. An initial invocation could not find `ctest` in PATH;
  rerun used the explicit VS-bundled executable.
- All 44 existing generated SPIR-V modules remain byte-identical to retained
  S-053 references. Existing offline/SPIR-V and Intel runtime regressions PASS.

Retained build failures: first three boundary iterations preceded the real-VMA
target; callback C/Vulkan-Hpp mismatches (04/05), excessive recording dependencies
(06), missing broker destructor (07), missing device-loss reporter (08). The
allocation translation-unit split and explicit fail-fast diagnostic guards
resolve isolation/linking; focused VMA build09 PASS. Final full build10 emulator and focused targets PASS (exit0). No memory-exhaustion retry.

## Remaining gate

Next bounded local task: dedicated recoverable descriptor ownership around these
unsubmitted buffers. Require allocation/setup failure rollback, complete binding
1 geometry and optional binding2 UV, no payload binding0, exact-once pool/buffer
cleanup, zero/nonzero coverage and actual Intel allocation/update validation.
Then implement completion retention around the scheduler's current recording
tick and test delayed cleanup, shutdown and rejected/abandoned preparation before
any renderer activation. Global `DescriptorHeap` fatal behavior remains intact.

Native RX for this new boundary and Astro Bot: **NOT RUN**. Prior S-064 native
isolated compiler-bundle result remains valid and separate. A future consolidated
resource/submission milestone requires its own clean-source package/native gate;
this allocation-only patch does not require a per-patch transfer under D-028.
