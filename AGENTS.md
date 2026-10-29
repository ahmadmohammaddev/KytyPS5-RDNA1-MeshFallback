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

## Current checkpoint S-045
Actual frontend geometry+UV native strict RX milestone PASS: sourcea183c190/build20261029T095304950Z,16/16 stages PASS, ten native RX5700XT API1.4.308 AMD25.6.1 selections/cleanups zero errors/warnings, full outputs/ten images exact.27 files copied/hash-verified `_Build/p3-s045/`; report docs/RDNA1_RX_FRONTEND_UV_S045.md. No production/game claim. Next single local task source-backed production adapter boundary contract: current compiler/draw metadata/resource/fragment/cache/synchronization mapping, precise unsupported-state rejection, OFF/native unchanged gate and one smallest implementation patch/test plan. No RX request for mapping; P5 not implemented; no push.

## Current checkpoint S-046
Source-backed adapter contract docs/RDNA1_ADAPTER_CONTRACT_S046.md DONE. Production rejects missing mesh before frontend and assumes native modules/draw/cache; all gates mapped. Next P2.2a production-owned pure candidate selector+CPU tests: OFF/non-mesh/native preference/strict Vulkan+compute limits/direct bounded draw state, precise fail-closed reasons. Unwired candidate is not renderer readiness. No RX rerun for policy-only patch. Six execution categories NOT RUN mapping-only; native S-045 retained. P5 not implemented; D-033 continuation, no push.
