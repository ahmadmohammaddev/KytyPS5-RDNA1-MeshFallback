# Context + implementation prompt — bounded mesh execution plan

## Your assignment and delivery boundary

Implement ONLY the next bounded local task after S-060: derive a pure execution
plan from compiler-proven output counts and the isolated compile bundle.
Deliver a complete, applyable Git diff for human review. Do NOT modify, stage,
commit, reset, clean, checkout, or apply patches in the owner's repository.
Work in your own separate disposable clone/copy with matching submodules. Do not
push anything, open a PR, run a game, or activate the fallback renderer.
This external-agent delivery restriction overrides the normal sequential-run /
commit requirements for your work. Do not continue to subsequent subtasks.

The owner will review and apply your patch themselves. This file is a handoff,
not authorization to run changes against their checkout.

## Repository and baseline

Owner workspace: D:/projects/KytyPS5-RDNA1-MeshFallback (reference only).
Branch: feature/rdna1-mesh-fallback.
Implementation baseline: commit3691b781 (`feat: compose isolated mesh compile bundle`).
Resolve and record its full40-character SHA in your clone before editing. Later
owner documentation-only commits may add this handoff; identify your exact base
and avoid reverting any such additions in the delivered diff.
Pinned upstream ancestry: u59-windows-20261024-int21 /744705e.
All14 recursive submodules matched at handoff preparation. No merges/rebases or
submodule revisions are part of this task. Preserve GPL/SPDX/upstream notices.

Read the ENTIRE root README.md, docs/RDNA1_MESH_FALLBACK_PLAN.md and applicable
AGENTS.md first. The living plan is the source of truth; retained old checkpoints
are historical. Read docs/RDNA1_COMPILE_BUNDLE_S060.md,
docs/RDNA1_ADAPTER_CONTRACT_S046.md, docs/RDNA1_RX_ACCEPTANCE_S059.md and
relevant source APIs before deciding on implementation details.

## Original requirements and intent

This is experimental C++/Vulkan emulator research targeting optional real RDNA1
mesh fallback. User wants small tested steps with evidence and no pushes. Native
mesh/default rendering must remain unchanged. No feature spoofing or silent loss
of geometry. Compilation, splash and rendered gameplay are distinct milestones.
Keep game assets, firmware, keys, dumps, proprietary shaders and large captures
outside Git. New code must be production-owned, with no includes from tests.
Reject unsupported input with stable diagnostics and no partial success result.

The latest concrete task was stated as: prove output counts and build the buffer /
binding plan for the triangle and zero-output cases before recording Vulkan commands.
This request delegates implementation and requests a Git diff; it does NOT request
a broader scheduler/renderer implementation.

## Verified state versus remaining work

S-059 standalone native RX acceptance:19/19 stages PASS,12 strict Vulkan1.3 native
RX5700XT selections and cleanups with zero validation errors/warnings. API1.4.308,
AMD25.6.1. Actual public frontend-generated compute, generated conventional vertex
bridge and native-emitted public pixel fixture passed full buffer, coverage,
per-channel UNORM and bounded RGBA32F tests. This is NOT integrated emulator/game
rendering. Astro Bot NOT RUN.

S-060: atomic isolated CPU-owned CompileBundle composes admission policy, existing
raw-IR compiler/private clone, fragment reflection/bridge and canonical key. It
owns compute/vertex/fragment modules, interface identity, key and geometry/UV sizes.
Specialization derives exclusively from the admitted Draw; a second supplied
specialization rejects. Failed paths return empty modules/key/ABI sizes. No Vulkan
handles, renderer call sites, allocation, native disk-cache mutation or draws.

S-060 focused two-job build PASS;43 CTests32 PASS/11 strict1.3 SKIPPED/0 FAIL(6.40s).
All44 generated SPIR-V modules byte-identical to earlier baseline. CPU bundle tests
use six actual public mesh frontend programs and independently authored plain/UV
fragment fixtures; production pixel emission is separately tested, not a new
bundle runtime claim. Package header staging was updated, not actually repackaged.

## Exact source anchors and likely changes

Paths below are repository-relative IN YOUR OWN CLONE. Confirm APIs; names for new
files are proposals, not claims that those files already exist.

1. src/graphics/shader/recompiler/MeshComputeLowering.h and .cpp
   - Existing Result ALREADY contains uint32_t vertices and primitives. Do not
     invent duplicate count inference or assume counts are missing. Trace Compiler::Run,
     allocation/export coverage/index proofs and failure paths to establish whether
     these fields form sufficient validated static proof, including0/0.
   - Reuse verified counts; strengthen metadata only if actually necessary. Do not
     infer counts from input draw.vertex_count, module filenames, guessed SPIR-V
     literals, or GPU/CPU readback. Failed lowering must not carry usable proof.
2. src/graphics/shader/recompiler/MeshComputeAdapter.h
   - Compile(const IR::Program&, const CompileRequest&) clones raw frontend IR,
     validates graph/wave, prepares, specializes, normalizes and calls Lower.
     Keep source preservation, full graph and rejection behavior. It already returns
     lowering Result; propagate proof rather than duplicating transformations.
3. src/graphics/host_gpu/renderer/meshFallbackBundle.h
   - CompileBundleResult currently omits the existing lowerer counts. Propagate
     validated counts/capacity/layout information atomically with successful modules.
     No usable plan/proof on failure. Default0 counts alone do not mean a valid
     zero-output program: preserve explicit successful-proof state.
4. Proposed src/graphics/host_gpu/renderer/meshFallbackExecutionPlan.h
   - Pure CPU execution-plan type/builder with checked arithmetic and stable failure
     diagnostics. No Vulkan handles, allocation, command recording or submission.
     You may place this in another well-justified production-owned file.
5. src/graphics/shader/recompiler/MeshComputeAbi.h
   - Authoritative geometry/UV constants; normally READ ONLY. Reuse offsets,
     guards, capacities and counts. Geometry1840 bytes, positions offset0,
     indices1032, counts1808, status1828;64 vertices/64 primitives/192 indices.
     Separate guarded UV544 bytes. Verify UV offsets in source; do not guess them.
6. src/graphics/host_gpu/renderer/meshFallbackPolicy.h
   - READ and preserve existing candidate restrictions/native preference. Admitted
     input draw has3 vertices, one instance, nonindexed, triangle list, single
     group and zero offsets. IMPORTANT: actual shader output may still be0/0.
7. src/graphics/shader/recompiler/MeshComputeCacheKey.h
   - Preserve backend/version/context/interface separation and full-byte equality.
     Decide/document whether added derived metadata needs a bundle schema bump.
     Never return a plan for a different bundle/context based only on a hash.
8. tests/MeshIrFrontendTests.cpp
   - Existing real raw frontend fixtures: constant, triangle, permuted, zero,
     UV and UV-zero; bundle/source-preservation/atomic failure tests already here.
     Extend to independently assert3/1 and0/0 proof/plan behavior, including UV.
9. tests/MeshIrComputeTests.cpp and a proposed focused execution-plan CPU test
   - Reuse lowerer rejection tests; add meaningful invalid/unproven/range cases.
     Do not write tests that merely repeat formulas from the implementation.
10. CMakeLists.txt
    - Existing mesh_ir_frontend_tests links lowerer, bridge, frontend, IR passes.
      Register new target/CTest under BUILD_TESTING/kyty_tests only if needed.
      Avoid unrelated cleanup of existing CMake issues.
11. tools/mesh-prototype/Build-Package.ps1
    - Stage any newly required public source/header dependencies. Existing package
      selection expects43 cases. If adding CTest cases, update inventory consistently.
      Do not build a transfer package merely for this CPU-only task.
12. docs/RDNA1_MESH_FALLBACK_PLAN.md, AGENTS.md and a new task report
    - Include proposed checkpoint/evidence/next-task updates in your diff. Keep all
      historical failures. Do not claim owner applied/committed your patch.

## Required behavior and acceptance

- Positive plan requires compiler-proven finite counts and a successful matching
  bundle. Counts respect capacity and triangle index arithmetic; no overflow in
  multiplication, offsets or end ranges. Bounds are relative to actual ABI blocks.
- Triangle fixture yields3 vertices/1 primitive/3 indices with correct draw ranges;
  zero fixture yields0/0/0 and explicitly no indexed draw, while preserving any
  required initialization/dispatch semantics. Never issue a fixed3-index draw
  just because the input policy admitted a three-vertex guest request.
- Represent geometry, input payload and optional UV allocations/bindings, position
  stride/index format and exact byte offsets/ranges using verified source contracts.
  Preserve guards/padding; do not pretend entire allocations are active vertices.
- State required compute-write to vertex/index consumer dependencies, UV vertex
  reads where enabled, and initialization/reuse requirements. Ground these in the
  existing harness and production scheduler source. Pure dependency descriptions
  may use project enums; if using Vulkan constants verify actual headers/docs.
  Do not pretend declaring a barrier executes it or proves GPU lifetime safety.
- Reject missing proof, invalid bundle, over-capacity/unproven counts, unsafe ranges,
  stale/inconsistent metadata and unsupported configurations, with empty plan /
  stable diagnostic. Avoid a public arbitrary boolean that can authorize a draw
  without validating count/layout invariants. Explicitly document trust boundaries.
- Independent tests cover zero and triangle, UV on/off, boundary capacity, integer
  overflow, offsets/end ranges, invalid/missing proof, count mismatch and failed
  compile. Repeated contexts preserve source IR and cannot reuse an incompatible plan.
- Keep shader output ABI and existing emitted modules unchanged unless a necessary
  change is explicitly justified and regression-tested. No blanket color tolerance
  or changes to S-057 portable paired acceptance in this task.

## Build, tests and evidence

Laptop is Windows11/Intel Iris Xe,16GB RAM; NOT RDNA1. Default2 jobs, retry1 only
on memory exhaustion, retain failed logs. No parallel build suites. Flags:
KYTY_BUILD_LAUNCHER=OFF, BUILD_TESTING=ON, KYTY_PGO_GENERATE=OFF,
KYTY_PGO_USE empty, KYTY_EMULATOR_IPO=OFF. Existing owner build directory is
_Build/windows-core; configure your own clone independently, do not reuse owner's
absolute-path CMake cache. Compiler clang-cl/LLD19.1.5, VS2022, CMake3.31.6,
Ninja1.12.1 were the previous host versions, not promises about your environment.

Before every dirty build retain full base SHA, diff, untracked-source copies/hash
inventory, submodule revisions, toolchain and flags outside the patch. Existing
_Build/p0-s012/capture.ps1 is an ignored LOCAL helper and may not exist in your clone;
it also fails on an empty tracked diff, so do not blindly depend on it.

Build focused frontend/IR/plan targets with --parallel2 (actual CLI: --parallel 2).
Run new CPU tests, mesh_ir_frontend_compatibility, lowerer positives/rejections,
policy/key/bridge/native-pixel offline regressions and SPIR-V validation. Existing
43-case package selection is in Build-Package.ps1. Run available Intel Vulkan
regressions with validation if present; missing strict1.3 is SKIPPED, never PASS.
Report six independent categories: compilation, unit, shader/SPIR-V, Vulkan,
RDNA1, Astro Bot. No new RX transfer or game test required for this offline task.
If tools unavailable, state NOT RUN and deliver the remaining meaningful work.

## Deliverables — mandatory

1. One complete UTF-8 Git patch, e.g. implementation.patch, including all new files,
   deletions, code/tests/CMake/docs changes; no logs/binaries/private paths/assets.
2. Exact base SHA and diffstat, concise explanation of proof source, plan layout,
   failure behavior, trust boundary and remaining scheduler/lifetime limitations.
3. Exact commands and actual test results, including failures/skips/not-run;
   evidence references available to reviewers. Do not fabricate owner-side evidence.
4. Validate patch application ONLY in a clean disposable copy of that exact base
   using git apply --check. Include new files in the patch (plain git diff omits
   untracked files). In your private clone only, you can stage them and use
   git diff --cached --binary --full-index --output=implementation.patch.
5. Do not apply your delivered patch to the owner repository, do not commit/push
   there, and do not start scheduler integration or the next phase. The owner will
   review and apply the complete diff themselves.
