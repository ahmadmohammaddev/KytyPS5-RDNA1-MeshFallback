# RDNA1 production adapter promotion — S-048 report

> Status: **Review repairs verified; user authorized commit.** The original uncommitted implementation and repair history are retained below. No push.
> Input commit: `a7fbbb97` (`feat: define bounded mesh fallback admission`, S-047).
> Date: 2026-10-29. Pre-build complete dirty snapshot: `_Build/p0-s012/p2-s048-pre-build-01/`
> (16 captured modified/deleted files + 7 untracked new source files, hash inventory retained).

## 1. Goal

Promote the previously tested research mesh-compute lowering into a **production-owned,
isolated compiler adapter** under `src/graphics/shader/recompiler/`, so that:

- production code has **no dependency on `tests/`** sources,
- the **actual pre-resource IR preparation is shared** instead of being a drifting
  hand-copied test-local sequence,
- byte-for-byte generated SPIR-V modules and the existing output ABI are preserved,
- the default native renderer path stays untouched and the adapter remains unwired.

## 2. What moved where (all moved verbatim unless stated)

| New production file | Content |
| --- | --- |
| `src/graphics/shader/recompiler/MeshComputeAbi.h` | The full mesh-output contract ABI (1840-byte geometry blob vocabulary + separate 544-byte UV buffer, static asserts incl. `kUvWords == kUvStatusWord + 3`), previously `tests/research/MeshIrComputeLowering.h` P2.1 bits |
| `src/graphics/shader/recompiler/MeshComputeLowering.h/.cpp` | The bounded SPIR-V GLCompute lowerer (`Lower(const Input&)`), namespace `Libs::Graphics::ShaderRecompiler::MeshCompute`; ABI referenced through a namespace alias `MeshOutput = Kyty::MeshOutput` |
| `src/graphics/shader/recompiler/MeshComputeSpecialization.h` | `SpecializeNonIndexedDraw(program, NonIndexedDraw, error&)` moved verbatim |
| `src/graphics/shader/recompiler/MeshComputeAdapter.h` | **New** header-only entry `Compile(IR::Program&, const CompileRequest&)` (see architecture below), `[[nodiscard]]`, returns a `Result` whose failure path has **empty binary + assembly** and a stable diagnostic string |
| `src/graphics/shader/recompiler/ir/passes/PreparePreResource.h/.cpp` | **New shared** `IR::PreparePreResource(Program&)`: the exact pass sequence previously duplicated in `ShaderRecompiler.cpp` — SSA rewrite, constant propagation, control-flow identity resolution, identity removal, dead-code elimination, gated `FoldLaneMasks` loop (specifier off by default), `EliminateReadLane` + its follow-up passes; returns `ReadLaneStats` for caller logging |
| `src/graphics/shader/recompiler/ShaderRecompiler.cpp` | Now calls `IR::PreparePreResource(ir)` instead of listing those passes inline; keeps its own `LOGF("read-lane elimination: reads=%u")` |

Test-side changes:

- `tests/MeshOutputContract.h` replaced by a forwarding shim to `MeshComputeAbi.h`
  (include sites preserved; layout/constants/writer semantics unchanged).
- Deleted `tests/research/MeshIrComputeLowering.cpp/.h` and
  `tests/research/MeshIrFrontendSpecialization.h`.
  `tests/research/MeshIrUvOracle.h` is kept — it is an **independent test-side oracle**,
  intentionally not production code.
- `tests/MeshIrComputeTests.cpp` / `tests/MeshIrFrontendTests.cpp` now consume the
  production sources through `namespace R = Libs::Graphics::ShaderRecompiler::MeshCompute;`.
  The frontend test's `Simplify()` calls `IR::PreparePreResource(program)` directly and an
  added assertion recompiles through the adapter and compares
  `result.binary`/`result.assembly` byte-for-byte (PASS, run below).
- `CMakeLists.txt`: `mesh_ir_compute_tests` and friends compile
  `${KYTY_SOURCE_DIR}/graphics/shader/recompiler/MeshComputeLowering.cpp`; the frontend
  target also adds `ir/passes/PreparePreResource.cpp`. The production `kyty_emulator`
  target needs no change — its existing `graphics/shader/recompiler/*.cpp|.h` glob already
  picks up the moved sources.
- `tools/mesh-prototype/Build-Package.ps1` payload staging paths updated to the new src
  locations. **No package was built or transferred.**

## 3. Adapter architecture (`MeshComputeAdapter.h`)

`Compile(program, request)` performs, in order:

1. **Stage gate** — anything other than `ShaderType::Mesh` is rejected immediately with
   `"unsupported stage; the adapter lowers complete mesh programs only"`; outputs stay empty.
2. `IR::PreparePreResource(program)` — the shared production preparation prefix.
3. **Optional bounded specialization** — when `request.specialization` (a
   caller-owned `NonIndexedDraw`) is supplied, `SpecializeNonIndexedDraw` bakes the
   direct non-indexed three-vertex/single-group draw into the NGG prologue parameter
   values. Failure returns the stable error with empty outputs and **no graph mutation**
   (the specialization is atomically out-of-place). Passing the same request twice is
   harmless; re-running the shared preparation afterwards removes the now-inactive
   indexed reads through ordinary propagation/DCE.
4. `Lower(input)` over the **complete retained block graph** (`program.blocks`,
   `export_info`, `block_info`), `request.mesh`, and the opt-in UV switch with the
   544-byte capacity from the production ABI.

Semantics preserved from S-036…S-043 evidence:

- unsupported programs produce a stable diagnostic with **empty binary/assembly**,
  never a partial module;
- the shared preparation is always the production function, never a test copy;
- `constexpr`/static_assert rejection of out-of-range UV capacity remains compile-time;
- nothing in the native/default renderer path is invoked, changed, or consulted.

Adapter call sites: **none** in production. The header is currently consumed only by
`tests/MeshIrFrontendTests.cpp` (equivalence check). Wiring the renderer is a later,
separate gate (P5-adjacent; cache/bridge/scheduler first).

## 4. Build (all builds dirty-source captured; 2 jobs; no PGO/IPO)

| Attempt | Result | Notes |
| --- | --- | --- |
| build-01 | FAIL | `static_assert` arithmetic typo `kUvWords == kUvCountWord + 3` → corrected to `kUvStatusWord + 3` (136 == 133+3). Log retained (`_Build/p2-s048/build-01.log`) |
| build-02 | FAIL | `using Kyty::MeshOutput;` (illegal namespace-alias form) → `namespace MeshOutput = Kyty::MeshOutput;`. Log retained (`build-02.log`) |
| build-03 (+ `kyty_emulator`) | **PASS** | Focused targets + incremental `kyty_emulator` build and **link** PASS, exit 0; only pre-existing upstream warnings (`shader.h` `-Winvalid-offsetof`, `codecvt`, `getenv`, `-Wreturn-type` in `pipelineCache.cpp`, `-Winvalid-noreturn`). No warnings from the new adapter sources |

The `[[nodiscard]]` on the shared `PreparePreResource` was removed (its stats return is
auxiliary); `EliminateReadLane` keeps its `[[nodiscard]]`. An accidental blank-line
removal in `ReadLaneElimination.h` was reverted; that file is byte-identical to HEAD.

## 5. Byte-for-byte module compatibility

Baseline: 33 generated modules emitted by the **pre-change** binaries into
`_Build/p2-s048/baseline/modules/` with a SHA-256 inventory. After the change the same
emitters (`mesh_ir_compute_tests --emit / --emit-uv`,
`mesh_ir_frontend_tests --emit`) were re-run into `_Build/p2-s048/after/modules/`
(all three emissions PASS).

Result: **33/33 baseline modules byte-identical** (SHA-256, 0 mismatches).
The run also emitted 67 additional newer test-side outputs (UV mutants, human-readable
`*.spvasm`, prepared/raw IR text snapshots) that are not part of the 33-module baseline;
they are consistent with the retained fixtures and are covered by the passing tests that
create and internally verify them.

## 6. Test results

Serial focused suite (`ctest`, build tree `_Build/windows-core`):

| Group | Result |
| --- | --- |
| `mesh_fallback_policy`, `mesh_output_contract`, `mesh_ir_compute_positive/rejections`, `mesh_ir_uv_positive/rejections`, `mesh_ir_frontend_compatibility` | **7 PASS, 0 FAIL** (`frontend_compatibility` includes the new adapter-equivalence assertion: PASS) |
| Intel Vulkan 1.2 auxiliary runtime `mesh_ir_runtime_vk12`, `mesh_ir_add_runtime_vk12`, `mesh_ir_draw_vk12`, `mesh_ir_textured_draw_vk12` | **4 PASS** (with process-local `VK_LAYER_PATH=_Build\p4-s013\tools\validationlayers`; zero-error core+synchronization gate unchanged) |
| Strict Vulkan 1.3 runtime `mesh_ir_runtime`, `mesh_ir_add_runtime`, `mesh_ir_draw`, `mesh_ir_textured_draw` | **4 SKIPPED** — the Intel device only implements API 1.2.172; strict 1.3 remains RX-only |

Procedural note kept for provenance: the **first** runtime pass ran without
`VK_LAYER_PATH` and correctly SKIPPED all four auxiliary cases with
*"VK_LAYER_KHRONOS_validation is not installed"* (exit 41, never a false pass). A local
process-local layer directory discovered under `_Build\p4-s013\tools\validationlayers`
restored the auxiliary gate exactly as in earlier sessions. Global configuration was
never modified.

## 7. Verification-category summary

| Category | Result |
| --- | --- |
| Compilation | PASS — focused targets + `kyty_emulator` build/link (2 jobs) |
| Unit/offline tests | PASS — 7 focused cases 0 FAIL |
| Shader/SPIR-V validation | PASS — 33/33 byte-identical modules; runtime shape+validation checks PASS inside auxiliary cases |
| Vulkan integration | PASS (Intel auxiliary 1.2 cases only, zero-error-gated); strict 1.3 SKIPPED — separate evidence class per D-019 |
| RDNA1 hardware tests | NOT RUN — no package, no RX request (D-028 batching; policy-only move) |
| Astro Bot compatibility | NOT RUN |

## 8. Remaining gaps / limitations

1. **Uncommitted.** Everything awaits explicit user review; on approval the tree commits
   as the S-048 change set.
2. **No production call site.** The adapter compiles and links inside `kyty_emulator`
   through the moved `.cpp` files, but nothing calls `Compile` yet; production cache,
   fragment bridge and draw scheduling integration are separate future gates.
3. Same research-scope limits as before: complete linear block chains only, explicit
   non-indexed specialization only, bounded arithmetic/UV export, strict fail-closed
   diagnostics. Indexed/dynamic/CFG/LDS/resource-dependent semantics remain unsupported
   **by design** and are not hidden.
4. Strict Vulkan 1.3 execution and any future generated-adapter GPU run remain RX-only;
   Intel auxiliary results are never treated as RDNA1 evidence.
5. `tools/mesh-prototype/Build-Package.ps1` paths were updated but the new layout has not
   been through a package build; the next consolidated package (when requested) must
   re-verify staging, manifests and relocation.

## Review repair — supersedes initial adapter/provenance claims above

The first review reproduced stale binaries after changing draw context, wave64/wave32 acceptance, and null-block access violation. Initial adapter equivalence only tested already prepared IR. Initial pre-build-01 does not capture the final repaired ABI/lowerer; that historical provenance gap is retained, not retroactively closed.

Repair: Compile now accepts const raw unsealed frontend IR, validates wave/linear graph/terminators/arity/parents before preparation, clones owned IR using existing IR::CloneProgram and specializes the private clone. Prepared inputs reject instead of silently using baked context. Follow-up normalization preserves SSA without repeating RewriteToSsa. Native pass order unchanged. Raw frontend tests compare direct sequence, repeat/context-changed vs fresh compilation, prove source unchanged and check null/duplicate/foreign blocks, missing metadata, stage/wave mismatch and indexed request rejection. No commit/push; original work preserved.

Full repair snapshots under `_Build/p0-s012/p2-s048-fix-pre-build-01/`, `-02/` and final `-03/`; first repair helper stopped on UTF-8 decoding after partial edits, retained snapshot/build and corrected helper. Subsequent final build/results recorded below; no claim that older failed builds were captured after the fact.

### Final repair verification (uncommitted)

Final two-job build including kyty_emulator and runtime/generated shader targets PASS.32 serial focused cases23 PASS/9 strict1.3 SKIPPED/0 FAIL(4.35s), with process-local validation layers.33/33 baseline modules byte-identical; inventory `_Build/p2-s048-fix/binary-comparison.json`. Full final code hashes match pre-build-03 snapshot. No RX/package/game execution or commit/push.

Independent original probes now show changed-context reused source matches fresh compilation and differs from old context; wave mismatch rejects with empty diagnostic result; null block returns diagnostic. The initial fixed-code null probe returned the diagnostic then crashed in the pre-existing Program destructor because the probe left nullptr in its caller-owned block list; restored that intentionally injected corruption before destruction (as the regression test does) and reran successfully. No change to upstream destruction behavior or hidden mutation of caller input. Logs retained, final probe results `_Build/p2-s048-fix/probes.json`.

Six categories: Compilation PASS focused+emulator; Unit PASS; SPIR-V PASS strict offline and byte-equivalence; Vulkan PASS Intel1.2 auxiliary/strict1.3 SKIPPED; new RDNA1 and Astro Bot NOT RUN. Native preparation sequence retains its original single SSA rewrite; adapter uses SSA-preserving normalization after specialization. Uncommitted and pending review; next cache/bridge/scheduler phase NOT STARTED.

### Commit authorization

User explicitly requested committing the reviewed implementation and fixes. Final diff check PASS; retained emulator build,23 PASS/9 SKIPPED/0 FAIL and33 exact modules verified. Code unchanged since final build snapshot. Commit includes production adapter/ABI/lowering/shared preparation, test migration/regressions, package source-path updates and synchronized documentation. Package rebuild/native RX/game remain NOT RUN; next single task remains cache/bridge/scheduler preparation after this compiler boundary, with no renderer activation in S-048.
