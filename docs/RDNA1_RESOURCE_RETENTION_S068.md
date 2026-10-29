# S-068 — Recording admission and completion retention

Input `7c893a25`, branch `feature/rdna1-mesh-fallback`. D-033 continuation after
S-066/067 allocation and descriptor rollback. No renderer fallback activation.

## Contract and implementation

`CommandScheduler::TryRetainForRecording(expected_tick, callback&)` admits only
an active valid current recording, on the recording producer, outside a deferred
callback/worker, with operation state Open and an exact tick match. It never
invokes cleanup inline, flushes, waits or records commands. Queue-allocation
failure restores the caller's callback; PendingOperation's noexcept move is
asserted. Accepted callbacks use the existing normal completed-tick queue, and
preserve queued-submission completion exactly as existing DeferOperation does.
Native DeferOperation behavior is unchanged. Draining/closed registration rejects.

`RetainBeforeUse` consumes an unsubmitted complete owner and transfers it into
a completion callback. Rejected/empty/host-allocation failure cannot publish a
usable owner; cleanup is safe because admission precedes the first GPU reference.
The admission adapter must have a strong exception guarantee and never run the
callback inline. `RetainResourcesBeforeUse` composes this with the real scheduler
and returns non-owning geometry/UV/set/layout views and the recording tick.

**Caller constraints:** serialized recording producer, no concurrent shutdown,
all final compute AND graphics uses in that SAME recording/queue before submit.
No reuse in a later recording, cross-queue use or cancellation after recording
references is supported. The tick is part of the view contract. The callback
must outlive every use; device/context must outlive callback drain. This is an
isolated boundary, not an activated renderer path or arbitrary-fence proof.

## Actual verification

Full dirty diffs/untracked raw copies/hashes/cache/submodules captured under
`_Build/p0-s012/p2-s068-pre-build-01/` through `-05/`; final built source is05.
Logs, negative artifacts and mutation source under ignored `_Build/p2-s068/`.
Two jobs, no PGO/IPO or memory retry. Full emulator and shader_cfg consumers
compile/link the real scheduler method and resource adapter. Actual
CommandScheduler **runtime is NOT RUN**; controlled completion and a real Vulkan
fence adapter below must not be relabeled as actual scheduler execution.

- Controlled CPU tests use the production retention helper: producer ownership
  transfer, incomplete/different ticks, two simultaneously live resources,
  wrong/closed admission, repeated completion, empty/abandoned recording and
  admission bad_alloc. Cleanup counts and no reuse while pending checked.
  A separately compiled missing-owner-capture mutant correctly FAILS.
- Actual Intel Iris Xe API1.2.172 auxiliary executes five public frontend modules:
  triangle/permuted/zero geometry, UV triangle/zero. Modules are validated offline
  for the explicitly selected API before device discovery, loaded beside the
  executable only. Dedicated real Buffer/VMA+descriptor owners feed compute.
- A real unsignalled timeline semaphore blocks each submitted command buffer;
  its fence is NotReady and VMA allocations remain live after producer ownership
  is gone. After host release, compute writes are copied to a separate Download
  buffer. Complete460-word geometry and optional136-word UV readbacks match an
  independent literal oracle, including guards/unused poison/counts/status.
- Upload resources have additional test-only TRANSFER_SRC usage for readback.
  Download readback is poisoned and owned by cleanup that waits before releasing
  it on every post-submit early return. Dependencies are host-to-compute/transfer,
  compute-write-to-transfer-read, transfer-write-to-host-read, fence and required
  invalidate. Readback waits are test-only; production retention introduces none.
- Fence completion alone leaves ownership alive. Explicit completed-callback
  drain destroys resources once, leaving only the test readback; its later scope
  cleanup returns VMA allocations to zero. This proves compute plus final copy
  use in one real submission, **not a graphics draw or actual scheduler runtime**.
- Existing allocation/descriptor fault cases remain included: six exhausted
  retry failures and16 descriptor scenarios. Total44 native buffers created and
  destroyed,18 injected refusals. Zero validation errors/warnings through cleanup.
- A relocated executable passes. Replacing triangle by a valid zero module
  correctly FAILS the full-buffer oracle with zero validation errors. Missing
  strict module FAILS before device discovery; RX guard rejects Intel. Strict1.3
  SKIPPED; auxiliary1.2 remains separate. No build-tree module fallback.

Retain failures: build02 failed on Windows MemoryBarrier macro collision, fixed
with the local undef. First GPU attempt exited321 at the legacy Download-only
Invalidate guard because the test tried to read Upload directly; corrected by
the separate GPU-copy Download fixture, without weakening Buffer invariants.
Build01 passed the boundary; builds03/04 passed; final05 status and serial suite
are appended at session completion. An exploratory full-harness scheduler test
addition was preserved as `deferred-scheduler-regression.patch` and removed after
inspection, not committed or claimed tested: that harness requires unrelated
barycentric/image-atomic/workgroup features and its executable was unavailable.
No existing user changes were removed. Automatic review initially refused its
removal; preserving and verifying this session-only diff allowed the safe retry.

## Remaining work / next gate

Production allocation/descriptor preparation and lifetime admission now exist
in isolation. Actual scheduler execution, command recording/state restoration,
cache wiring, synthetic guest submission and game integration remain open.
Do not activate the fallback from this evidence. No physical OOM, guaranteed
noncoherent allocation, concurrent shutdown/recording or cross-queue guarantee.

Next single task: clean consolidated resource milestone package, including
S-064 bundle regressions, CPU allocation/retention test and real resource
allocation/descriptor/dispatch/completion test. Verify clean identity/checksums,
relocation, exact buffers/images, strict skip and RX guard/negative outcomes.
Then request one native RX strict1.3 resource milestone run and stop under
D-028/D-033 before recording integration. New native resource execution is NOT RUN;
prior S-064 bundle native acceptance is retained. Astro Bot remains NOT RUN.

Final build05 emulator/focused+link PASS exit0. Final serial49 cases36PASS/13SKIPPED/0FAIL(7.39s); actual VMA summary44 created/44 destroyed/18 refusals, zero errors/warnings through cleanup. All44 generated modules byte-identical; final12-file source manifest matches snapshot05. Six categories: compilationPASS; unitPASS; shader/SPIR-V PASS; VulkanPASS Intel1.2 auxiliary and strict1.3SKIPPED; new RDNA1NOT RUN; Astro BotNOT RUN.
