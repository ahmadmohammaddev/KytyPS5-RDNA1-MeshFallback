# Instructions for Codex — KytyPS5 RDNA1 Mesh Fallback

This project is experimental C++/Vulkan emulator research, **not** a finished patch. The source of truth across sessions is `docs/RDNA1_MESH_FALLBACK_PLAN.md`.

## Before every task
1. Read the ENTIRE `docs/RDNA1_MESH_FALLBACK_PLAN.md` and root `README.md` (plus relevant upstream docs).
2. Run `git status`, `git log -1 --oneline`, `git branch --show-current`, and inspect submodule state.
3. Summarize what is verified vs merely planned. Select **only one** subtask of the active phase.
4. Verify exact file names, APIs, code paths and build flags in the checked-out tree; never guess.

## Development rules
- Preserve GPL license, third-party notices, and upstream credit.
- Pinned starting tag: `u59-windows-20261024-int21` (`744705e` short); work on `feature/rdna1-mesh-fallback`. No silent upstream merges/rebases.
- No game assets, firmware, keys, dumps, compiled proprietary shaders or user-specific paths in git. No DRM circumvention work.
- Use isolated and opt-in fallback code; never spoof a missing Vulkan feature or silently skip required geometry.
- Favor small testable patches; keep working native mesh rendering unchanged by default.
- A passing compile, splash screen, and GPU-rendered gameplay are three **different** milestones.
- Build and run automated tests where possible; state what was *not* run. Use validation tooling for SPIR-V, Vulkan and synchronization.
- Do not assume Mesa Lavapipe results imply native RX 5700 XT success.

## Two-machine workflow

- **Development host (user-reported):** Huawei laptop, Windows 11 Home x64, Intel Core i5-1135G7 (4 cores / 8 threads), 16 GB system RAM, Intel Iris Xe integrated graphics. Use for Codex, editing, compilation, static analysis and CPU/offline tests. Never assume this laptop has RDNA1 hardware.
- **Hardware test target (user-reported):** Windows x64 desktop, Ryzen 5 7500F, Radeon RX 5700 XT 8 GB, 32 GB DDR5 RAM. Astro Bot stays installed locally on this PC.
- Default laptop builds to **2 jobs** (`CMAKE_BUILD_PARALLEL_LEVEL=2` or `cmake --build ... --parallel 2`). On memory exhaustion retain the failure log, stop concurrent builds and retry with **1 job**. Prefer incremental builds, separate build directories and one build/test suite at a time.
- **No PGO during initial development:** use `KYTY_PGO_GENERATE=OFF` and an empty `KYTY_PGO_USE`. Keep `KYTY_EMULATOR_IPO=OFF` initially to reduce link pressure. Upstream's optimized release recipe is not the initial laptop baseline.
- Optional Qt-free core/CLI builds are supported by root CMake: `KYTY_BUILD_LAUNCHER=OFF`, targets `kyty_emulator` and (with `BUILD_TESTING=ON`) `kyty_tests`. This removes the Qt launcher; it does not guarantee GPU-free execution. Reverify source support when updating build instructions.
- Report six independent categories: **Compilation; Unit tests; Shader compilation and SPIR-V validation; Vulkan integration tests; RDNA1 hardware tests; Astro Bot compatibility tests**. Inspect CTest cases before classifying them: `kyty_tests` includes GPU-related tests as well as CPU tests.
- Intel Iris Xe, hosted CI and software ICD results cannot complete RDNA1 milestones. RX 5700 XT tests must independently identify device/driver; compilation, splash, menu and gameplay remain distinct outcomes.
- Every build records its full Git commit ID, dirty state, submodule revisions, toolchain and flags. Dirty builds also need a retained local diff/untracked-source inventory; transferable packages require clean committed source.
- After a stable baseline, implement and validate Windows x64 packaging with commit-named artifacts, manifest and SHA-256 checksums, then reproduce deployment and rollback on the desktop. Future GitHub Actions must reuse that process; hosted CI is not an RX 5700 XT test.
- After every hardware test, append an experiment with commit/package identity, commands/configuration, device/driver, duration, result, logs, relevant screenshots, performance metrics and artifact locations. Record unavailable metrics and reasons, including early failures.
- Keep game files, firmware, keys, dumps, proprietary shader binaries and large shader caches outside Git. Keep bulky/private captures outside the checkout; commit sanitized summaries and shareable evidence references. Preserve package license notices and upstream credit.
- Keep this file and the living plan synchronized as workflow changes. Detailed build, deployment and evidence gates live in plan sections 2.1–2.5.

## Plan is intentionally editable
You MAY reprioritize, split, merge, or remove future milestones based on evidence. If so, update the live plan and append a reason to Decision Register and Change History. Never rewrite experiment/session history to hide failures or declare an unverified milestone completed.

## At end of EVERY session, mandatory
1. Update live plan checkpoint, evidence, phase status, experiment/decision log when relevant, and append a detailed session handoff entry.
2. Give the exact next single task with a testable acceptance gate.
3. Commit the .md updates alongside related code, or explicitly document why a commit was impossible.
4. In your response state branch, SHA, files changed, commands/tests + actual results, blockers, and next step.
5. **Stop** after the selected subtask; do not start the next phase automatically.

### RX test cadence (D-028, supersedes per-subtask transfer expectations)

Develop and validate a coherent group of related changes locally before requesting a desktop transfer. Do not package or request an RX rerun for every small patch or opcode. Keep individual changes reviewable and testable, with focused builds, CPU/offline/SPIR-V checks and available Intel Vulkan tests.

Request native RX testing when a defined milestone acceptance gate is ready, when changed behavior depends on AMD/device/driver characteristics, or when a regression cannot be resolved locally. A pending RX gate does not block further related development that can be validated locally; mark it pending and keep the accumulated scope explicit. Before transfer, consolidate the relevant regressions in one clean committed-source package with manifest/checksums and relocation verification.

Intel Vulkan 1.2 and native RX strict Vulkan 1.3 evidence remain distinct. Never mark an RX gate complete from local results. Report PASS, FAIL, SKIPPED and NOT RUN accurately, retain failures and all provenance requirements. The one-selected-subtask rule still bounds each development session; it does not require a hardware handoff after every session.

## Current next task (checkpoint S-031)
Generated-IR triangle/permuted/zero now feed conventional indexed drawing in the standalone harness. Intel Vulkan 1.2 auxiliary: all buffers/pixels exact, zero validation errors/warnings through cleanup; 22 focused cases 16 PASS/6 strict SKIPPED/0 FAIL (2.92 s). Valid wrong fragment correctly FAILS 496 pixels; missing strict module FAILS before device discovery. Report `docs/RDNA1_IR_DRAW_S031.md`; complete dirty build snapshots and logs retained. Native RX draw gate is pending and batched under D-028; no new transfer package or game evidence.

Next single local task (D-029): offline lowering of one bounded per-vertex float2 UV parameter export into the existing separate 544-byte guarded UV ABI. Gate explicit metadata/type/mask/capacity rejection, independent layout/output coverage tests and Vulkan 1.3 SPIR-V validation; preserve the position/index ABI and native/default renderer. Runtime textured integration and consolidated strict RX verification follow as distinct gates. Commit and stop; no push.

## Retained checkpoint S-030
S-030 approved batching related development before RX tests (D-028); avoid per-patch hardware handoffs.

## Retained checkpoint S-029
Restricted IR arithmetic and wrap PASS on native RX 5700 XT strict Vulkan 1.3: full 460-word buffers, zero errors/warnings through cleanup; six stages PASS. Source `3fd0fdd507a91759a0ca41be4150dff94d5e6485`, build `20261028T215237152Z`. Raw evidence `_Build/p3-s029/`, report `docs/RDNA1_RX_ADD_S029.md`. No game or emulator integration claim.

Next single task: connect restricted IR-generated triangle to the existing isolated conventional indexed offscreen draw. Require independent full-buffer and complete pixel expectations, correct compute-to-vertex/index synchronization, offline SPIR-V validation, focused regression tests and zero validation errors including cleanup. Strict RX execution is a separate hardware gate; preserve native/default path, capture dirty-source builds, commit and stop. No push.

## Retained checkpoint S-028
Clean package source `3fd0fdd507a91759a0ca41be4150dff94d5e6485`, build `20261028T215237152Z`, SHA-256 `FBC0FA54BA000B2FA9952BE8982A1C86BE177D4F20E997F7F5A2FAF031351989`. 20-case suite 15 PASS/5 SKIPPED; relocated Intel auxiliary add/wrap PASS, strict SKIPPED, RX guard rejects Intel. Next: transfer ZIP+sidecar and run `Run-IR-Add.cmd -IsolateImplicitLayers` on RX; require two exact full buffers and zero errors/warnings through cleanup; retain all logs and result.json. No push.

## Retained S-028 preparation
Arithmetic runner, launcher, generated module packaging and two CTests implemented; 4 IR tests PASS and 2 strict SKIPPED on Intel, build PASS. Commit source then produce clean package, verify relocation/integrity and Intel auxiliary; RX arithmetic remains NOT RUN.

## Retained checkpoint S-027
P3.3 generated arithmetic and wrap modules PASS exact full buffers on Intel Vulkan 1.2 auxiliary with zero validation errors/warnings including cleanup; isolated valid wrong-module mutation correctly FAILS one word. Strict RX arithmetic NOT RUN. First default-layer attempt SKIPPED; process-local VK_LAYER_PATH retry passed. Two-job build PASS after sandbox regeneration failure. Next single task: clean committed-source portable arithmetic package, CTest/runner coverage and relocation/integrity verification, followed by independent strict RX evidence. Keep Vulkan 1.2 and 1.3 evidence separate; no production integration or push.

## Retained checkpoint S-026
P3.3 offline fixtures validate nonzero and wrapping additions, reject unsafe packed index, and pass SPIR-V validation; 2 focused tests PASS and 2 Vulkan tests SKIPPED. Host arithmetic checks are not GPU output verification. Next single task: execute generated arithmetic and wrap modules with independently specified full-buffer oracles, test a valid wrong-operation mutation, retain validation through cleanup and distinguish Intel auxiliary from RX strict evidence. Capture complete dirty builds; no production integration or push.

## Retained checkpoint S-025
P3.3 IN_PROGRESS: isolated `IAdd32` lowering added; focused build PASS, two offline tests PASS and two runtime cases SKIPPED. The zero-add fixture and coverage/type rejection do not yet prove nonzero arithmetic or 32-bit wraparound. Next single task: add independent arithmetic/wraparound and unsafe-index/coverage fixtures, validate SPIR-V and rerun focused tests. Preserve complete dirty-source provenance; GPU and RX execution remain separate. See S-025 in the living plan.

## Retained checkpoint S-024
P3.2 narrow IR-generated triangle/permuted/zero strict Vulkan 1.3 runs PASS on native RX 5700 XT (AMD 25.6.1, API 1.4.308): exact full 1840-byte buffers and zero validation errors/warnings including cleanup. Six desktop stages PASS. Intel Vulkan 1.2 auxiliary remains separate. Evidence/report: `docs/RDNA1_RX_IR_S024.md`, ignored `_Build/p3-s024/`. This does not prove full NGG, emulator integration or Astro Bot.

Next single task P3.3: extend the isolated research lowerer with bounded `IAdd32` lane-dependent expressions. Acceptance: source-backed semantics, finite coverage/index safety proofs, explicit rejection of unsupported or unproven cases, independent positive/negative host tests, Vulkan 1.3 SPIR-V validation and focused regressions with complete dirty-build provenance. Keep native/default path unchanged; runtime/RX validation is a distinct subsequent gate. Commit and stop; no push.

### Retained S-015 package checkpoint
Standalone portable package DONE locally; clean-source ZIP 681,008 bytes, SHA-256 `9F36E3774F9C99B6E73F95D10216F8AE94E65ECD2A80E8216422360908F2A897`, under ignored `_Build/mesh-prototype-packages/` with sidecar. See `docs/RDNA1_MESH_PACKAGE_S015.md`. Six-case suite 5 PASS/1 strict-1.3 SKIPPED; eight relocation/integrity probes match expected results; auxiliary Intel 1.2 nine blobs match with zero validation errors/warnings after cleanup. No absolute build-path fallback.

### Retained S-014 checkpoint
All eight S-013 findings fixed in the existing P4.1 implementation, preserving prior work. Focused 2-job builds PASS; both public shader variants compile/validate. Final six-case serial suite: 5 PASS, 1 SKIPPED, 0 failures (0.99 s). Actual native Intel Iris Xe/Vulkan 1.2 auxiliary run completes nine exact 460-word scenarios (eight dispatches plus Pending initialization), including two true same-buffer overflow recoveries, with zero core/synchronization validation errors or warnings after cleanup. Host fixture and valid-but-wrong-binding negative-checker regressions PASS. Missing-layer tests correctly SKIP both runtime cases. Full dirty snapshots/hashes/logs retained. See `docs/RDNA1_P41_RESULTS_S014.md` and living plan D-019/E-014/S-014.

Strict `mesh_compute_proto` (`--run`, Vulkan 1.3) remains SKIPPED: this Intel driver exposes only API 1.2.172. `mesh_compute_proto_vk12` (`--run-vulkan12`) is explicit auxiliary evidence and never closes the 1.3 gate. Current registered totals: 68 with passing executions, 1 failed-only deferred timer case, 1 skipped-only strict 1.3 case, 361 never run, 431 total. Noncoherent runtime path, RDNA1, Astro Bot and deployment/package gates remain unverified.

Next single task: continue P4.1 by executing strict Vulkan 1.3 dispatch/readback on a qualifying device, preferably the RX 5700 XT desktop. Use commit-identified source/build provenance and validation layers; require all nine exact blob comparisons, zero validation errors including cleanup, native device/driver/ICD identification and retained commands/logs. Follow existing clean-build/deployment requirements; the dirty-build laptop binary is not a transfer package. No renderer integration, guest translation, next phase or push.

### Retained S-012 contract checkpoint
P2.1 and S-011 review fixes complete: `tests/MeshOutputContractTests.cpp` uses Pending=`0xFFFFFFFF`, checks stored Ok on every success including overflow recovery, and compares every non-status word against nonzero snapshots after rejected requests. Full dirty patches/raw source copies/hashes and submodule/toolchain/cache identity were retained before compilation. Focused 2-job build PASS; serial CTest 1/1 PASS; five temporary mutations correctly rejected (missing success store, recovery alone, vertex/index/combined zero stores). Layout unchanged: offsets 0/1032/1808/1828, 1840 bytes, eight `0xC0DEFEED` guards, 64 vertices/64 primitives/192 indices. See living plan S-012/E-012/D-018. Historical S-010 snapshot gap remains documented; current evidence is complete.

Historical S-012 totals: 64 distinct passing registered cases, 1 failed-only timer case, 361 unexecuted. CondWaitUntil and generic inventory remain deferred under D-016. Existing compute mesh-indirect conversion still needs native mesh shaders. S-014 adds separate auxiliary Vulkan evidence above; no RDNA1 or Astro Bot milestone is complete, and host final-state checks alone do not establish GPU ordering/visibility.

Next single task: P4.1 synthetic Vulkan compute prototype of the hardened contract. Initialize Pending, author a public compute shader writing three float4 positions, indices 0/1/2, counts 3/1/reserved 0 and final stored Ok; preserve guards and unused words. Acceptance: SPIR-V validated offline for Vulkan 1.3; one-workgroup dispatch with core + synchronization validation; explicit dependencies/fence and required mapped-memory handling; full-blob host readback compared exactly; record physical device/driver/ICD and provenance. Intel results are Intel only; software ICD results are software only; unavailable Vulkan is a skip, never a pass. No emulator/renderer source change, guest translation or timer change; commit and stop. Ready for this isolated task under D-016; no next-phase work performed in S-012.
