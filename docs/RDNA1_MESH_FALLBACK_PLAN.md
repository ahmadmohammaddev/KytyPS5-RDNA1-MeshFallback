# RDNA1 Mesh Fallback — Living Project Plan

> **This is the project's durable memory.** It is a living plan, not a frozen specification. Keep it current in every Codex session, commit it alongside code changes, and do not pretend untested work is complete.
>
> **Last edited:** 2026-10-27. **Plan revision:** 0.4. **Overall status:** P0 IN_PROGRESS — core baseline compiled; S-003 lightweight regression batch 22/22 passed; full CPU/offline audit, packaging and hardware validation remain pending.

## 0. Mission and boundaries

**Goal:** Investigate and, if feasible, implement a correctness-first optional Vulkan mesh-draw fallback for an AMD Radeon RX 5700 XT (RDNA1, 8 GiB VRAM), enabling KytyPS5 to use the real AMD GPU for ordinary graphics and compute rather than falling back to a CPU-only Vulkan ICD. Ultimate target: get meaningful Astro Bot (2024) game frames, then improve compatibility and performance. This is experimental: playable FPS is **not guaranteed**.

**Upstream:** `https://github.com/Jetsku/KytyPS5`

**Pinned starting point:** release `u59-windows-20261024-int21`, full commit `744705ee1df4b0acb1af382ff5c32ca4a20aa5f7` (verified locally in S-001). The current `feature/rdna1-mesh-fallback` branch descends from this tag; its only changes before S-001 were the initial plan and AGENTS.md. Do not silently rebase to a newer release.

**License:** Preserve GPL-2.0-only and all notices; respect dependency licenses.

**Two separate machines (user-reported specifications; runtime capabilities not measured this session):**

| Role | Machine | Responsibilities |
| --- | --- | --- |
| Development host | Huawei laptop; Windows 11 Home x64; Intel Core i5-1135G7, 4 cores / 8 threads; 16 GB system RAM; Intel Iris Xe integrated graphics | Codex, editing, memory-conscious compilation, static analysis, CPU/unit tests and offline shader checks; optional Intel Vulkan integration labeled Intel only |
| Hardware test target | Windows x64 desktop; Ryzen 5 7500F; Radeon RX 5700 XT 8 GB (RDNA1); 32 GB DDR5 RAM | Independent native RDNA1 Vulkan tests, correctness/performance measurements and Astro Bot compatibility |

Astro Bot stays installed locally on the desktop. Title `PPSA21567`, launcher-reported version `01.019.000` and ~148.88 GB extracted size are prior user reports, not checked here. Exact game paths stay local and must not be hardcoded or committed. Never assume the laptop has an RDNA1 GPU or transfer Intel results into an AMD hardware gate.

**Already observed (outside this fork, user screenshots):**
- KytyPS5 U59-based build lists Astro Bot and recognizes `eboot.bin`.
- Native RX 5700 XT initially failed `fragmentShaderBarycentric`, then a newer build advanced and failed on `VK_EXT_mesh_shader` at a mesh draw.
- Forcing Mesa Lavapipe/llvmpipe via `VK_DRIVER_FILES` displayed a splash screen, but did not prove game menu/gameplay; CPU was ~82–85% and GPU near 0%, so software rendering is not the desired final path.
- These observations are **not automated regression tests** and do not prove native GPU rendering works.

**Non-goals (until the fallback is correct):** performance tuning, 60 FPS, ray-tracing acceleration, global emulator refactors, removing validation, spoofing Vulkan features, replacing system drivers, publishing game assets, bypassing DRM.

## 1. Current checkpoint — update EVERY session

| Field | Current value |
| --- | --- |
| Current phase | **P0 — Fork and baseline** |
| Current subtask | P0.4 — lightweight CPU/offline batch complete (S-003); full baseline gate remains open |
| Status | `IN_PROGRESS` for P0/P0.4; core compilation PASS, broader regression baseline incomplete; no hardware gate complete |
| Last verified git commit | S-003 test build source `53bd13f21416d2dd67af05814a7781e2dc4c05bd` (clean); checkpoint commit identified by containing commit. S-002 emulator binary was not rebuilt |
| Working branch | `feature/rdna1-mesh-fallback` (verified) |
| Checkout evidence | S-002 rechecked clean status, pinned ancestry, branch, full SHAs, local remotes and 14 matching recursive submodules; `src`, `tests`, root CMake and `3rdparty` match pinned source |
| Build status | Qt-free Release/Ninja/clang-cl core configure PASS; `kyty_emulator` build PASS (1089.09 s, 2 jobs, no PGO/IPO); GUI/install/package not attempted |
| Native RDNA1 smoke test | Not run in this fork |
| Passing tests | S-003: 22/22 PASS, 0 failures/skips, serial CTest 1.69 s (20 CPU helpers + 2 offline SPIR-V); 403 not run. Named inventory: `docs/RDNA1_BASELINE_TEST_INVENTORY.md` |
| Known blockers / limits | Initial fetch/glslang blockers resolved without source changes. Broader regression baseline pending; Qt and standalone `spirv-val` not located; no emulator launch, GPU/game test, install or deployment; remote fork/push state unchecked |
| Next single actionable task | P0.4 source/dispatch audit only: resolve the 82 deferred cases in `docs/RDNA1_BASELINE_TEST_INVENTORY.md` using registrations, main/argument branches and initialization paths; record CPU/offline vs device/host requirements and select the next bounded build batch (including the already-confirmed CPU `null_texture_desc` as appropriate). Gate: zero unresolved dispositions among those 82, exact source/dispatch evidence and named next batch; no tests/build, packaging or P1 in that audit subtask. |

### Definition of status
`NOT_STARTED` / `IN_PROGRESS` / `BLOCKED` / `DONE (with evidence)` / `ABANDONED (with reason)`.

Never change a phase to `DONE` without a command/test/log and its actual result. A compile passing is not the same as the game rendering.

| Validation category | Current evidence in this fork |
| --- | --- |
| Compilation | PASS: unmodified Qt-free `kyty_emulator` linked, exit 0; GUI/install/package not run |
| Unit tests | PASS (subset): S-003 20 CPU helper cases; see named inventory. Full CPU baseline not yet classified/executed |
| Shader compilation and SPIR-V validation | PASS (scoped): 38 existing build shaders generated; `spirv_local_arrays` synthetic assembly/validation for Vulkan 1.3 passed. S-003 also passed `spirv_cache_salt` synthetic Vulkan 1.3 assembly/validation. No independent validation of the full generated shader corpus; standalone `spirv-val` absent |
| Vulkan integration tests | NOT RUN on either machine |
| RDNA1 hardware tests | NOT RUN on RX 5700 XT |
| Astro Bot compatibility tests | NOT RUN on RX 5700 XT; pre-fork observations retained separately |

## 2. Operating protocol for Codex (mandatory)

**At session START:**
1. Read root `AGENTS.md`, this entire file, README, current release build notes, and `git status` / `git log -1` / `git branch --show-current`.
2. Summarize the current verified state in 3–6 lines, then select **one smallest unfinished subtask** in the current phase.
3. Confirm factual source paths and flags by inspecting the checked-out code; filenames given in proposals are hypotheses until verified.
4. Never assume a previous response, private scratchpad, or context window is a valid record if it disagrees with this file or git.

**During session:**
1. Work only on the selected subtask unless a blocker forces replanning.
2. Prefer isolated branches/commits, minimal diffs, unit tests and synthetic examples. Preserve native mesh path as default where supported.
3. An experimental fallback flag (provisional name `KYTY_MESH_FALLBACK=compute`) must be **off by default** and must not falsely advertise `VK_EXT_mesh_shader` to the Vulkan loader.
4. Do not use Lavapipe for validation of GPU acceleration: check the selected Vulkan physical device and confirm AMD GPU activity.
5. Record unexpected findings, unsuccessful approaches and command output summaries (not huge raw dumps). Use local-only diagnostic artifacts for licensed game content.

**At session END:**
1. Run relevant tests if possible; report exact commands, return codes and limitations.
2. Update this checkpoint, phases, experiment table, decision log, and append a session log entry **before** any final answer.
3. Commit documentation in the same branch (together with code if appropriate). If commit/push is not possible, say why and leave an explicit next action.
4. Provide one clear next task, without automatically jumping to a new milestone.
5. If the goal or phase ordering needs change, edit this **living plan** deliberately: explain why under Change history. Never silently rewrite past evidence.

### 2.1 Laptop build policy and source-verified options

This is the build policy; actual execution evidence for the first Qt-free baseline is in S-002. Use an x64 Visual Studio developer shell with Ninja and clang-cl as documented in README. Default to **2 jobs**, preserve incremental outputs, close unnecessary memory-heavy applications and run only one build/test suite at a time. Intel Iris Xe shares system memory; 16 GB system RAM is not a dedicated GPU budget. On memory exhaustion, retain the failure log and retry the same build with **1 job**. If a single compile/link still exhausts memory, record the blocker and reassess configuration.

Source checks at S-001 input HEAD:

- Root `CMakeLists.txt` defines `KYTY_BUILD_LAUNCHER` (default ON), `KYTY_PGO_GENERATE` (OFF), `KYTY_PGO_USE` (empty), `KYTY_EMULATOR_IPO` (OFF), `KYTY_MARCH` (empty), `BUILD_TESTING` via CTest, and targets `kyty_emulator` and `kyty_tests`.
- The root only enters `src/launcher/CMakeLists.txt` when the launcher is enabled; that file requires Qt **6.8+** Concurrent/Network/Widgets. `KYTY_BUILD_LAUNCHER=OFF` therefore supports a Qt-free core/CLI build on Windows as well as the README's Linux example. This does not remove Vulkan/runtime dependencies or make all tests headless.
- Root CMake requires `glslangValidator` or `glslang` on PATH. `3rdparty/CMakeLists.txt` forces `SPIRV_SKIP_EXECUTABLES=ON`: do not assume a `spirv-val` executable is produced by this build; inventory a separate validator where needed.
- `src/generate_version.cmake` and `src/kytyGitVersion.h.in` already generate full `KYTY_GIT_REVISION` plus short/dirty labels. Verify generated identity for each build; do not invent a build-ID flag.

Qt-free configure/build recipe (core configure/build executed in S-002; the aggregate `kyty_tests` command below has NOT been run):

```powershell
$env:CMAKE_BUILD_PARALLEL_LEVEL = '2'
cmake -S . -B _Build/windows-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DKYTY_BUILD_LAUNCHER=OFF -DBUILD_TESTING=ON -DKYTY_PGO_GENERATE=OFF -DKYTY_PGO_USE:FILEPATH= -DKYTY_EMULATOR_IPO=OFF -DKYTY_MARCH:STRING=
cmake --build _Build/windows-core --target kyty_emulator --parallel 2
cmake --build _Build/windows-core --target kyty_tests --parallel 2
# Only after an observed memory failure, retry the failed target(s):
$env:CMAKE_BUILD_PARALLEL_LEVEL = '1'
cmake --build _Build/windows-core --target kyty_tests --parallel 1
```

Retry whichever target actually failed; the final line illustrates a test-target failure. Initial development uses **no PGO generation or profile use**, with emulator IPO off. Do not copy the release workflow's PGO path/junction recipe into the laptop baseline. Use a separate `_Build/windows-gui` for launcher builds with `KYTY_BUILD_LAUNCHER=ON` and the installed `CMAKE_PREFIX_PATH`; apply the same no-PGO/IPO policy and 2-job limit. Never use `KYTY_MARCH=native` for another CPU; record the portable host baseline.

### 2.2 Six independent validation categories

Every checkpoint/test report must give a separate result for each category: PASS, FAIL, INCONCLUSIVE or NOT RUN, with command, commit and environment. A skip is not a pass. One test may supply evidence to multiple categories only when its actual checks justify it.

| Category | Where / method | Acceptance evidence |
| --- | --- | --- |
| Compilation | Laptop initially, future Windows x64 CI | Configure/selected targets exit 0; flags, tool versions, job count, build logs and generated Git identity retained. Compilation says nothing about rendering. |
| Unit tests | Laptop / future CI, verified CPU-only subset | Inventory with `ctest --test-dir <build-dir> -N`; inspect registrations/source before selecting cases; run with `--output-on-failure --parallel 1`, record named tests/counts/failures/skips. S-003 confirms a 20-case CPU helper batch; broader selection remains incomplete. |
| Shader compilation and SPIR-V validation | Offline synthetic fixtures on laptop / future CI | Record compiler/SPIRV-Tools versions, fixture identity, exact Vulkan/SPIR-V target environment and validator results. Building glslang shaders alone does not validate all generated recompiler SPIR-V. |
| Vulkan integration tests | Available device; laptop explicitly Intel; desktop explicitly AMD | Record physical device, driver/ICD, features, validation-layer/synchronization settings and messages, correctness assertions and skips. The aggregate `kyty_tests` includes GPU-related executables; a full CTest run is not automatically a CPU unit-test result. |
| RDNA1 hardware tests | **RX 5700 XT desktop only** | Confirm native AMD ICD and selected physical device; archive phase-specific synthetic/rendering correctness, validation and performance evidence. Intel, hosted CI and Lavapipe cannot close this gate. |
| Astro Bot compatibility tests | **RX 5700 XT desktop only**, local installation | Record title/version, package identity, settings, duration and separate app-start/splash/new-frame/title/menu/gameplay outcomes, logs and screenshots. Synthetic RDNA1 success does not imply game compatibility. |

Hardware-dependent gates in P0.5 and P3–P8 remain incomplete until required tests run on RX 5700 XT. Generic Intel Vulkan integration is independent evidence. No RDNA1 milestone can be completed from laptop tests alone.

### 2.3 Build identity, future packaging and reproducible PC deployment

**Planned after the baseline is stable; no packaging script/workflow is implemented or validated this session.** Stable means P0.4 compilation passes and the classified CPU/offline baseline has recorded results with failures/skips explained. The first 3 passing cases in S-002 leave that broader gate open (D-010). P0.5 hardware baseline remains separate. Every local build, including failed attempts, records full `git rev-parse HEAD`, dirty/untracked state, submodule revisions, UTC build time, configuration and toolchain. Dirty builds retain a local diff plus untracked-source inventory/content; do not distribute them as clean commit builds.

1. From clean committed source, verify pinned ancestry, recursive submodule cleanliness and full SHA. Start fresh staging; record configure/build/install commands, toolchain/dependency versions, CMake options, portable CPU baseline, 2/1-job setting and six-category results. Sanitize manifests; user-specific absolute paths stay local.
2. Build Windows x64 core or GUI with the validated recipe, then `cmake --install <build-dir> --prefix <fresh-stage-dir>`. Use an explicit allowlist derived from install output/dependencies. GUI packages need matching Qt runtime/plugins and the existing updater dependency. Review MSVC runtime prerequisites: current `windeployqt` arguments exclude compiler runtime, so install alone does not prove a self-contained package.
3. Include runtime assets, matching `tools/u59-preset.json` when using the launcher, GPL license, `LICENSES/Kyty-MIT.txt`, relevant third-party notices and required fonts/assets. `.github/workflows/u59-windows-release.yml` is a reference for install/preset/font staging; its PGO recipe and fixed archive name need deliberate adaptation. Audit contents; omit games, firmware, keys, dumps, personal settings, saves, large caches and proprietary shaders. Keep optional debug symbols separately in local/controlled storage.
4. Name archives `KytyPS5-RDNA1-Windows-x64-<full-commit>-<core-or-gui>-<build-id>.zip`. Include a manifest with full commit/branch, clean state, submodule SHAs, tool versions, dependency identities, sanitized flags/commands, preset checksum, build ID and validation status. Record per-file SHA-256 checksums and an external archive checksum (`Get-FileHash -Algorithm SHA256`). Rebuilds need not be byte-identical; unique build IDs/checksums distinguish attempts.
5. Transfer archive, manifest and checksum to the desktop. Verify archive hash before extraction into a new commit/build-ID directory; verify per-file hashes, executables/dependencies and record PC OS/AMD driver/device. Preserve the official reference installation. Check generated identity against the manifest where diagnostics expose it. Resolve missing runtime prerequisites explicitly.
6. Launch from the package directory; configure game location only on the desktop. Record command/settings/preset and `VK_DRIVER_FILES`, `VK_ICD_FILENAMES` or `SDL_VULKAN_LIBRARY` overrides; confirm RX 5700 XT selection and no software ICD. Inspect the executable's `--help` before adding test/validation flags. Keep caches/settings external; record cold/warm state and shared settings effects.
7. Run the selected desktop test; record section 2.5 evidence. Retain the previous known-good package/official executable and demonstrate rollback by launching it with its matching preset and recorded settings. Return sanitized summaries/artifact references to the plan.

**Packaging/deployment gate (P0.4a):** commit-identified archive+manifest+hashes, license/content audit, independent desktop extraction/dependency launch check and demonstrated rollback. Deployment evidence does not complete P0.5 or any RDNA1 rendering gate.

### 2.4 Future GitHub Actions integration

Plan a separate opt-in branch CI workflow after local baseline and packaging validation (P0.6). Existing `.github/workflows/build.yml` and `u59-windows-release.yml` are references, not evidence that this research branch's CI passed. Reuse the no-PGO Windows x64 recipe, recursive checkout and commit manifest; pin/record toolchain/dependency inputs, cache by toolchain/configuration and upload commit/build-ID packages, hashes and logs. Separate compilation, CPU unit tests, offline shader/SPIR-V validation and supported Vulkan integration jobs; report unsupported device cases as skips. Preserve failure logs. Initial CI should upload artifacts without automatically publishing releases.

Hosted runners cannot certify RX 5700 XT or Astro Bot compatibility. Keep desktop gates manual unless a deliberately configured private self-hosted target is introduced later; never put games, firmware or large shader caches in Git or hosted artifacts.

**CI gate:** an actual run at the intended commit produces traceable packages and independent category results without private assets; hardware statuses remain NOT RUN until desktop evidence exists.

### 2.5 Hardware experiment record and artifact handling

After **every** desktop hardware test, including early failure, append an E-ID in section 5 with:

- Date/time, duration/time budget; full source SHA, package build ID/archive checksum; hypothesis and validation category.
- Machine/device, Windows version, AMD driver, Vulkan/ICD identity, validation/synchronization settings; title/version when applicable.
- Exact sanitized command, preset/settings changes, resolution, scene/route and cold/warm cache state sufficient to repeat the test.
- PASS/FAIL/INCONCLUSIVE with observed startup/rendering checkpoint, first failure and relevant validation messages; never infer gameplay from splash.
- Logs, relevant screenshots and metrics: FPS/frame-time distribution when frames exist, CPU/GPU utilization, RAM and dedicated/shared VRAM usage/peaks where measurable. Record unavailable metrics/screenshots and why, e.g. failure before window creation. Label diagnostics-on performance.
- Sanitized artifact references, checksums and storage location. Keep raw logs/captures, games, firmware, dumps and large caches outside Git; commit compact sanitized summaries and only shareable small evidence. No licensed shader dumps in history.

## 3. Phases and acceptance gates

### P0 — Manual fork, pinned checkout, reproducible baseline
Status: **IN_PROGRESS** — S-002 verifies toolchain and unmodified Qt-free core build plus 3 CPU/offline tests; broader regressions, packaging and hardware gates remain pending.

**Tasks:**
- P0.1: User creates a GitHub fork of Jetsku/KytyPS5; clones with submodules; adds `upstream`; fetches tags; creates/pushes feature branch from `u59-windows-20261024-int21`.
- P0.2: Record full `git rev-parse HEAD`, remotes, branch and submodule status, and verify license and original attribution.
- P0.2a: Document two-machine roles, laptop memory policy, validation categories and future packaging/deployment/CI gates. **DONE in S-001 (docs only).** P0.1/P0.2 local evidence verified; remote fork/push state unchecked.
- P0.3: **DONE (inventory, S-002):** Git/CMake/Ninja/x64 VS tools/LLVM/Python verified, explicit Qt-free choice, glslang provisioned locally; Qt and standalone SPIR-V CLI validator not located. Exact versions/evidence below. This is not a GUI build/toolchain gate.
- P0.4: **IN_PROGRESS:** unmodified pinned renderer/source (documentation commits allowed) built successfully on laptop with Release/Ninja/clang-cl, no PGO/IPO, 2 jobs; 3 selected CPU/offline tests passed. S-003 completed a bounded 22-case lightweight CPU/offline batch and named all 425 registrations. Continue source/dispatch classification of 82 deferred cases and later execution of remaining confirmed CPU/offline cases before calling the broader baseline stable (D-011). Record shader and Vulkan categories independently; hardware success is a separate gate.
- P0.4a (after P0.4): Implement Windows x64 packaging and verify desktop deployment/rollback per section 2.3, independently of rendering validation.
- P0.5 (after deployment): On RX 5700 XT verify GPU/driver, reproduce expected mesh-draw failure or record any different result; capture logs/screenshots/available metrics in an experiment. No gameplay success claimed.
- P0.6 (future, after local recipe validation): Integrate GitHub Actions per section 2.4 as its own subtask; manual hardware baseline does not depend on hosted CI.

**Gate:** P0.1–P0.5 evidence: full SHA, clean submodules, independent build/test category results, reproducible package deployment/rollback and actual RX 5700 XT baseline limitation. P0.6 stays independently tracked future CI work. **No fallback code until the baseline gate passes.**

### P1 — Map the real native mesh pipeline (read-only)
Status: **NOT_STARTED**

- Find guest mesh draw decoding/dispatch, shader types and recompiler IR/SPIR-V backend; host pipeline-cache Vulkan extension requirements; memory/barrier and draw command flow.
- Produce `docs/rdna1-mesh-architecture.md` with an accurate call graph and exact source-file:line references.
- List native mesh semantics relied on: workgroups, subgroup width, per-vertex/per-primitive outputs, primitive topology, indices, culling, payload and indirect draws.
- Assess what existing IR can express as compute and what must be added. Do NOT presume a simple SPIR-V execution model substitution works.

**Gate:** Reviewed architecture note, risks, proposed minimal insertion point, no runtime behavior change.

### P2 — Feature detection, traceability and negative test
Status: **NOT_STARTED**

- Distinguish `meshShader` feature vs extension presence, Barycentric and other independent capabilities.
- Add targeted diagnostics on RDNA1 for a failed mesh draw, including safe shader/pipeline identifiers and capability reasons, without repeated log spam.
- Add a feature-mocked/synthetic test that exercises selection logic; native supported devices must keep their existing path.
- Design fallback mode selection using explicit capability check; optional env flag remains off by default.

**Gate:** Same baseline behavior when flag off; deterministic test/log for unsupported GPU; no fake Vulkan extensions.

### P3 — Offline synthetic shader translation feasibility
Status: **NOT_STARTED**

- Define a very small supported subset of guest mesh IR (position output + one triangle; then multiple primitives; then per-primitive data).
- Implement or prototype a compute-oriented codegen path isolated from gameplay, using tiny synthetic public-domain fixtures.
- Validate generated SPIR-V with SPIRV-Tools as an offline gate; independently run the required smoke test on RX 5700 XT. If unavailable, leave the hardware gate pending.
- Document unsupported semantics and a clean fail-fast path. This phase may reveal the approach is infeasible or must change.

**Gate:** Synthetic test demonstrates compute processing on the **real AMD GPU** with correct positions/indices; no illegal SPIR-V or undisclosed fallback to CPU renderer.

### P4 — GPU output-buffer and traditional draw prototype
Status: **NOT_STARTED**

- Use device-local scratch buffers for mesh output (vertex, primitive/index, relevant varyings), with explicit capacities and bounds checks.
- Compute dispatch generates geometry; Vulkan barriers make writes visible to vertex/index consumers; draw with conventional vertex/fragment stages.
- Avoid CPU readback in the hot path where feasible; design safe overflow/fallback behavior.
- Build deterministic synthetic visual/golden-output or buffer validation tests.

**Gate:** Render a synthetic textured mesh/triangle with native RX 5700 XT GPU utilization, proper validation-layer results, no memory/device loss.

### P5 — Connect fallback to Kyty's actual mesh draw path
Status: **NOT_STARTED**

- Integrate the experimental fallback with real guest mesh translation, resource bindings, draw scheduling and pipeline-cache keys.
- Cover instancing, restart/topology, wave32 semantics and indirect draws in incremental tests; preserve default native mesh path.
- Fail with precise diagnostics for unsupported cases rather than silently skipping draws or inventing output.

**Gate:** A small representative guest mesh workload works on RX 5700 XT with flag ON, OFF unchanged, all relevant automated tests passing; preserve supported-device native-path evidence separately.

### P6 — Astro Bot integration on RX 5700 XT
Status: **NOT_STARTED**

- Run with a **local** legitimate game dump (never commit game data, game binaries, shaders or saves).
- Confirm `VK_DRIVER_FILES` is not accidentally forcing Lavapipe; check Vulkan physical device identifies RX 5700 XT.
- Record checkpoint screenshots/notes: app starts, splash, first new frame, title, menu, gameplay—do not equate them.
- If blocked, capture the first actual error, minimal sanitized shader metadata and a reproducible test; one issue per iteration.
- Treat `PPSA21567` v`01.019.000` separately from reference patches targeting `01.018.000`.

**Gate:** At least one genuinely game-rendered frame **beyond the splash**, demonstrably on RX 5700 XT; otherwise remain `BLOCKED` with evidence.

### P7 — Correctness and broader feature coverage
Status: **NOT_STARTED**

- Systematically close gaps found in shader semantics, primitive outputs, scratch memory, synchronization, raster state and long-run stability.
- Use bounded workloads and repeatable captures. Track crashes, missing geometry, scene correctness and game progression separately.
- Add tests for each fixed regression; keep fallback opt-in.

**Gate:** Title or gameplay scenes reproducibly rendered without gross geometry corruption, with known issues documented.

### P8 — Performance and 8 GiB VRAM budget
Status: **NOT_STARTED**

- Profile GPU occupancy, VRAM peaks, buffer allocation pressure, CPU overhead and pipeline compilation **only after correctness**.
- Compare cold/warm caches; test 720p then higher resolution; experiment with staging, pooling and batch dispatch without sacrificing barriers.
- Log repeatable frame-time distributions and driver version. Do not claim performance from screenshots alone.

**Gate:** Defensible benchmarks, no new rendering regressions and quantified overhead compared with a native-mesh GPU where available.

### P9 — Integration, release and future upstream sync
Status: **NOT_STARTED**

- Document full build/run instructions, supported GPUs, limits, flags and diagnostic output.
- Produce patch series / reviewable PRs that comply with upstream attribution, license and AI contribution policy.
- Evaluate a newer upstream release on a separate branch; merge/rebase only after deliberate compatibility review.

**Gate:** Reproducible installation, tested rollback, versioned release notes, human-reviewed contributions.

## 4. Decision register (append-only; add superseding decision rather than erase)

| ID | Date | Decision | Why | Revisit trigger |
| --- | --- | --- | --- | --- |
| D-001 | 2026-10-27 | Fork Jetsku/KytyPS5 at U59 int21 (`744705e`) | Latest confirmed release with RDNA1 startup improvements and known missing mesh fallback | A newer upstream release implements a real fallback |
| D-002 | 2026-10-27 | Treat full-mesh emulation on RDNA1 as R&D, not a promised game patch | No verified native RX 5700 XT gameplay evidence | P4/P6 experimental evidence |
| D-003 | 2026-10-27 | Avoid Lavapipe as main solution | CPU-only splash test consumed high CPU and showed no gameplay | Only for controlled reference tests |
| D-004 | 2026-10-27 | Keep fallback opt-in and source-level | Feature-spoofing or DLL tricks do not implement mesh semantics | Renderer reviewed and proven |
| D-005 | 2026-10-27 | Separate Huawei/Intel development from RX 5700 XT desktop tests | User clarified two machines; laptop has no RDNA1 GPU | Hardware/workflow changes |
| D-006 | 2026-10-27 | Initial builds: 2 jobs, 1-job memory retry, no PGO, emulator IPO off, optional verified Qt-free core | 16 GB laptop RAM; reproducible correctness builds first | Measured memory pressure or later performance phase |
| D-007 | 2026-10-27 | Add P0.2a workflow docs, P0.4a packaging/deployment, independent P0.5 hardware baseline and future P0.6 CI | Build, deployment and RDNA1 success need separate gates | Stable baseline/package evidence; CI capacity |
| D-008 | 2026-10-27 | Require commit/build identity, hashes and per-category experiment evidence | Separate-machine tests must map to exact source/package | Provenance or artifact policy changes |
| D-009 | 2026-10-27 | Use installed VS 2022 x64 LLVM 19.1.5 for no-PGO core baseline; add digest-verified glslang 16.6.0 only under ignored `_Build` | Developer shell resolved PATH gaps; dependencies and missing glslang were resolved without source edits; baseline linked | New compiler/shader failure, GUI requirements or later PGO work |
| D-010 | 2026-10-27 | Keep P0.4 broader regression gate open after first core build and 3 passing tests | Actual CTest inventory has 425 cases across CPU and GPU categories; 3 passes cannot certify the remaining 422 | Classified CPU/offline baseline results |
| D-011 | 2026-10-27 | Split P0.4 into bounded source-audit/build batches; retain full baseline gate | 425 registrations include 317 branches of one large Vulkan/mixed executable and full-emulator-linked CPU tests. S-003 validates 22 lightweight cases; 82 unresolved dispositions and one confirmed CPU full-emulator build remain. A named inventory is not complete execution/classification | Remaining source/dispatch audit and CPU/offline runs |

## 5. Experiments and evidence

| ID | Environment / git SHA | Hypothesis and test | Actual observed result | Outcome / artifact |
| --- | --- | --- | --- | --- |
| E-000 | Pre-fork user test, Oct 2026 | Run on RDNA1 native with U59 | Error: `VK_EXT_mesh_shader` required | Baseline context only; needs local reproduction |
| E-001 | Pre-fork user test, Oct 2026 | Use Lavapipe Vulkan ICD | Splash visible; no confirmed menu/gameplay; CPU ~85% | CPU-only reference; not native GPU solution |
| E-002 | 2026-10-27; local checkout `d1df5ff71d670174fe88191f182cfbf971dec94d`; no GPU selected | Documentation/source audit: verify checkout and proposed flags/targets | PASS for local clean checkout, pinned ancestry, 14 matching recursive submodules, flags/targets and version helper; no configure/build/test execution | S-001 summaries and section 2.1 source references; driver/device/performance N/A (read-only audit) |
| E-003 | 2026-10-27; Huawei development host; clean source `7f5213f42ba50aa1a36066cca2d65c7a3a762e63`; no GPU selected | P0 unmodified Qt-free Release baseline with 2 jobs, no PGO/IPO; inspect/execute CPU/offline tests | PASS configure (8.07 s), core build (1089.09 s), selected test build (9.25 s), CTest 3/3 (0.32 s). Fetch attempts and glslang error resolved without source changes. 38 build shaders generated; synthetic SPIR-V validation passed; no GPU/game evidence | Exact commands/attempt history in S-002; local ignored `_Build/p0-s002/` logs and `_Build/windows-core/` outputs. Driver/device N/A; broader baseline remains incomplete |
| E-004 | 2026-10-27; clean source `53bd13f21416d2dd67af05814a7781e2dc4c05bd`; development host, no GPU/driver selected | P0.4 lightweight CPU/offline batch, x64 LLVM 19.1.5, cached Release/no-PGO/no-IPO, 2 build jobs | Build exit 0; serial CTest 22/22 PASS, 0 failures/skips, 1.69 s. 403 not run; full baseline still open | Named inventory and commands in `docs/RDNA1_BASELINE_TEST_INVENTORY.md`; ignored `_Build/p0-s003/` evidence. No hardware metrics applicable |

New experiments MUST include real date, commit SHA, driver, device, exact command/config, time budget, result (PASS/FAIL/INCONCLUSIVE), and logs/artifact location. Hardware tests must also follow section 2.5 for package identity, screenshots and metrics (or explicit unavailability reasons). Avoid adding game code/assets to git.

## 6. Risks / exit criteria

| Risk | Severity | Mitigation |
| --- | --- | --- |
| Guest shader semantics incompatible with straightforward mesh-to-compute | HIGH | Offline IR feasibility first; reject fake one-line translation |
| Missing primitive/fragment varyings and clipping semantics | HIGH | Incremental synthetic fixtures before game integration |
| Incorrect barriers cause device loss / corruption | HIGH | Vulkan validation, bounded buffers, explicit dependencies |
| VRAM exhaustion at 8 GiB | HIGH | Instrument allocations and fixed budgets before optimization |
| Shader recompiler complexity exceeds available resources | HIGH | Stage-gated feasibility review; allow stop/reshape rather than fake success |
| Upstream moves and breaks experimental branch | MEDIUM | Pin release, deliberate syncs only |
| Unlicensed artifacts accidentally added to git | HIGH | Local paths outside repo, gitignore outputs, inspect `git status` before committing |
| Laptop memory exhaustion during compile/link | MEDIUM | 2 jobs, retry 1; incremental core-only builds, no initial PGO/IPO; record persistent failures |
| Desktop uses a stale/mismatched package | HIGH | Full commit/build ID, manifest and hashes, fresh extraction, identity checks |
| Intel/CI results mistaken for RDNA1 success | HIGH | Six validation categories and mandatory RX 5700 XT evidence |

**Stop or replan:** If P3 cannot express critical mesh semantics, stop implementation and present options (IR redesign, scope reduction, or alternate renderer) with evidence. If P4 cannot draw a correct synthetic triangle, do not proceed to Astro Bot.

## 7. Session handoff template — append new entries, do not remove history

### Session S-000 — 2026-10-27 — Initial plan (assistant, not Codex)
- Branch / HEAD: not created/verified.
- Intended task: prepare durable roadmap before fork.
- Changes: initial plan and operating protocol only.
- Verified: upstream release `u59-windows-20261024-int21`, short SHA `744705e` from GitHub; source build described in upstream README.
- Tests: none; no local clone or build.
- Blocks: user must fork/clone, then run P0.
- **Next exact task:** Verify fork, pinned tag, feature branch and recursive submodules; do not change renderer.

### Session S-001 — 2026-10-27 — Two-machine workflow documentation (Codex)
- **Branch / HEAD before → after:** `feature/rdna1-mesh-fallback`; input `d1df5ff71d670174fe88191f182cfbf971dec94d` → documentation commit containing this entry. Resolve final identity with `git log -1 --format=%H`; a commit cannot contain its own hash. No upstream merge/rebase or push.
- **Phase / one subtask:** P0.2a only, document the user-requested workflow. Local P0.1/P0.2 checks supplied evidence; no renderer implementation or P0.3 toolchain inventory started.
- **Changes:** `AGENTS.md`, `README.md`, this plan. Added machine roles/specs, memory policy, 2/1-job builds, no initial PGO/IPO, verified Qt-free option, six validation categories, future package/deployment/CI gates and hardware evidence policy. Updated checkpoint, phase status, decisions and change history. No scripts, workflows or renderer sources changed.
- **Git commands / actual results:** initial sandbox `git status --short` failed reading LibAtrac9 metadata; `git submodule status --recursive` failed due Git sh/MSYS permissions. Repeated required read-only checks outside sandbox: PASS, status clean; `git log -1 --oneline` = `d1df5ff7 docs: initialize RDNA1 mesh fallback roadmap`; branch correct; all 14 recursive submodules initialized at recorded SHAs (no mismatch/uninitialized/conflict markers). `git rev-parse HEAD` and tag resolved to the full SHAs above; `git merge-base --is-ancestor u59-windows-20261024-int21 HEAD` exited 0. `git diff u59-windows-20261024-int21 HEAD --stat` at input showed only initial AGENTS.md/plan additions.
- **Remotes / attribution:** local `git remote -v` reports origin `git@github.com:ahmadmohammaddev/KytyPS5-RDNA1-MeshFallback.git` and upstream `https://github.com/Jetsku/KytyPS5.git` for fetch/push. No network/fork ownership/push verification performed. README retains upstream credit, GPL-2.0-only and original Kyty MIT attribution; `LICENSE` header inspected and `LICENSES/Kyty-MIT.txt` exists. License/third-party files unchanged.
- **Source inspection:** read entire root README/AGENTS/plan plus relevant release build notes; inspected root and launcher CMake, dependency SPIRV-Tools settings, Git-version generator and existing build/release workflows. Confirmed flags/targets in section 2.1. An exploratory lookup of `tests/CMakeLists.txt` failed because test registration is in root CMake; corrected inspection to root. No guessed test-file path is used as build guidance.
- **Validation:** `git diff --check` PASS (exit 0; only configured LF→CRLF conversion notices); reviewed the full three-file diff against source and user requirements. PowerShell reference/fence checks PASS (all 12 documented source/reference files exist; Markdown code fences balanced). All six execution categories remain NOT RUN: no configure, compile, unit test, shader validation, Vulkan test, RDNA1 test or Astro Bot run in this documentation-only session.
- **Evidence / artifact locations:** compact command results in this entry and E-002; source references in section 2.1. No runtime logs, screenshots, performance data or game artifacts generated. Machine specifications are user reports, not runtime probes.
- **Decisions / revision:** D-005–D-008; revision 0.2. Added future tasks without completing build, packaging, CI or hardware milestones. Remote service state, laptop prerequisites, runtime dependencies and desktop behavior remain unverified.
- **Next single task / acceptance gate:** P0.3 only: inventory Git/CMake/Ninja, x64 VS/Build Tools, clang-cl/lld-link/llvm-lib, Python, glslang, separate SPIR-V validator availability and Qt 6.8+ MSVC x64 (or explicit Qt-free choice). Record actual versions/locations locally, sanitized versions/missing prerequisites in this plan, then stop. No build or renderer changes yet.

### Session S-002 — 2026-10-27 — P0 toolchain and unmodified core baseline (Codex)

- **Branch / HEAD before → after:** `feature/rdna1-mesh-fallback`; build input `7f5213f42ba50aa1a36066cca2d65c7a3a762e63` → documentation commit containing this entry. The executable retains the input SHA; the later documentation commit is not a rebuilt executable. No merge/rebase/push.
- **Phase / scope:** user explicitly authorized P0 inventory followed by baseline build for THIS SESSION ONLY, superseding S-001's inventory-only stop. Performed P0.3 prerequisites and one P0.4 baseline build/test attempt. No P1, packaging, renderer changes, game-file changes or game run.
- **Repository verification (PASS):** `git status --short` clean before and after execution; `git log -1 --oneline` = `7f5213f4 docs: define two-machine workflow`; branch correct; `git rev-parse HEAD` and `git rev-parse 'u59-windows-20261024-int21^{commit}'` matched recorded SHAs. `git merge-base --is-ancestor u59-windows-20261024-int21 HEAD` exit 0. `git submodule status --recursive` showed all 14 initialized at recorded revisions; `git remote -v` matched S-001. Git checks ran outside the sandbox due the already established submodule metadata restriction. `git diff --exit-code u59-windows-20261024-int21 HEAD -- src tests CMakeLists.txt 3rdparty` exit 0: build source matches the pin. Only the three Markdown files differed from the tag before this session. No remote fork-ownership or push-state check.
- **Toolchain inventory:** initial shell PATH lacked CMake/Ninja/LLVM tools. `vswhere -all -products '*' -format json` located VS Build Tools 2019 and 2022; `vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath` and `-find '**/cmake.exe'`, `-find '**/ninja.exe'`, `-find '**/clang-cl.exe'` located the VS 2022 installation and bundled tools. Selected its x64 developer environment, not the ARM64 compiler. Commands/results: `git --version` = 2.39.2.windows.1; `cmake --version` = 3.31.6-msvc6; `ninja --version` = 1.12.1; `clang-cl --version` = 19.1.5 targeting x86_64-pc-windows-msvc; `lld-link --version` = 19.1.5; `llvm-lib /?` worked. VS developer prompt = 17.14.42; `set VCToolsVersion` = 14.44.35207; `set WindowsSDKVersion` = 10.0.26100.0; target architecture x64. `python --version` = 3.11.0.
- **Qt / shader tools:** deliberately chose source-supported `KYTY_BUILD_LAUNCHER=OFF`; Qt tools were not found on PATH or in the checked conventional tool directories, so GUI prerequisites remain unverified. `glslang`/`glslangValidator` and standalone `spirv-val` were initially absent from those locations. The VS-bundled vcpkg executable existed but was not used to install anything. Standalone `spirv-val` remains unavailable; the selected offline regression uses the pinned SPIRV-Tools library directly.
- **Local glslang provisioning (PASS):** inspected [official Khronos glslang releases](https://github.com/KhronosGroup/glslang/releases/tag/16.6.0) and queried `https://api.github.com/repos/KhronosGroup/glslang/releases/tags/16.6.0`. Downloaded `https://github.com/KhronosGroup/glslang/releases/download/16.6.0/glslang-16.6.0-windows-x86_64-release.zip` with `Invoke-WebRequest -OutFile <ignored-local-archive> -TimeoutSec 90`; `Get-FileHash -Algorithm SHA256` matched the API asset digest `82bf434e69b9bb4829de7e2b4bc2c5e7a7861e53d66cf75e5cc70f5f694a8d9b` before `Expand-Archive`. Files remain under `_Build/p0-tools/glslang-16.6.0/`; `extracted/bin/glslang.exe --version` reported `11:16.6.0`, SPIR-V 1.6 support. Added that directory only to configure/build process PATH; no global install or persistent PATH change.

**Exact successful baseline commands** (from repository root in `cmd.exe`; command scripts/logs retained under ignored `_Build/p0-s002/`):

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64
set "PATH=%CD%\_Build\p0-tools\glslang-16.6.0\extracted\bin;%PATH%"
set CMAKE_BUILD_PARALLEL_LEVEL=2
cmake -S . -B _Build/windows-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DKYTY_BUILD_LAUNCHER=OFF -DBUILD_TESTING=ON -DKYTY_PGO_GENERATE=OFF -DKYTY_PGO_USE:FILEPATH= -DKYTY_EMULATOR_IPO=OFF -DKYTY_MARCH:STRING=
cmake --build _Build/windows-core --target kyty_emulator --parallel 2
cmake --build _Build/windows-core --target device_compat_tests range_set_tests spirv_local_arrays_tests --parallel 2
```

Configure retries 03–05 also set these transient Git transfer limits; no repository/global Git config was changed:

```bat
set GIT_CONFIG_COUNT=2
set GIT_CONFIG_KEY_0=http.lowSpeedLimit
set GIT_CONFIG_VALUE_0=1
set GIT_CONFIG_KEY_1=http.lowSpeedTime
set GIT_CONFIG_VALUE_1=30
```

**Attempt history / exact errors (retained, not rewritten as successes):**

| Attempt | Command / environment | Actual result |
| --- | --- | --- |
| Configure 01 | Same core CMake flags, x64 developer shell, default sandbox, no local glslang yet | FAIL, exit 1 after 10.25 s. Compiler ABI detection completed, then Opus clone failed three times: `fatal: unable to access 'https://github.com/xiph/opus.git/': getaddrinfo() thread failed to start`; CMake: `Failed to clone repository: 'https://github.com/xiph/opus.git'` at generated `opus-gitclone.cmake:50`, called by `3rdparty/CMakeLists.txt:24`. |
| Configure 02 | Same flags, outside sandbox for public pinned dependency fetch | Interrupted with Ctrl+C while Opus clone appeared stalled. Tool reported exit 1 for the interruption; no natural CMake result. |
| Connectivity diagnostic | `git -c http.lowSpeedLimit=1 -c http.lowSpeedTime=20 ls-remote https://github.com/xiph/opus.git HEAD`, owned process with 45-second cap | PASS, Git exit 0 in 6.39 s; HEAD `a8b2545eec71e0f53ecead2d583e7b6baa42684d`. This checks connectivity only; dependencies still use their pinned commits, not this moving HEAD. |
| Configure 03 | Same flags outside sandbox, transient Git low-speed settings above | Interrupted after 194.32 s at the three-minute diagnostic budget (tool interruption exit 1, not a natural configure failure). Reviewing the latest log showed Opus at `ddbe48383984d56acd9e1ab6a090c54ca6b735a6` and OpenXR at `f2448a8797c85814aa892efc1ab8707900fbcc78`; progress had resumed near the cutoff. Continued from this cache rather than declaring the fetch impossible. |
| Configure 04 | Same flags/settings, resumed cached dependencies | Dependency downloads completed. FAIL, exit 1 after 162.78 s at root `CMakeLists.txt:339`: `Could not find KYTY_GLSLANG_VALIDATOR using the following names: glslangValidator, glslang`. |
| Configure 05 | Same flags/settings plus verified local glslang PATH | PASS, exit 0 in 8.07 s, generated `_Build/windows-core/build.ninja`. Cache confirms Release, testing ON, launcher OFF, PGO generation OFF/use empty, emulator IPO OFF and empty portable CPU baseline. |
| Core build 01 | `cmake --build _Build/windows-core --target kyty_emulator --parallel 2` | PASS, exit 0 in 1089.09 s; UTC start `2026-10-27T18:11:00.0636229Z`. Linked `kyty_emulator.exe`. Nonfatal CRT deprecations and return-path warnings (including `src/libs/libKernel.cpp:2177`, `src/loader/runtimeLinker.cpp:992`) remain unchanged; no source patches or warning suppression. |
| Selected test build | Three target names in command block above, 2 jobs | PASS, exit 0 in 9.25 s. |

**CTest commands / actual results** (same bundled CTest executable; run from PowerShell, after target build):

```powershell
ctest --test-dir _Build/windows-core --show-only=json-v1
ctest --test-dir _Build/windows-core -N -R '^(device_compat|range_set|spirv_local_arrays)$'
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 -R '^(device_compat|range_set|spirv_local_arrays)$'
```

- Full inventory exited 0 with **425 registered cases**; selected inventory exited 0 with 3. Actual invocation used the absolute VS-bundled `ctest.exe` path because the parent PowerShell PATH lacked it. Selection source/registrations inspected: `tests/DeviceCompatTests.cpp` (synthetic capability/breadcrumb logic, Vulkan headers only), `tests/RangeSetTests.cpp` (CPU range/allocator cases) and `tests/SpirvLocalArraysTests.cpp` (synthetic assembly/validation/shrink using `SPV_ENV_VULKAN_1_3`; no Vulkan device).
- Run at UTC `2026-10-27T18:29:35.8203871Z`: `range_set` PASS 0.15 s; `spirv_local_arrays` PASS 0.09 s; `device_compat` PASS 0.05 s. **3/3 PASS, 0 failed, 0.32 s CTest total, exit 0**. Remaining 422 registered cases not executed; aggregate `kyty_tests` target not built. No full-suite claim.
- **Shader scope:** ordinary build generated 38 existing `.spv` files: tiler 26, blit 7, DCC 4, mesh-indirect helper 1. This is existing baseline shader generation, not Mesh Shader emulation implementation. The offline test validates its synthetic modules using the pinned SPIRV-Tools library; the complete set of 38 generated shaders was not independently validated by a CLI validator.
- **Memory:** 2 jobs throughout; no observed memory-exhaustion error and no 1-job retry. Valid sampled snapshot: 16183 MiB total physical RAM, 2658 MiB free, two Clang processes; this is one observation, not peak usage. An initial sandbox CIM query was denied; discarded its invalid zero-valued sample and replaced it with a successful outside-sandbox snapshot. No hardware rendering/performance inference.
- **Artifact identity:** `_Build/windows-core/kyty_emulator.exe`, 33,247,744 bytes; SHA-256 `8dc7ec2680205f1c7d0663d1b41380f796c740f39d5818701545e7bfe941b149`. Generated `kytyGitVersion.h` contains full `KYTY_GIT_REVISION` = `7f5213f42ba50aa1a36066cca2d65c7a3a762e63`, short label `7f5213f` without dirty suffix. No binary/package committed or distributed; no emulator launch, GUI build, install or deployment test.
- **Local evidence paths:** `_Build/p0-s002/provenance.txt`, `inventory.log`, `configure-01.log` through `configure-05.log`, `configure-*-result.txt` where produced (interrupted 02 has no natural result file), `opus-connectivity.*.log`, `opus-connectivity-result.txt`, `glslang-download.txt`, `glslang-version.txt`, `build-01.log`, `build-01-result.txt`, `test-build.log`, `test-build-result.txt`, `ctest-inventory.json`, `ctest-selected-inventory.log`, `ctest-selected.log`, `ctest-selected-result.txt`, `memory-snapshots.jsonl`, `artifact-identity.txt`; build cache/generated headers in `_Build/windows-core/`. These are ignored local outputs; this entry preserves compact sanitized results in Git.
- **Changed tracked files / validation:** `AGENTS.md` current task and this plan only. `git diff --check` PASS (exit 0, only LF→CRLF conversion notices); reviewed documentation against command logs and checked balanced code fences, retained prior session headings and current evidence IDs (PASS). No source, CMake, workflow, game, firmware or cache files added to Git; `git check-ignore` confirmed representative logs/executable/local compiler are ignored.
- **Decisions / limits:** D-009/D-010; revision 0.3. No unresolved blocker for the achieved core-build/3-test scope. Broader CPU/offline regression coverage, standalone SPIR-V CLI availability, GUI prerequisites, packaging/deployment and all Vulkan/RDNA1/Astro Bot results remain unverified. No GPU selected; no screenshots/FPS/VRAM data applicable to this build-only experiment.
- **Next exact single task / gate:** P0.4 CPU/offline baseline regression validation: classify the 425 cases from actual source/registrations; build/run the confirmed CPU/offline subset with 2 build jobs (1 if memory is exhausted) and serial tests. Gate: committed named case/category inventory, exact commands and actual pass/fail/exclusion counts; keep device-dependent categories explicitly pending. Then stop; do not package or begin P1 automatically.

### Session S-003 — 2026-10-27 — P0.4 lightweight regression batch (Codex)

- **Branch / input SHA:** `feature/rdna1-mesh-fallback`, `53bd13f21416d2dd67af05814a7781e2dc4c05bd` → containing documentation commit; no merge/rebase/push. Required Git status/log/branch and 14 recursive submodules verified clean/matching outside sandbox. `git diff --exit-code u59-windows-20261024-int21 HEAD -- src tests CMakeLists.txt 3rdparty` exit 0. Source still matches pin.
- **One subtask / revision:** P0.4 lightweight CPU/offline regression batch. D-011 splits the previously broad task after inspection showed 317 registrations on one large mixed Vulkan executable and costly full-emulator-linked CPU tests. This completes only the 22-case batch and named inventory, not the full P0.4 acceptance gate. No renderer/CMake changes, packaging, P1 or GPU/game execution.
- **Changes:** new `docs/RDNA1_BASELINE_TEST_INVENTORY.md` records all 425 exact names, command targets/arguments, root registration lines, selected source/rationale, commands and dispositions. Plan and AGENTS checkpoint synchronized. Current counts: 20 CPU PASS; 2 offline SPIR-V PASS; 1 additional confirmed CPU case (`null_texture_desc`) deferred full-emulator build; 318 Vulkan/mixed registrations conservatively excluded; 2 harness/setup excluded; 82 source/dispatch audits pending. Exclusions are not runtime skips or passes.
- **Build / tests:** S-002 x64 VS developer shell and local glslang PATH; `cmake --build _Build/windows-core --target <22 exact targets in inventory> --parallel 2` exit 0, 44 Ninja actions, build-log UTC file timestamps 18:47:40–18:47:57. Existing cached flags confirmed Release, launcher OFF, testing ON, PGO OFF/use empty, emulator IPO OFF, empty MARCH. No reconfigure, emulator rebuild or launch; old emulator identity remains S-002. LLVM 19.1.5, CMake 3.31.6-msvc6, Ninja 1.12.1, VS 17.14.42, MSVC headers 14.44.35207 and SDK 10.0.26100.0 rechecked. Nonfatal CRT deprecation warnings retained; no memory exhaustion/retry.
- **Actual CTest:** absolute VS-bundled CTest, `--test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 60 -R <anchored 22-name regex in inventory>` exit 0: 22/22 PASS, 0 failures/skips, 1.69 s. Both shader fixtures use pinned SPIRV-Tools `7f2d9ee926f98fc77a3ed1e1e0f113b8c9c49458`, `SPV_ENV_VULKAN_1_3`. Standalone validator/full generated shader corpus not run.
- **Failures / corrected observations:** initial sandbox status could not access LibAtrac9 Git metadata; submodule command hit MSYS permission failure. Required checks succeeded outside sandbox. An exploratory grouping of S-002 JSON failed because commands are absent for unbuilt executables; switched to generated CTest registrations with root-source backtraces. Initial executable-group classification put `null_texture_desc` among device exclusions; inspected its entire main and corrected it to confirmed CPU, deferred build (full emulator sources in CMake). A documentation check hit Python Windows default-encoding decoding/mojibake; corrected UTF-8 handling and reviewed the diff to preserve historical text. No test/build failures or source workarounds.
- **Evidence / provenance:** `_Build/p0-s003/`: build/toolchain `.cmd`/`.log`, result files, full named inventory JSON, selected regex, source SHA, clean status, empty source diff/untracked inventory, all recursive submodule revisions. Precise process duration was not instrumented; build timestamps are file observations. CTest detail in `_Build/windows-core/Testing/Temporary/LastTest.log`. No GPU selected; driver, screenshots, FPS/VRAM metrics N/A. No transferred packages/private assets.
- **Six categories:** Compilation PASS (22 test targets, prior core baseline retained); Unit tests PASS (20-case subset); Shader compilation and SPIR-V validation PASS (2 synthetic offline fixtures, scoped); Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN.
- **Documentation verification:** `git diff --check`, inventory count/unique-name/source-registration checks and Markdown fence checks PASS; fresh CTest JSON confirms 425 registrations. Build source was clean; subsequent edits are Markdown only. Checkpoint is identified by its containing commit, not the unrecompiled S-002 binary.
- **Next exact single task / gate:** P0.4 source/dispatch audit only: resolve the 82 deferred registrations with exact source/main/argument/initialization evidence, record device/host dependencies, and name the next bounded CPU/offline build batch. Gate: all 82 have resolved dispositions and evidence; keep execution pending. Do not build/run, package or begin P1 during that audit subtask. The additional confirmed CPU `null_texture_desc` remains unexecuted until a later build batch.

### Template for S-004 onward
- **Date / who:**
- **Branch / HEAD before → after:**
- **Phase / one subtask:**
- **What changed (files / commit):**
- **Commands executed and actual PASS/FAIL:**
- **Evidence / log paths (sanitized):**
- **Unexpected findings / known limits:**
- **Decisions / plan revisions:**
- **Next single task:**

## 8. Change history (append-only)

| Date | Plan revision | Sections | Reason / evidence |
| --- | --- | --- | --- |
| 2026-10-27 | 0.1 | All | Initial roadmap; phases subject to evidence-based evolution |
| 2026-10-27 | 0.2 | 0–9; AGENTS.md; README | User specified Huawei/Intel laptop and separate RX 5700 XT PC. Local checkout/source audit supports low-memory build policy and Qt-free option. Split validation, add future package/deployment/CI gates and hardware evidence requirements; preserve prior observations/history. Documentation only (S-001). |
| 2026-10-27 | 0.3 | 1–5, 7–8; AGENTS.md | S-002 user-authorized P0 inventory/build: installed x64 developer tools plus local digest-verified glslang enabled unmodified core compilation and 3 CPU/offline passes. Preserve failed/interrupted attempts, record exact source/artifact identity and keep broader 425-case baseline/hardware gates open. |
| 2026-10-27 | 0.4 | 1–5, 7–8; AGENTS.md; named test inventory | S-003 builds/runs 22 lightweight cases with all passing; records all 425 names and honest exclusions/deferred audits. D-011 splits P0.4 into bounded batches; baseline remains incomplete. |

## 9. Quick commands (reference; verify before running)

```powershell
# Initial setup on development laptop only, after manual GitHub fork.
# Already verified locally in S-001; do not re-clone or recreate the branch.
cd C:\
git clone --recurse-submodules https://github.com/YOUR_USERNAME/KytyPS5-RDNA1-MeshFallback.git C:\KytyDev
cd C:\KytyDev
git remote add upstream https://github.com/Jetsku/KytyPS5.git
git fetch upstream --tags
git switch -c feature/rdna1-mesh-fallback u59-windows-20261024-int21
git submodule update --init --recursive
git rev-parse HEAD
git status
git push -u origin feature/rdna1-mesh-fallback
```

Toolchain and baseline build directions: read the checked-out upstream `README.md` and `docs/EXPERIMENTAL.md`, especially the Windows Release + Ninja + clang-cl requirements. **Do not assume the first local non-PGO build matches the optimized upstream release FPS.**
