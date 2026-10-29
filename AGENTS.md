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

### Authorized sequential run (D-033)
The user explicitly requests related steps one after another with a commit for each, until a native RX test is unavoidable. During this authorized run, continue after each bounded, tested and documented subtask rather than stopping automatically. At a necessary RX-only gate, first complete a verified clean consolidated ZIP, provide one experiment command and stop. This supersedes the default per-subtask stop for this run; D-028 batching, accurate statuses, source provenance and no-push remain mandatory. Later user steering/cancellation takes precedence.

## Retained next task (checkpoint S-036)
Actual authored ISA decoder/mesh frontend plus normal pre-resource SSA/constant/identity/dead-code/read-lane preparation passes and validates. Both complete probes retain two IR blocks; research lowerer correctly rejects the structural limit with empty output. Five offline tests PASS, build PASS. Source-backed live vertex-ID inventory includes GetBuiltin/MeshDrawParameter and guarded guest index loads. See `docs/RDNA1_FRONTEND_S036.md` and ignored evidence.

Next bounded local task: support complete acyclic linear IR block chains with strict graph/SSA order checks; reject forks/joins/cycles/incomplete graphs; lower the full constant frontend probe and retain the next precise vertex-ID semantic rejection. Continue subsequent local fixes/commits under D-033 until required native RX handoff; no production renderer change or push.

## Retained checkpoint S-035
Native RX consolidated generated draw/textured milestone PASS: 13 stages, five RX5700XT strict Vulkan1.3 selections, exact geometry/UV buffers and all images, zero errors/warnings through cleanup. Five supplied PPMs independently match every RGB pixel/hash. Package source `70a80620c583037e1ed4fe372bdb89586dbd895a`, build `20261029T070921174Z`; report `docs/RDNA1_RX_IR_MILESTONE_S035.md`, raw copies/hashes/verification under ignored `_Build/p3-s035/`. No game or complete frontend/NGG support claim.

Next single task P3.7 (D-032): offline compatibility fixture through the real mesh frontend/prologue and normal IR preparation, using public authored guest instructions. Pass all resulting IR blocks/metadata to the research lowerer; do not manually replace or strip prologue/blocks to hide unsupported semantics. Gate successful frontend translation and source-backed required-operation inventory; then either independently validated compute output or deterministic empty-output rejection with the first real unsupported semantic and regression/next bounded fix. Production renderer defaults remain unchanged; no guest/proprietary captures in git. Batch related local fixes under D-028; commit and stop, no push.

## Retained checkpoint S-034
Clean consolidated package DONE locally: source `70a80620c583037e1ed4fe372bdb89586dbd895a`, build `20261029T070921174Z`, ZIP 1,555,283 bytes/SHA-256 `3D78510DF6E474855ACBF89B39AEBF9329CEFB2C1BABE31898FA29A5D0DFBBAD` with sidecar. Clean 26-case suite 19 PASS/7 strict SKIP/0 FAIL (3.21 s). Fifteen expected relocation/integrity/API/RX-guard/legacy outcomes verified; Intel auxiliary five images/blobs exact with zero errors/warnings through cleanup. Independent RGB/hash checks PASS. Initial local controller exit-code error retained and fixed; package unaffected. See `docs/RDNA1_IR_PACKAGE_S034.md`.

Next single task: transfer ZIP+sidecar, verify/extract fresh on RX, run ordinary-user `Run-IR-Milestone.cmd -IsolateImplicitLayers`. Require all 13 stages PASS, all five native RX strict1.3 selections, exact complete geometry/UV/image checks and zero errors/warnings through all cleanups; retain result.json, 13 logs and five PPM images. Native consolidated gate and Astro Bot remain NOT RUN until evidence. Commit evidence and stop; no next phase or push. Final docs commit does not alter package source identity.

## Retained checkpoint S-033
P3.6 actual generated geometry/UV now feed standalone textured draw. Intel Vulkan 1.2 auxiliary nonzero/zero images and both full guarded buffers exact; zero validation errors/warnings including cleanup. Two 2-job builds PASS; 26-case suite 19 PASS/7 strict SKIPPED/0 FAIL (3.39 s). Four negative probes correctly FAIL; independent RGB images exact. See `docs/RDNA1_IR_TEXTURE_S033.md`; full snapshots/logs retained. New native RX generated draw/textured milestone and Astro Bot NOT RUN.

Next single task: one clean committed-source package/runner for accumulated generated draw and textured UV milestone under D-028. Cover all 26 focused cases, UV modules and oracle source dependency, per-mode images/buffers and required-RX identity for every strict device. Gate provenance/manifests/checksums/import/license audit, Unicode relocation, exact images/blobs and intended integrity/fail/skip/RX-guard outcomes before hardware handoff. Preserve native/default path; no game integration or push. Commit and stop.

## Retained checkpoint S-032
P3.5 offline UV parameter lowering DONE: opt-in parameter-0 xy into separate 544-byte guarded storage at set0/binding2, 17 rejection cases, independent binary/full-snapshot oracle and eight valid SPIR-V mutants detected. Three 2-job builds PASS; 24-case suite 18 PASS/6 strict SKIPPED/0 FAIL (3.53 s); ten legacy generated modules byte-identical. New UV GPU execution NOT RUN; earlier Intel auxiliary regressions validation-clean. See `docs/RDNA1_IR_UV_S032.md`; all dirty snapshots and logs retained. Native generated draw/UV tests remain batched under D-028.

Next single local task P3.6: connect actual generated position/index/UV output to the existing standalone textured draw. Extend generated-mode descriptor binding 2, initialize and compare full guarded UV/geometry buffers and all pixels, preserve texture uploads and correct compute/vertex/sample/host dependencies, prevent authored UV-generator overwrite, keep strict and auxiliary API tests separate and validation clean through cleanup. Consolidate RX packaging at the textured milestone. No emulator/game wiring or push; commit and stop.

## Retained checkpoint S-031
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

## Retained checkpoint S-037
Complete linear-chain lowering and constant actual frontend SPIR-V PASS. Focused suite 20 PASS/7 strict SKIPPED/0 FAIL, 3.45 s; existing Intel auxiliary regressions clean. Live vertex-ID prologue fails closed at GetBuiltin. Full provenance and initial test-lifetime failure retained; see docs/RDNA1_LINEAR_IR_S037.md. Next bounded local task: explicit non-indexed single-group frontend specialization and local invocation semantics, rejection coverage and offline validation. Continue authorized sequential commits under D-033 until necessary RX handoff; no push.

## Retained checkpoint S-038
Bounded non-indexed single-group specialization and actual frontend triangle SPIR-V PASS offline. Full graph retained, ordinary propagation/DCE removes inactive index reads; nine context negatives and malformed parameter reject without partial mutation. 20 PASS/7 strict SKIPPED/0 FAIL (3.29 s), full snapshots/logs retained. Next run actual frontend triangle through standalone indexed draw with complete independent oracles and validation through cleanup. New frontend GPU/RX/game NOT RUN. Continue sequential commits under D-033; no push.

## Retained checkpoint S-039
Actual frontend-derived triangle/permuted/zero conventional draws PASS Intel1.2 auxiliary, all full buffers/pixels and zero validation messages through cleanup. 29 cases 21 PASS/8 strict SKIPPED/0 FAIL (4.00 s); wrong valid module/missing strict module correctly FAIL. Complete snapshots/logs `_Build/p3-s039/` and capture folders retained. New native frontend gate NOT RUN. Next clean consolidated frontend milestone package/runner, relocation/integrity/API/RX guard verification; then give ZIP and one RX command and stop under D-033. No production/game claim or push.

## S-040 package preparation
Frontend milestone runner/launcher and 29-case clean-source package inventory prepared. Commit source, build/verify package including Unicode relocation, checksums, auxiliary exact buffers/images, strict skip and RX guard; append actual results. Native acceptance requires 15 stages/eight strict RX selections/eight cleanups, exact blobs/images, zero errors/warnings. Then deliver ZIP/one command and stop under D-033. No push.

## Retained checkpoint S-040 — stop for native frontend gate
Clean package source87e912bd9786abd206b635ff7c68a832562f9b8a/build20261029T081156341Z verified. 29 tests 21 PASS/8 strict SKIPPED/0 FAIL (3.89 s), 15 expected package outcomes, eight exact images/full blobs and zero validation messages through eight Intel1.2 cleanups. ZIP2,586,529 bytes SHA256336303B17E837E97E5CEFF4EEE458460AECDB5C9AFDE22DB377ECDCB7FD06D89. See docs/RDNA1_FRONTEND_PACKAGE_S040.md. Next only: RX normal-user `Run-Frontend-Milestone.cmd -IsolateImplicitLayers`, require all15 stages/eight native strict1.3 selections/cleanups, exact guarded buffers/eight images, zero errors/warnings and full result folder. New frontend native gate and game NOT RUN. Stop under D-033, no push or production integration.

## Retained checkpoint S-041
Native actual frontend triangle/permuted/zero strict1.3 gate PASS on RX5700XT API1.4.308 AMD25.6.1. Package source87e912bd/build20261029T081156341Z; all15 stages PASS, eight cleanups zero errors/warnings, full buffers/images exact. All24 files copied/hash-verified `_Build/p3-s041/`; see docs/RDNA1_RX_FRONTEND_S041.md. No production/game claim. Next one local task: bounded actual frontend parameter0 xy UV export into existing544-byte guarded ABI; public ISA/full graph, metadata/mask rejection, independent expected UVs, SPIR-V1.3 and focused regressions/provenance. Runtime integration follows separately; D-028 batched RX cadence, no push.

## Retained checkpoint S-042
Actual frontend UV/zero export offline PASS through complete graph and independent136-word binary oracle; rejection and valid wrong-UV mutation checked. Build PASS;29 cases21 PASS/8 strict SKIPPED/0 FAIL(4.48s), full snapshot/logs retained. New frontend UV runtime/RX/game NOT RUN. Next actual frontend UV textured runtime with exact buffers/images and validation through cleanup; D-033 sequential commits, no push.

## Retained checkpoint S-043
Actual frontend UV/zero textured runtime PASS Intel1.2: full geometry/UV and pixels exact, zero validation through cleanup.31 cases22 PASS/9 strict SKIPPED/0 FAIL(4.42s), wrong valid UV/missing strict mutants fail correctly, full provenance retained. Next clean consolidated frontend geometry+UV package/relocation/integrity/API/device verification then native strict gate. No production/game claim or push; D-033 continuation.

## S-044 package preparation
Existing FrontendMilestone now includes actual frontend UV/zero texture. Commit source then build31 tests/package and verify Unicode relocation,16 stages/ten auxiliary selections/images, strict SKIP/RX guard/integrity negatives. New native gate NOT RUN. Once verified deliver ZIP/one launcher command and stop, no push.

## Retained checkpoint S-044 — stop for native frontend UV gate
Clean sourcea183c1904f11e9c6f6c035ad58432d3456f859e0/build20261029T095304950Z package verified.31 tests22 PASS/9 strict SKIPPED/0 FAIL(4.38s),15 expected package outcomes, ten exact auxiliary images/full buffers/cleanups zero validation.32 legacy modules identical. ZIP2,600,296 bytes SHA2566447AE462F7869E2A6CCBDF1AADA661C68288EBF30ADC8989BBF021DA3010F1A. Report docs/RDNA1_FRONTEND_UV_PACKAGE_S044.md. Next only native RX `Run-Frontend-Milestone.cmd -IsolateImplicitLayers`:16 stages/ten strict native selections+cleanups, exact geometry/UV/images, zero errors/warnings and complete result folder. New native UV/game NOT RUN; prior S-041 geometry native PASS. Stop under D-033; no production integration or push.

## Retained checkpoint S-045
Actual frontend geometry+UV native strict RX milestone PASS: sourcea183c190/build20261029T095304950Z,16/16 stages PASS, ten native RX5700XT API1.4.308 AMD25.6.1 selections/cleanups zero errors/warnings, full outputs/ten images exact.27 files copied/hash-verified `_Build/p3-s045/`; report docs/RDNA1_RX_FRONTEND_UV_S045.md. No production/game claim. Next single local task source-backed production adapter boundary contract: current compiler/draw metadata/resource/fragment/cache/synchronization mapping, precise unsupported-state rejection, OFF/native unchanged gate and one smallest implementation patch/test plan. No RX request for mapping; P5 not implemented; no push.

## Retained checkpoint S-046
Source-backed adapter contract docs/RDNA1_ADAPTER_CONTRACT_S046.md DONE. Production rejects missing mesh before frontend and assumes native modules/draw/cache; all gates mapped. Next P2.2a production-owned pure candidate selector+CPU tests: OFF/non-mesh/native preference/strict Vulkan+compute limits/direct bounded draw state, precise fail-closed reasons. Unwired candidate is not renderer readiness. No RX rerun for policy-only patch. Six execution categories NOT RUN mapping-only; native S-045 retained. P5 not implemented; D-033 continuation, no push.

## Current checkpoint S-048
Production-owned isolated compiler adapter implemented locally and AWAITING REVIEW: lowerer/specialization/ABI moved verbatim to `src/graphics/shader/recompiler/MeshCompute*{,.h/.cpp}` with new `Compile` entry and shared `IR::PreparePreResource` pass; `tests/MeshOutputContract.h` shim; test-side research lowerer/specialization headers deleted, UV oracle retained. Focused 2-job build PASS and `kyty_emulator` incremental build+link PASS after two retained build failures (static_assert typo; namespace using-form). Byte-for-byte 33/33 baseline modules exact. Focused suite: 7 offline PASS/0 FAIL; Intel1.2 auxiliary runtime add/draw/textured PASS and four strict1.3 SKIPPED with process-local `VK_LAYER_PATH=_Build\p4-s013\tools\validationlayers`; first no-layer-path pass honestly SKIPped all runtime cases. Report `docs/RDNA1_ADAPTER_S048.md`; evidence `_Build/p2-s048/`, snapshot `_Build/p0-s012/p2-s048-pre-build-01/`. **Nothing committed or pushed; all S-048 edits pending user review.** Next: review/commit, then a separate cache/bridge/scheduler gate only if directed; no RX package (D-028), no renderer integration.

## Retained checkpoint S-047
Pure production-owned candidate policy+CPU invariants PASS, no renderer call sites/activation.32 cases23 PASS/9 strict SKIPPED/0 FAIL(4.76s);512 coarse combinations/two modes and negative capability/draw limits. Full snapshot/logs `_Build/p2-s047/`/capture folder; report docs/RDNA1_FALLBACK_POLICY_S047.md. Native S-045 evidence retained, no new RX/game. Next one local task isolated production compiler adapter/lowering/specialization/ABI promotion with shared pre-resource preparation, no test-source dependency; exact binaries/ABI/default native preservation, unsupported diagnostic-empty results and focused SPIR-V/tests/provenance. Cache/bridge/scheduler remain separate; no push, D-033 continuation.

## S-048 review repairs — pending review, no commit
Adapter now compiles a private clone of const raw frontend IR, validates wave/CFG before preparation and rejects prepared sources. Raw positive/context-change/negative/source-preservation regressions PASS; repeated SSA replaced by normalization-only follow-up. Full final code snapshot p2-s048-fix-pre-build-03 verified. Emulator/focused builds PASS,32 cases23 PASS/9 strict SKIPPED/0 FAIL(4.35s),33 modules exact; logs `_Build/p2-s048-fix/`. Historical snapshot gap retained; upstream malformed Program destruction requires caller restore injected test corruption. Next review uncommitted S-048 repairs; no commit/push or next phase until review.

## S-048 final committed checkpoint
User approved committing original adapter promotion plus verified repairs. Emulator/focused build PASS;23 PASS/9 strict SKIPPED/0 FAIL;33 modules exact; final source snapshot retained. No renderer activation/new RX/game/package or push. Next single local task canonical backend/draw-context/ABI cache identity tests, native/compute separation and changed baked contexts distinct; no RX transfer for this CPU-only task. Historical uncommitted/review entries above are retained history, superseded by this checkpoint.

## Current checkpoint S-049
Canonical adapter byte-key helper/CPU tests PASS: exact domain/backend/version/full source+interface/mesh/UV/draw identities, no pointer/padding/hash-only equality. No production cache/renderer wiring.33 cases24 PASS/9 strict SKIPPED/0 FAIL(4.45s), full snapshot/logs retained; report docs/RDNA1_CACHE_IDENTITY_S049.md. Next bounded conventional vertex bridge matching actual guest fragment parameter0.xy; explicit unsupported type/interpolation/component rejection, independent SPIR-V interface checks and focused regressions. No RX rerun for identity-only work, no push.

## Current checkpoint S-050
Bounded reflected vertex bridge offline PASS (actual guest ABI float4 vs authored harness float2). Strict1.3 position/UV bridge, native-shaped fragment xy proof/rejection and independent binary mutant oracle; no production activation.34 tests25 PASS/9 strict SKIPPED/0 FAIL(4.37s), three snapshots and fixture failures retained `_Build/p3-s050/`. Report docs/RDNA1_VERTEX_BRIDGE_S050.md. Next single local task actual public pixel ISA through production emitter paired with bridge, exact interface/dataflow/rejection/SPIR-V tests; current authored fragments alone do not complete this gate. New bridge runtime/RX/game NOT RUN; no RX transfer or push.

## Current checkpoint S-051
Actual top-level production pixel ISA compiler/native emitter paired with vertex bridge offline PASS. Plain/xy accepted, z/w/remap/flat/no-perspective/missing UV rejected; independently matched binary interfaces. First full shader_cfg_tests2-job build PASS;37 cases28 PASS/9 strict SKIPPED/0 FAIL(4.65s), IR validation/SSA-use byte-equivalence pair PASS. Snapshot/logs/paired SPIR-V `_Build/p3-s051/`; report docs/RDNA1_PIXEL_BRIDGE_S051.md. Next isolated combined actual frontend geometry+UV/generated bridge/production pixel runtime with independent buffers/RGBA/clear+zero controls and cleanup validation. New paired runtime/RX/game NOT RUN; native transfer batched at combined gate, no push.

## Current checkpoint S-052
Actual generated stages combined draw PASS Intel1.2: complete geometry/UV/RGBA/clear/zero and two cleanups zero validation.41 cases31 PASS/10 strict SKIPPED/0 FAIL(5.10s), two full snapshots; valid wrong bridge/missing strict fail correctly. Report docs/RDNA1_COMBINED_RUNTIME_S052.md. Next clean consolidated paired milestone package/runner and relocation/integrity/API/device checks, then RX strict gate. Native combined RX/game NOT RUN; no renderer activation or push.

## S-053 package preparation
CombinedMilestone launcher/runner prepared:19 stages/12 native strict cases/cleanups/images. Clean source commit then41-test package build and relocation/integrity/auxiliary/strict/RX guard verification. Full native pixel compiler producer not shipped, actual generated modules included. Append final evidence and deliver one ZIP/command, stop for native gate; no push.

## Current checkpoint S-053 — stop for native combined-stage gate
Clean sourcead57feab7d794b7cedb855a6fb5411ca76881200/build20261029T122933634Z package verified.41 cases31 PASS/10 strict SKIPPED/0 FAIL(5.11s),16 expected package/negative outcomes;19 auxiliary stages/12 images/full buffers/cleanups exact and zero validation.36 previous modules identical. ZIP2,780,495 bytes SHA2567DC0B74CB00106E63B706B14F15C6311D6C38318A78E652487BF1F98799F6469. Report docs/RDNA1_COMBINED_PACKAGE_S053.md. Next only RX Run-Combined-Milestone.cmd -IsolateImplicitLayers,19 stages/12 strict native cases+cleanups/exact complete outputs/images/zero warnings+errors; return full result folder. New native paired/game NOT RUN; no renderer activation or push. Stop D-033.

## Current checkpoint S-054 — native combined gate FAIL

Source ad57feab7d794b7cedb855a6fb5411ca76881200/build20261029T122933634Z. Native RX5700XT API1.4.308 AMD25.6.1:18 stages PASS/1 FAIL/0 SKIPPED. Ten prior runtime cases/images exact, zero-error/warning cleanup summaries. Paired frontend_uv full buffers/clear PASS but58/4096 RGBA pixels mismatch; zero final validation errors including cleanup, normal final warning summary absent. Independent RGB confirms58 pixels with only -1 differing channels; cause unproven. Paired zero NOT RUN after failure,11/12 native selections. Report docs/RDNA1_RX_COMBINED_S054.md; raw logs/images/hash inventory `_Build/p3-s054/`.

Next one local task diagnose actual paired color mismatch with independent interpolation/conversion expectations and discriminating controls. Acceptance: explain observed values and independently test justified correction/precision contract while preserving full coverage/negative regressions; then qualifying strict RX rerun before closing gate. No silent tolerance relaxation or next-phase renderer wiring. Intel auxiliary evidence separate; Astro Bot NOT RUN. No new build/package/push in S-054.

## Current checkpoint S-055 — paired precision capture

Independent Fraction analysis reproduces58 RX differences; all channels within ideal floor/ceil, but RX pre-conversion floats unknown. Official UNORM rule permits either adjacent integer; do not replace original nearest checker or claim cause resolved. Added same native stage RGBA32F diagnostic, full4096-row triangle/zero CSVs, exact clear/channel/finite checks and observational errors; diagnostic completion is not paired-color acceptance. Failure cleanup counts explicit.43 tests32 PASS/11 strict SKIPPED/0 FAIL(6.57s), Intel1.2 captures validation-clean/maxerror1.17317078e-7,44 shaders byte-identical, swapped bridge FAIL488 pixels. Full source snapshots `_Build/p0-s012/p3-s055-pre-build-*`, evidence `_Build/p3-s055/`, report docs/RDNA1_PAIRED_PRECISION_S055.md.

Next single task after clean package verification: RX Run-Paired-Diagnostic.cmd -IsolateImplicitLayers; original combined failure remains FAIL while float capture still runs. Collect20-stage logs,JSON,PPMs and both4096-row CSVs; independent device/API identity, full buffers/clear/zero and zero errors/warnings through cleanup. Diagnose actual values before justified correction and final RX gate; no renderer integration/push/game claim.

### S-055 final stop / clean diagnostic transfer

Source0ebb417077d53534f0a38091cf0081d3ffd0449f/build20261029T130742949Z ZIP2,789,604 bytes SHA256 `0D1419F8DEC9876F1D35F49BB7FBB61A7E59C173F61A6314754130D18C1F49D5`; matching sidecar. Clean2-job build PASS,43 tests32 PASS/11 strict SKIPPED/0 FAIL(5.73s).21 expected transfer/negative outcomes; actual Intel auxiliary20 stages/14 selections+cleanups validation-clean,12 exact PPMs/2 complete float CSVs;44 prior shader bytes unchanged. Explicit local wrong-bridge fixture confirms originalFAIL preserved while both float captures complete. Details docs/RDNA1_PAIRED_PRECISION_S055.md; no push.

Next ONLY RX fresh extraction then Run-Paired-Diagnostic.cmd -IsolateImplicitLayers; return complete results including both paired-float CSVs. Original paired may repeatFAIL/abort its zero case, but float stage must still capture triangle and zero. Diagnostic success never closes paired color gate. Native numerical cause and justified correction remain unresolved; no further renderer integration before evidence.

## Current checkpoint S-056 — native float evidence reviewed

Native RX5700XT API1.4.308 AMD25.6.1, source0ebb417077d53534f0a38091cf0081d3ffd0449f/build20261029T130742949Z:20 stages19PASS/1FAIL/0SKIPPED. Original paired RGBA8 repeats58pixelFAIL, original paired zeroNOT RUN; new float triangle+zero capturesPASS, max independent four-channel error1.1920928955078125e-7, exact backgrounds/zero and channel pairing.13 native selections,12 successful+1 failure cleanup all zero errors/warnings.34 files/hash inventory `_Build/p3-s056/`, report docs/RDNA1_RX_PRECISION_S056.md.

Nearest-only oracle exceeds Vulkan adjacent-integer conversion guarantee; all observed RGB channels satisfy ideal and captured-float brackets, but separate-format draw correlation does not prove internal RGBA8 values. Keep originalFAIL; diagnostic floatPASS not numeric acceptance. Next one local task portable per-channel UNORM sets plus independently justified bounded float numeric gate, exact geometry/UV/coverage/clear and independent missing/swapped/shifted/nonfinite/value/boundary negative tests. Focused SPIR-V/Intel checks and source provenance before batched clean strict RX acceptance rerun. No renderer wiring/game/push.

## Current checkpoint S-057 — portable acceptance locally verified

Per-channel exact rational UNORM floor/ceil sets, exact integer/clear/saturation, separate nearest diagnostics. Normal paired runs additionally require full RGBA32F numeric error<=2^-20 (explicit conservative fixture budget, not universal guarantee), finite/channel/clear/zero controls. Full dirty snapshots -01/-02 retained.43 cases32 PASS/11 strict SKIPPED/0 FAIL(8.70s), Intel1.2 paired triangle/zero validation-clean/maxerror1.17317078e-7. Wrong bridge FAIL488 UNORM pixels and separately numericFAIL0.952628968 despite structural0;44 modules unchanged. Report docs/RDNA1_PAIRED_ACCEPTANCE_S057.md; evidence `_Build/p3-s057/`. Historical RXFAIL retained; new strict RX acceptance and gameNOT RUN.

Next single task clean acceptance package with mandatory float CSV artifact inventory,43 focused cases, relocation/integrity/API/RX guards and wrong-module checks; then fresh strict RX handoff. No renderer wiring or push.

### S-058 — clean acceptance package preparation

Input4f633f3d clean branch/14 submodules. Mandatory paired triangle/zero CSV
inventory added to combined runner. Source commit precedes clean packaging;
verify43 focused tests, manifests/checksums/imports/licenses, relocation,
strict/auxiliary/RX guards and valid-wrong-module failures. Next only verified
native RX Run-Combined-Milestone.cmd -IsolateImplicitLayers with19 stages,
12 native cleanups, exact buffers/coverage/permitted UNORM+bounded float and
12 PPM/2 CSV artifacts. Do not close native gate from Intel. No push/game/wiring.

## Current checkpoint S-058 — STOP for native portable acceptance

Clean source9081c154137440411ba516e38fab984b4b6fd63e/build20261029T132623339Z, ZIP2,794,884 bytes SHA256 `3A0089400C4BAD3367E4A64CA025501C13D4ED9AA8C961E0A9815A006F9238AA`, matching sidecar.43 clean cases32 PASS/11 strict SKIPPED/0 FAIL(5.58s).22 expected relocation/integrity/API/device/negative outcomes verified; actual Intel auxiliary19 stages/12 validation-clean cleanups,12 exact reference PPMs/2 complete bounded float CSVs;44 modules unchanged. Report docs/RDNA1_ACCEPTANCE_PACKAGE_S058.md, evidence `_Build/p3-s058/`. New native acceptance/game NOT RUN; no push.

Next ONLY RX fresh extraction Run-Combined-Milestone.cmd -IsolateImplicitLayers. Require19 stages PASS/12 native RX strict selections+zero-error/warning cleanups, full geometry/UV/coverage/clear/zero, permitted UNORM sets and bounded numeric float checks. Return whole folder19 logs+JSON+12 PPMs+paired-frontend_uv.csv/paired-frontend_uv_zero.csv(4096 rows each). Nonzero nearest-different alone is not failure; permitted-set-invalid/numeric errors must be0. Preserve prior native failures. No renderer wiring until independent gate.

## Current checkpoint S-059 — native combined portable acceptance PASS

Source9081c154137440411ba516e38fab984b4b6fd63e/build20261029T132623339Z. RX5700XT API1.4.308 AMD25.6.1:19 stages PASS/0FAIL/0SKIPPED,12 strict native selections+cleanups zero validation errors/warnings. Full buffers/coverage/clear/zero/permitted UNORM and mandatory bounded float gates PASS; independent14 artifact hashes,11 exact reference images/paired rational RGB checks and two4096-row float CSVs verified. Maxtriangleerror1.1920928955078125e-7,zero exact.34 raw files/hash inventory `_Build/p3-s059/`, report docs/RDNA1_RX_ACCEPTANCE_S059.md. Old failures retained; no new local build/push/game claim.

Next single local task D-035: atomic isolated production-owned compile bundle joining existing admission/compiler adapter/reflected bridge/canonical key. Gate accepted public pair modules+ABI/interface/context identity, every rejection stable diagnostic/empty partial outputs, source preservation/context separation and independent CPU/SPIR-V regressions with complete provenance. No renderer activation/native cache mutation. Scheduler/descriptors/scratch/lifetime follow separately; no RX transfer for offline bundle alone. Standalone success does not imply emulator integration or Astro Bot.

## Current checkpoint S-060 — isolated compile bundle locally verified

Production meshFallbackBundle.h composes policy/private-clone adapter/reflected bridge/canonical identity with one Draw-derived specialization. Owning modules/key/ABI sizes published atomically; all rejection outputs empty with stable diagnostic, source preserved. Six real frontend fixtures plus authored fragment-interface regressions PASS; actual native pixel emission separate. Final full snapshot p3-s060-pre-build-03,2-job build PASS,43 cases32 PASS/11 strict SKIPPED/0 FAIL(6.40s),44 modules unchanged. Evidence `_Build/p3-s060/`, report docs/RDNA1_COMPILE_BUNDLE_S060.md. No renderer activation/cache mutation/new GPU or RX request/push/game claim.

Next single local task lowering-proven static counts/capacities and pure execution plan: triangle3/1 andzero0/0, exact ABI allocations/bindings/vertex-index ranges/dependencies, invalid/unproven counts and unsafe ranges reject atomically. Do not assume successful compilation means fixed draw counts; no Vulkan command recording yet. Scheduler/descriptors/lifetime follow separately; S-059 standalone RX acceptance retained.

### External-agent handoff after S-060

User requested context-plus-prompt.md for the next pure execution-plan task,
with implementation performed in a separate clone and a complete Git diff delivered
for owner review/application. No implementation in this handoff session. Exact
source check found lowerer Result already exposes vertices/primitives: external
agent must verify/reuse this proof rather than invent duplicate inference.
README/full plan/status/branch/14 submodules inspected; baseline3691b781.
Handoff includes file map, acceptance, tests/provenance and no-owner-write/no-push
boundary, overriding sequential implementation for the delegated task. Compilation,
unit, SPIR-V, Vulkan, RX and game tests NOT RUN (documentation only); diff checkPASS.
Next: external agent delivers full patch against declared base, owner reviews/applies
it. No scheduler activation or automatic continuation in this session.

### S-061 — In-repository work resumed / pure execution layout

User cancelled external-agent delivery restriction; context-plus-prompt.md retained as superseded history. Lowerer already proves vertices/primitives, now atomically propagated into optional checked execution layout; readiness requires plan, bundle identity version2. Generated set0 binding1 geometry/binding2UV, no payload binding0; exact guarded allocation/active ranges,3/1 vs0/0 draw gating and initialization/dependency/lifetime requirements. No allocation or commands. Full snapshots -01/-02,2-job buildsPASS;43 cases32PASS/11 strictSKIPPED/0FAIL(6.60s), final readiness-only frontend1PASS(0.14s),44 modules unchanged. Evidence `_Build/p3-s061/`, report docs/RDNA1_EXECUTION_PLAN_S061.md. New plan GPU/RX/gameNOT RUN; existing Intel regressions and S-059 native standalone evidence distinct. No push.

Next bounded task consume bundle/plan in isolated Vulkan harness with actual module identity, count/range-driven triangle/permuted/zero+UV draw, independent output and cleanup/synchronization checks. No production renderer activation. D-033 sequential authorization remains active; consolidate RX only at coherent execution milestone.

### S-062 — Isolated bundle execution locally verified

In-process actual public frontend+production CompileBundle/native public fragment; immutable proof and plan drive modules/descriptors1/2/no payload, allocation/ranges/barriers/dispatch/index count; zero no indexed draw. Runtime API qualified independently from offline reference compiler profile; explicit1.2 module adaptation not strict evidence.46 focused cases34 PASS/12 strictSKIPPED/0FAIL(7.46s),3 actual Intel1.2 bundle cases validation-clean/full outputs/numeric floats exact within declared budget. Valid wrong native pixelFAIL488pixels, missing fragmentFAILbefore device discovery, valid other-module/proof mismatch rejected offline;44 modules unchanged. Full source snapshots -01/-02/-03 final retained; report docs/RDNA1_BUNDLE_RUNTIME_S062.md, evidence `_Build/p3-s062/`. New native bundle runtime/gameNOT RUN, no production renderer wiring/push.

D-033 continuation: next clean consolidated bundle execution package/runner with46-case tests, relocation/integrity/API/RX guards and required three PPM+threeCSV bundle artifacts; then one native RX command and stop. No further production scheduler integration before independent coherent gate.

### S-063 compiler-bundle package preparation

D-033 authorized continuation after tested S-062: BundleMilestone runner/strict RX launcher adds offline proof tests and3 actual bundle cases to combined suite.21 stages/15 native strict selections+cleanups/15 PPMs+5 floatCSV required. Commit source then clean46-case build, relocation/integrity/auxiliary/strict/RX guards/wrong-module verification; deliver verifiedZIP+onecommand and STOP for native gate. No renderer/game activation or push. Actual final evidence appended after verification.
