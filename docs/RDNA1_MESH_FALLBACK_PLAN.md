# RDNA1 Mesh Fallback — Living Project Plan

> **This is the project's durable memory.** It is a living plan, not a frozen specification. Keep it current in every Codex session, commit it alongside code changes, and do not pretend untested work is complete.
>
> **Last edited:** 2026-10-27. **Plan revision:** 1.0. **Overall status:** P1 static investigation complete (S-009); P0 baseline remains incomplete with known timing failure. Generic inventory expansion and timer fixes deferred under D-016. No fallback implemented; P2.1 targeted output-contract test next.

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
| Current phase | **P1 — native mesh source investigation complete; P2.1 next** |
| Current subtask | S-009 strategic review and P1 static architecture mapping complete |
| Status | P1 DONE (static evidence); P0/P0.4 incomplete and deferred, hardware gates open |
| Last verified git commit | S-009 input `57348577135f7ae12ed01d7fefad15856be9b92e`; clean source, 14 matching submodules; documentation commit is not a rebuilt binary |
| Working branch | `feature/rdna1-mesh-fallback` (verified) |
| Checkout evidence | S-009 clean status, recent eight commits, branch, pinned ancestry and 14 matching submodules verified; only source/test difference from pin is one unbuffered stdout line |
| Build status | Qt-free Release/Ninja/clang-cl core configure PASS; `kyty_emulator` build PASS (1089.09 s, 2 jobs, no PGO/IPO); GUI/install/package not attempted |
| Native RDNA1 smoke test | Not run in this fork |
| Passing tests | S-007 6/7 PASS, thread_service FAIL twice; prior evidence retained. 63 distinct passing cases, 1 failed-only, 361 never run |
| Known blockers / limits | CondWaitUntil failure retained as known baseline issue, no evidence blocking static renderer research. Early-signal check unreached; no stable full-suite or hardware claim |
| Next single actionable task | P2.1 offline synthetic output-contract test only: fixed positions/indices/count/status layout, alignment/capacity/zero-output/overflow/guards; focused 2-job build and serial CPU pass with provenance. No dispatch, renderer wiring, guest translation or timer change; commit and stop. |

### Definition of status
`NOT_STARTED` / `IN_PROGRESS` / `BLOCKED` / `DONE (with evidence)` / `ABANDONED (with reason)`.

Never change a phase to `DONE` without a command/test/log and its actual result. A compile passing is not the same as the game rendering.

| Validation category | Current evidence in this fork |
| --- | --- |
| Compilation | PASS: unmodified Qt-free `kyty_emulator` linked, exit 0; GUI/install/package not run |
| Unit tests | S-007 FAIL: six cases PASS, thread_service FAIL in batch and isolated retry. Prior passes/failures retained; baseline incomplete |
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
Status: **IN_PROGRESS** — prior core build/S-003 22 passes retained; S-004 source audit and S-005 16-case build/initial failure/retry recorded. Further CPU/offline execution, mixed-harness classification, packaging and hardware gates remain pending.

**Tasks:**
- P0.1: User creates a GitHub fork of Jetsku/KytyPS5; clones with submodules; adds `upstream`; fetches tags; creates/pushes feature branch from `u59-windows-20261024-int21`.
- P0.2: Record full `git rev-parse HEAD`, remotes, branch and submodule status, and verify license and original attribution.
- P0.2a: Document two-machine roles, laptop memory policy, validation categories and future packaging/deployment/CI gates. **DONE in S-001 (docs only).** P0.1/P0.2 local evidence verified; remote fork/push state unchecked.
- P0.3: **DONE (inventory, S-002):** Git/CMake/Ninja/x64 VS tools/LLVM/Python verified, explicit Qt-free choice, glslang provisioned locally; Qt and standalone SPIR-V CLI validator not located. Exact versions/evidence below. This is not a GUI build/toolchain gate.
- P0.4: **IN_PROGRESS:** unmodified pinned renderer/source (documentation commits allowed) built successfully on laptop with Release/Ninja/clang-cl, no PGO/IPO, 2 jobs; 3 selected CPU/offline tests passed. S-003 completed a bounded 22-case lightweight CPU/offline batch and named all 425 registrations. S-004 resolves those 82 by source (37 CPU/offline, 31 CPU with host dependencies, 13 offline shader, 1 Vulkan-required), with no execution. S-005 built those 16 (PASS), initial CTest 15/16 PASS with live_switch rename FAIL then isolated outside-sandbox retry PASS. S-006 remaining 19 focused CPU cases PASS (67.74 s), clean generated identity outside sandbox. S-007 host batch built (PASS), CTest 6/7 PASS and thread_service FAIL twice; S-008 measured coarse condition-variable oversleep; known failure deferred by D-016; further generic CPU/offline runs paused. Historically planned other CPU/offline runs and per-branch review of the 317-case conservative mixed-harness exclusion are still required before calling the baseline stable (D-011/D-012). Record shader and Vulkan categories independently; hardware success is a separate gate.
- P0.4a (after P0.4): Implement Windows x64 packaging and verify desktop deployment/rollback per section 2.3, independently of rendering validation.
- P0.5 (after deployment): On RX 5700 XT verify GPU/driver, reproduce expected mesh-draw failure or record any different result; capture logs/screenshots/available metrics in an experiment. No gameplay success claimed.
- P0.6 (future, after local recipe validation): Integrate GitHub Actions per section 2.4 as its own subtask; manual hardware baseline does not depend on hosted CI.

**Gate:** P0.1–P0.5 evidence: full SHA, clean submodules, independent build/test category results, reproducible package deployment/rollback and actual RX 5700 XT baseline limitation. P0.6 stays independently tracked future CI work. **P0 is not complete. D-016 permits source investigation and narrowly scoped synthetic feasibility work without completing every generic baseline/packaging gate; production/game integration still requires applicable regression, deployment and hardware evidence.**

### P1 — Map the real native mesh pipeline (read-only)
Status: **DONE (static investigation S-009)** — `docs/rdna1-mesh-architecture.md`, source `57348577135f7ae12ed01d7fefad15856be9b92e`. No runtime/fallback claim; independent human review pending.

- Find guest mesh draw decoding/dispatch, shader types and recompiler IR/SPIR-V backend; host pipeline-cache Vulkan extension requirements; memory/barrier and draw command flow.
- Produce `docs/rdna1-mesh-architecture.md` with an accurate call graph and exact source-file:line references.
- List native mesh semantics relied on: workgroups, subgroup width, per-vertex/per-primitive outputs, primitive topology, indices, culling, payload and indirect draws.
- Assess what existing IR can express as compute and what must be added. Do NOT presume a simple SPIR-V execution model substitution works.

**Gate:** Source-backed architecture note maps guest detection and ABI, front/back translation, IR exports/allocation, native SPIR-V and pipeline creation, capability checks, direct/indirect draw and recorder paths, I/O, wave/LDS and synchronization assumptions, risks and minimal compute prototype with explicit limits. Agent source review complete; no runtime behavior change, independent review pending.

### P2 — Output contract, feature detection, traceability and negative test
Status: **NOT_STARTED**

- P2.1 next: focused CPU/offline fixed output-contract test as specified in the architecture report. No Vulkan dispatch or guest lowering in this task.
- Native extension AND feature checks already exist (S-009); extend selection diagnostics/tests only where evidence requires. Keep Barycentric and other independent capabilities separate.
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
| D-012 | 2026-10-27 | Use source dispatch/initialization to classify the 82 deferred cases; next run 16 focused CPU targets | Kernel file-system main requires Vulkan despite its name; shader CFG is offline (validator helper Vulkan 1.2); CPU placement may internally skip. Linkage and test labels alone are insufficient. Audit completes classification only; 317 mixed-harness registrations remain conservatively excluded | New source/dispatch change or runtime contradiction |
| D-013 | 2026-10-27 | Retain initial sandbox failure and isolated outside-sandbox pass separately; retain generated dirty/unknown label | S-005 source checked clean outside sandbox but version helper suppresses Git errors; live_switch temp rename failed only in sandbox attempt. Environment-specific causation is inferred, not proven. No source patch or clean-package claim | Repeated failure outside sandbox or future packaging/provenance work |
| D-014 | 2026-10-27 | Diagnose thread_service before another baseline batch; do not weaken timing assertion | S-007 fails the precise-wait median threshold in batch and one isolated outside-sandbox retry; actual median/host timer conditions unavailable, so no cause established | Measured diagnostic evidence |
| D-015 | 2026-10-27 | Fix precise-wait coarse-phase assumption next; retain thresholds | S-008 measures cv.wait_for(2500 us) at 15959 us, exceeding whole 5 ms deadline; fine phase cannot recover. Keep unbuffered test diagnostics, no global timer-policy workaround | Focused fix evidence |
| D-016 | 2026-10-27 | Supersede D-014/D-015 next-action priority: defer CondWaitUntil fix and generic test expansion; allow P1 static review and bounded synthetic feasibility before full P0 closure | User strategic review; core build and 63 distinct passes suffice for source analysis, no evidence timer failure blocks mesh development. P0 remains incomplete, no thresholds changed | Direct renderer/prototype dependence on timing failure, or integration regression gate |

## 5. Experiments and evidence

| ID | Environment / git SHA | Hypothesis and test | Actual observed result | Outcome / artifact |
| --- | --- | --- | --- | --- |
| E-000 | Pre-fork user test, Oct 2026 | Run on RDNA1 native with U59 | Error: `VK_EXT_mesh_shader` required | Baseline context only; needs local reproduction |
| E-001 | Pre-fork user test, Oct 2026 | Use Lavapipe Vulkan ICD | Splash visible; no confirmed menu/gameplay; CPU ~85% | CPU-only reference; not native GPU solution |
| E-002 | 2026-10-27; local checkout `d1df5ff71d670174fe88191f182cfbf971dec94d`; no GPU selected | Documentation/source audit: verify checkout and proposed flags/targets | PASS for local clean checkout, pinned ancestry, 14 matching recursive submodules, flags/targets and version helper; no configure/build/test execution | S-001 summaries and section 2.1 source references; driver/device/performance N/A (read-only audit) |
| E-003 | 2026-10-27; Huawei development host; clean source `7f5213f42ba50aa1a36066cca2d65c7a3a762e63`; no GPU selected | P0 unmodified Qt-free Release baseline with 2 jobs, no PGO/IPO; inspect/execute CPU/offline tests | PASS configure (8.07 s), core build (1089.09 s), selected test build (9.25 s), CTest 3/3 (0.32 s). Fetch attempts and glslang error resolved without source changes. 38 build shaders generated; synthetic SPIR-V validation passed; no GPU/game evidence | Exact commands/attempt history in S-002; local ignored `_Build/p0-s002/` logs and `_Build/windows-core/` outputs. Driver/device N/A; broader baseline remains incomplete |
| E-004 | 2026-10-27; clean source `53bd13f21416d2dd67af05814a7781e2dc4c05bd`; development host, no GPU/driver selected | P0.4 lightweight CPU/offline batch, x64 LLVM 19.1.5, cached Release/no-PGO/no-IPO, 2 build jobs | Build exit 0; serial CTest 22/22 PASS, 0 failures/skips, 1.69 s. 403 not run; full baseline still open | Named inventory and commands in `docs/RDNA1_BASELINE_TEST_INVENTORY.md`; ignored `_Build/p0-s003/` evidence. No hardware metrics applicable |
| E-005 | 2026-10-27; clean audit source `226ea5adb896fcd610b82d8e34b1657e0392b16c`; no GPU/driver selected | Read-only P0.4 source/dispatch classification of 82 deferred registrations | PASS static gate: all 82 resolved with main/branch/init evidence and CTest properties; next 16-case focused CPU batch selected. No build/test/validator/runtime execution | `docs/RDNA1_BASELINE_SOURCE_AUDIT.md`, updated 425-case inventory; ignored `_Build/p0-s004/` audit/verification data. Performance/screenshots N/A |
| E-006 | 2026-10-27; build/test source `073eefecbc4f708d289439c6e3591d5a39d7a367`; no GPU selected | 16 focused CPU targets, Release/x64 LLVM 19.1.5, 2 jobs, serial CTest | Build PASS exit 0, 46.57 s. Initial 15/16 PASS, live_switch rename FAIL (0xc0000409), exit 8, 6.55 s, 0 skipped; isolated outside-sandbox retry PASS, exit 0, 1.58 s. Generated version label discrepancy retained | `docs/RDNA1_BASELINE_BATCH_S005.md`, ignored `_Build/p0-s005/` command logs, XML, hashes and provenance; no driver/ICD or performance data applicable |
| E-007 | 2026-10-27; clean `4ead9cbe9143d4ebb59f6c688e7ffcef5e965091`; development host, no GPU selected | 19 focused CPU cases, Release/x64 LLVM 19.1.5, outside sandbox, 2 build jobs/serial tests | Build exit 0, 131.95 s; CTest 19/19 PASS, 0 failed/skipped, exit 0, 67.74 s; no retries; clean generated version | `docs/RDNA1_BASELINE_BATCH_S006.md`, ignored `_Build/p0-s006/` logs/XML/hashes/provenance; device/driver/performance N/A |
| E-008 | 2026-10-27; clean `40626837f0f76d99096862a6337cf84e4ad28295`; no GPU selected | Five targets/seven host timer-thread cases, 2 build jobs, serial tests outside sandbox | Build PASS 20.80 s; CTest 6/7 PASS, thread_service FAIL, exit 8, 7.83 s; isolated retry FAIL, 2.19 s; 0 skips. Root cause pending | `docs/RDNA1_BASELINE_BATCH_S007.md`, ignored `_Build/p0-s007/`; no hardware/performance evidence |
| E-009 | 2026-10-27; dirty diagnostic variants at `3e7e5bb3175a4a9484b6c32ed73c347a5c4ca116`; laptop Windows build 26300 | Capture median lateness and component wait latency, two target builds/runs | Builds PASS; tests FAIL twice unchanged threshold; precise late 10449–10994 us, coarse 2500 us wait elapsed 15959 us, fine 500 us sleep elapsed 1116 us | `docs/RDNA1_THREAD_SERVICE_DIAGNOSIS.md`, ignored `_Build/p0-s008/` source diffs/logs/host snapshots/hash; no GPU |
| E-010 | 2026-10-27; clean source `57348577135f7ae12ed01d7fefad15856be9b92e`; no device/driver selected | Static P1 source investigation using rg/Get-Content and Git provenance checks | PASS static mapping; baseline sufficient for investigation only, known timing failure retained. No build/test/dispatch executed | `docs/rdna1-mesh-architecture.md`; S-009. No runtime duration/FPS/screenshots or hardware metrics applicable |

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

### Session S-004 — 2026-10-27 — P0.4 deferred source/dispatch audit (Codex)

- **Branch / input SHA:** `feature/rdna1-mesh-fallback`, `226ea5adb896fcd610b82d8e34b1657e0392b16c` → containing documentation commit. Clean input; last three commits reviewed. Required Git status/log/branch/recursive submodule checks PASS outside sandbox (known metadata limitation); all 14 revisions match. Pinned ancestry and source diff checks PASS. No merge/rebase/push or remote service query.
- **One subtask / gate:** resolve the 82 S-003 deferred registrations using checked-out source main/arguments/initialization and root CMake properties; select a next bounded build batch. Completed the source-audit gate: 82/82 resolved, zero unresolved; all still NOT RUN. No build/test/game/validator commands were executed, and no renderer/CMake/source changes made.
- **Files / evidence:** new `docs/RDNA1_BASELINE_SOURCE_AUDIT.md` provides 53 target evidence groups, all 82 case dispositions and inherited/explicit environment, CTest timeout/WILL_FAIL properties, source links, and exact next 16-case build/CTest commands. Updated `docs/RDNA1_BASELINE_TEST_INVENTORY.md` retains all 425 names and S-003 execution results while replacing the 82 deferred rows. AGENTS and current plan synchronized; historical S-003 entries unchanged.
- **Actual audit results:** 37 CPU/offline; 31 CPU with host VM/process/scheduling/timing/dummy SDL dependencies; 13 offline shader/IR/codegen; 1 Vulkan-required (`kernel_file_system` creates SDL hidden Vulkan window and calls CreateVulkan after its unregistered mutex-only branch). Across all registrations: 104 known CPU/offline candidates including 22 previously run; 319 Vulkan-required or conservatively mixed; 2 harness/setup. These are not new test passes and the 317 registrations on the compute harness still lack a full per-branch audit.
- **Significant requirements:** `shader_cfg` config/log/map initialization is CPU-only; validation helper is `SPV_ENV_VULKAN_1_2`, not the prior Vulkan 1.3 fixtures. CLI --help returns before Run but optional pre-parser CUDA initialization requires `KYTY_CUDA_EARLY_INIT` unset/0 for CPU-only runs. CPU placement can internally skip V-cache/live branches on unsuitable topology while exiting 0. Dummy SDL/fake HID/audio callbacks do not require physical controllers. Memory/JIT/fault/child-process tests need separate host-resource runs; full-emulator-linked CPU tests remain expensive builds, not GPU tests by linkage alone.
- **Commands / verification:** inspected root registrations and generated CTest backtraces/properties (read retained JSON only), each of 53 target main/dispatch paths, initialization, fake backend and relevant production helper paths using `rg`, `Get-Content` and Python UTF-8 readers. Static verification checks exact 425-name preservation, all 82 new dispositions, evidence file/line targets, registration/source mappings, counts and 16 focused next targets; `git diff --check` and Markdown fences checked before commit. No CTest invocation in S-004.
- **Failures / corrections:** first source-excerpt command hit Python Windows stdout encoding on a Japanese fixture filename; reran with UTF-8 stdout (no file mutation). A guessed `src/libs/libHttp.cpp` lookup failed; `rg` resolved URI registration to actual `src/libs/libNet.cpp:706`, then inspected it. Local classification script initially had one malformed quoted dictionary key; fixed and reran successfully before documentation generation. No build or runtime failure occurred.
- **Artifacts / limits:** ignored `_Build/p0-s004/` retains classification/generation scripts, audit JSON, next-batch JSON and verification/provenance. Source mapping is static evidence, not confirmation of compilation, link/runtime dependencies, memory use or host permission behavior. No GPU/driver/ICD selected; screenshots/performance N/A. No assets, packages, private captures or binary files committed.
- **Six categories (this session):** Compilation NOT RUN; Unit tests NOT RUN; Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Prior S-002 compilation and S-003 20 CPU + 2 offline passes remain valid only at their recorded inputs.
- **Decision / status:** D-012, revision 0.5. Source-audit subtask DONE; P0/P0.4 remain IN_PROGRESS. No blocker to attempting the next focused batch; broad CPU execution, mixed-harness audit, Qt/standalone validator, packaging/deployment, remote state and hardware gates remain pending.
- **Next exact single task / acceptance:** build and serially run only the 16 named focused CPU cases in the source audit (occlusion_reset through ces). Use the S-002 x64 VS 2022 shell, no PGO/IPO, two build jobs; retain memory failure then retry one job only if necessary. Gate: exact selection 16, build exit 0, all actual outcomes/failures/skips/duration/provenance recorded; commit evidence and stop. No renderer changes, full-emulator-target builds, packaging or P1. Null texture descriptor and other CPU/offline batches stay pending.

### Session S-005 — 2026-10-27 — P0.4 focused 16-case CPU build/run (Codex)

- **Branch / input SHA:** `feature/rdna1-mesh-fallback`, `073eefecbc4f708d289439c6e3591d5a39d7a367` → containing documentation commit. Required Git status/log/branch and all 14 matching recursive submodules PASS outside sandbox. Source clean before and after build; pinned ancestry/source-diff checks exit 0. No merge/rebase/push, packaging or P1.
- **One subtask / files:** completed only the selected 16-case CPU build/run attempt with all outcomes explained. New `docs/RDNA1_BASELINE_BATCH_S005.md` contains exact commands, 16 named results/times, failure/retry and next 19-case selection; inventory execution states updated, historical S-004 source audit retained with follow-up pointer. AGENTS/plan synchronized. No renderer/test/CMake source edits.
- **Build:** S-002 x64 VS shell/local glslang PATH, cached Release/testing ON/launcher OFF/no PGO/no IPO/empty MARCH reverified. `cmake --build _Build/windows-core --target <16 exact targets in batch evidence> --parallel 2` exit 0; 47 Ninja actions, 46.565 s; UTC 2026-10-27T19:12:58.8793562Z–19:13:45.4443596Z. Toolchain rechecked VS 17.14.42, clang-cl/LLD 19.1.5 x64, CMake 3.31.6-msvc6, Ninja 1.12.1, MSVC headers 14.44.35207, SDK 10.0.26100.0. Nonfatal CRT/offsetof warnings retained. No memory exhaustion or one-job retry, no peak memory measurement, no reconfigure/emulator/full-emulator-test target build.
- **Actual tests:** anchored source-audit regex selects exactly 16 cases, confirmed before/after build; all executable commands present afterward. CTest `--output-on-failure --parallel 1 --timeout 120 --output-junit <local>` initial exit 8, 15/16 PASS, live_switch FAIL at rename assertion with exit 0xc0000409, 0 skips, 6.55 s. Retained original XML/log/LastTest before the retry. One bounded `-R '^live_switch$'` retry outside sandbox passed unchanged binary, exit 0, 1.55 s case/1.58 s total. No other cases repeated; no full-batch 16/16 claim. No internal skip markers in initial detail log, no inherited KYTY/TRACY/VK_/SDL_ overrides.
- **Failure inspection:** `tests/LiveSwitchTests.cpp:247` temporary write/rename fixture, `src/common/liveSwitch.cpp:182` delete-sharing reader. No OS error code is printed; exact rename stage/cause remains unknown. Sandbox temp-file restriction inferred from isolated outside pass, not proven. No source workaround; preserve both attempts. Initial empty environment pipeline emitted no JSON file; corrected to explicit empty array and rechecked before tests.
- **Identity discrepancy:** generated header has full input SHA, but version unknown and short hash `073eefe-dirty`; retained original header locally. Version generator suppresses Git error output and treats any nonzero diff-index as dirty. Outside sandbox `git diff-index --quiet HEAD --` exit 0, `git describe --tags --always --dirty` = `u59-windows-20261024-int21-5-g073eefec`. Restricted Git/submodule metadata likely explains it, but suppressed build-time error not captured. Retained empty source diffs/untracked inventories and matching submodules; do not distribute as clean packages. Old emulator binary remains S-002, even though shared generated header was refreshed.
- **Evidence:** ignored `_Build/p0-s005/` retains commands, timed JSON results, toolchain output, cache copy, before/after clean-source provenance, full submodule list, generated header, selection manifests, initial/retry CTest logs/XML and detail logs, 16 executable SHA-256s, next batch manifest and documentation verification. No binary/assets/private captures committed. No physical device/driver/ICD, screenshots/FPS/VRAM metrics applicable.
- **Six categories:** Compilation PASS (16 selected targets); Unit tests initial 15 PASS/1 FAIL, isolated retry 1 PASS; Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Prior S-003 shader evidence retained at its original input. Across sessions 38 distinct cases have passing executions (36 CPU + 2 offline shader) with the S-005 failure retained; 387 registered cases never run.
- **Documentation verification / status:** exact 425 names/arguments/registrations preserved; only 16 new execution dispositions, named counts/XML/hash manifest and balanced UTF-8 Markdown checked; `git diff --check` PASS before commit. D-013/revision 0.6; P0/P0.4 remain IN_PROGRESS. No blocker to next bounded CPU attempt; environment sensitivity/version-label issue stays recorded for future reproduction/packaging.
- **Next exact single task / gate:** remaining 19 focused CPU cases in batch evidence, no full-emulator targets. Use 2 jobs (1 only on retained memory failure), serial 120 s CTest cap and exact 19-name regex. Gate: selection 19, build exit 0, all named actual results/failures/skips/duration/source/header provenance documented, checkpoint committed; stop. Other host-sensitive/13 shader CFG/full-emulator/mixed-harness cases, packaging and P1 remain later work.

### Session S-006 — 2026-10-27 — P0.4 remaining 19 focused CPU cases

- **Branch/input:** `feature/rdna1-mesh-fallback`, `4ead9cbe9143d4ebb59f6c688e7ffcef5e965091` → containing documentation commit; required Git status/log/branch/14 recursive submodules, pinned ancestry/source diff PASS. Clean before/after execution. No merge/rebase/push.
- **One subtask:** 19 focused CPU target build/run only. Exact commands, named outcomes and next seven-case batch in new `docs/RDNA1_BASELINE_BATCH_S006.md`. Updated AGENTS, plan and inventory; no source/CMake/renderer changes, packaging or P1.
- **Build:** outside sandbox, S-002 x64 VS/local glslang, Release/no PGO/no IPO/empty MARCH, two jobs. PASS exit 0, 131.954 s, start UTC 2026-10-27T19:19:03.0751678Z, 85 actions. Tool versions/cache verified in batch evidence. No memory exhaustion/retry. Generated full SHA correct, clean short label/version; S-005 discrepancy retained historically. Emulator binary remains S-002. Nonfatal deprecation warnings retained.
- **Tests:** selection exactly 19, outside sandbox, serial with 120 s cap and JUnit output. PASS 19/19, 0 failures/skips, exit 0, 67.74 s. draw_prep 65.18 s. No retries or source workarounds. No new build/test failures. Internal skip scan and exact XML verification retained with local evidence.
- **Provenance/evidence:** `_Build/p0-s006/` retains source/submodule before/after clean inventories/diffs, build script/log/timing, CMake cache, generated header, environment/selection, test log/XML/detail, 19 executable SHA-256s and verification. No private assets/binaries committed. No GPU/driver/ICD/screenshots or performance measurements applicable.
- **Six categories:** Compilation PASS; Unit tests PASS (19 subset); Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Across sessions 57 distinct cases have passing executions, 368 never run; S-005 initial failure retained, no full-suite claim.
- **Plan/status:** revision 0.7, no scope revision; continue D-011/D-013 bounded batches, P0/P0.4 IN_PROGRESS. Documentation names/counts/UTF-8/fences and git diff --check verified before commit. No blocker to next bounded attempt; mixed harness, other CPU/offline, Qt/validator, packaging/hardware/remote gates pending.
- **Next single task/gate:** seven host timer/thread cases in batch document, five focused targets, 2/1-job memory policy, serial tests, retain existing CTest properties (watchdog fire/fatal timeout 15 s). Gate selection 7, build exit 0, all named results/internal skips/failures/duration/provenance documented; commit and stop. No memory/JIT/topology/GPU/full-emulator/package/P1 scope.

### Session S-007 — 2026-10-27 — P0.4 seven host timer/thread cases

- **Branch/input:** `feature/rdna1-mesh-fallback`, `40626837f0f76d99096862a6337cf84e4ad28295` → containing documentation commit. Git status/log/branch, 14 recursive submodules, pinned source diff/ancestry PASS; source clean before/after execution. No merge/rebase/push.
- **One subtask/files:** five focused targets/seven host cases only. New batch S-007 evidence includes exact commands, named results and failure; updated AGENTS/plan/inventory. No source/renderer/CMake change, game run, packaging or P1.
- **Build:** x64 VS 17.14.42, LLVM/LLD 19.1.5, CMake 3.31.6-msvc6, Ninja 1.12.1, headers 14.44.35207/SDK 10.0.26100.0. Cached Release/no PGO/no IPO/empty MARCH, two jobs, outside sandbox. PASS exit 0, 20.801 s, start UTC 2026-10-27T19:30:39.8499760Z, 20 actions. Nonfatal warnings retained; no memory failure/retry. Full generated SHA and clean label match input; emulator remains S-002.
- **Actual tests/failure:** exact selection seven; serial CTest 120 s cap with watchdog fire/fatal existing 15 s property retained. Initial exit 8, 6/7 PASS, thread_service FAIL (0xc0000409), 0 skips, 7.83 s. Retained initial log/XML/detail before isolated unchanged-binary retry outside sandbox: FAIL at same assertion, exit 8, 2.19 s. No third retry. No internal skip markers in initial detail. No threshold/assertion/source workaround.
- **Source diagnosis/limits:** tests/ThreadServiceTests.cpp:220 and 245–252 measure median lateness across 25 five-ms waits, allowing three attempts below 400 us. Failed message identifies precise timed-wait deadline assertion. Buffered output lacks medians; host load/timer resolution/power conditions unmeasured. Cause unknown; cannot label this a harmless scheduling fluctuation. D-014 makes diagnosis the next single task.
- **Evidence:** ignored `_Build/p0-s007/` retains full before/after Git/submodule/diff/untracked provenance, toolchain/cache/build commands/log/timing, correct generated header, environment/selection, initial/retry logs/XML/details and five executable SHA-256s. No binaries/private assets committed. Driver/device/ICD/screenshots/performance N/A.
- **Six categories:** Compilation PASS; Unit tests FAIL (six pass, thread_service fails twice); Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. 63 distinct cases with passing executions, one failed-only, 361 never run; prior S-005 failure retained.
- **Status/verification:** P0/P0.4 IN_PROGRESS; timing failure blocks declaring this baseline stable. Revision 0.8/D-014 records next diagnosis, not new implementation scope. Exact registration/name preservation, result counts, UTF-8/fences and git diff --check verified before commit.
- **Next exact single task/gate:** thread_service timing diagnosis only, source paths in batch document. Capture unbuffered actual median lateness and host timer/scheduler evidence; reproduce and identify source-backed cause or bounded remaining hypotheses plus minimal follow-up. Leave thresholds unchanged, no next test batch/renderer/GPU/package/P1; commit findings and stop.

### Session S-008 — 2026-10-27 — thread_service timing diagnosis

- **Branch/input:** feature/rdna1-mesh-fallback, `3e7e5bb3175a4a9484b6c32ed73c347a5c4ca116` → containing commit. Clean input, required status/log/branch/14 matching submodules checked; no merge/rebase/push.
- **One task/results:** diagnosis only. Two thread_service diagnostic target builds at two jobs PASS, both direct serial runs outside sandbox FAIL unchanged 400 us assertion. Unbuffered medians 10449/10548/10564 us, then 10971/10994/10575 us. Direct 2500 us cv wait median 15959 us; SleepMicro500 median 1116 us. Coarse wait overshoots entire deadline before fine mode; this establishes a violated implementation assumption on this host, not the exact OS internal mechanism. Early-signal checks not reached.
- **Changes:** tests/ThreadServiceTests.cpp keeps unbuffered stdout only, no threshold/behavior change. Temporary probes removed; both dirty variants retained as local diffs. New diagnosis document, AGENTS/plan updated; no renderer/CMake/game changes. Final source equals first diagnostic variant, last local executable includes temporary second probe; do not distribute it as final source.
- **Host/evidence:** Windows 11 Home build26300, i5-1135G7/8 logical CPUs, Balanced plan, pre-run CPU20%, QPC10MHz, NtQueryTimerResolution current1ms/min15.625ms/max0.5ms; no settings changed. Snapshot is not process wait-resolution guarantee or peak/load trace. `_Build/p0-s008/` retains source SHA/submodules/dirty diff/untracked inventory/cache, both build/run logs/results, timer/CPU/OS/power snapshots, generated dirty header and binary hash. Exact commands/toolchain use S-002 x64 VS LLVM19.1.5/noPGO/noIPO, isolated thread_service target; details in diagnosis. No memory failure/retry; no other suite run.
- **Six categories:** Compilation PASS diagnostic target; Unit tests FAIL twice; Shader/SPIR-V NOT RUN; Vulkan NOT RUN; RDNA1 NOT RUN; Astro Bot NOT RUN. Counts unchanged:63 passing cases, one failed-only,361 never run. Prior failures retained.
- **Decision/status:** D-015/revision0.9; diagnostic gate complete, P0/P0.4 incomplete pending fix. Documentation/source diff checks before commit; no GPU/performance claim.
- **Next exact task/gate:** focused Windows precise CondWaitUntil fix with unchanged assertions, preserve wake/lock/dispatch semantics; focused two-job build and measured serial repeated deadline/early-signal passes, full provenance, commit and stop. No global timer/power change, unrelated batch/package/P1.

### Session S-009 — 2026-10-27 — strategic review and P1 static investigation

- **Branch/input:** feature/rdna1-mesh-fallback, 57348577135f7ae12ed01d7fefad15856be9b92e -> containing documentation commit. Clean input; recent eight commits reviewed; pin ancestry exit 0; 14 recursive submodules match. Sandbox Git status/submodule checks failed from metadata/MSYS permissions; outside-sandbox repeat PASS. No merge/rebase/push.
- **Selected task:** P1 investigation only, explicitly authorized by user in place of S-008's proposed timer fix. Read AGENTS/whole living plan/README and relevant release/diagnosis notes. Core compilation and 63 distinct passing cases suffice for static work; one failed-only and 361 unexecuted remain. No evidence timer issue blocks this work; defer repair and generic inventory expansion. No baseline declared stable.
- **Files:** new docs/rdna1-mesh-architecture.md supplies source map, NGG ABI, recompiler/pipeline/draw/capability/recording paths, output/memory semantics, risks, insertion hypothesis and proposed bounded GPU experiment. Plan and AGENTS synchronized; diagnosis gets superseding-priority pointer. No production/test/CMake source changes.
- **Commands/results:** rg/Get-Content source audit; git status --short clean; git log -8/-1, branch and submodule checks PASS outside sandbox; git rev-parse HEAD and git merge-base --is-ancestor pin HEAD PASS; source diff from pin shows only prior one-line stdout diagnostic. Some exploratory guessed paths/globs failed, corrected through rg --files/directory searches; report references verified in tree. git diff --check PASS after whitespace cleanup; source-reference existence/line-range and Markdown fence checks PASS. No benchmark or new test run.
- **Six categories this session:** Compilation NOT RUN (prior core PASS); Unit tests NOT RUN (prior thread_service FAIL retained); Shader compilation/SPIR-V validation NOT RUN (prior scoped passes retained); Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. No device/driver, screenshots, FPS/VRAM or duration metrics because no runtime experiment. Existing local S-008 probe executable still is not distributable final-source evidence.
- **Findings/limits:** explicit unsupported-mesh failure occurs at FinishMeshStage and graphics pipeline creation; native compute indirect converter still needs mesh rendering. NGG ABI, wave/LDS, allocation, packed indices/cull, fragment linkage and lifetimes require real lowering. Simple compute buffers or triangle drawing cannot prove PS5 compatibility. No Astro Bot shader subset established.
- **Decision/revision:** D-016/revision 1.0; P1 static exit met, independent review pending. P0 incomplete/deferred; package/CI/hardware gates retained. No fallback implementation. End this session after committing docs.
- **Next exact single task/gate:** P2.1 offline synthetic output-contract test only: fixed positions/indices/count/status layout, alignment/capacity/zero-output/overflow/guards; focused 2-job build and serial CPU pass with provenance. No dispatch, renderer wiring, guest translation or timer change; commit and stop.

### Template for S-010 onward
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
| 2026-10-27 | 0.5 | 1, 3–5, 7–8; AGENTS; inventory and new source audit | S-004 resolves all 82 deferred source dispositions and identifies host/hidden Vulkan dependencies. D-012 chooses next 16-case focused CPU batch; no execution gate advanced |
| 2026-10-27 | 0.6 | 1, 3–5, 7–8; AGENTS, inventory, audit follow-up, new batch evidence | S-005 selected build passed; retain 15/16 initial result and outside-sandbox failed-case retry pass plus version-label discrepancy. D-013 preserves environment/provenance limits; next 19-case CPU batch, baseline gate open |
| 2026-10-27 | 0.7 | 1, 3, 5, 7–8; AGENTS/inventory/new batch | S-006 19/19 focused CPU passes and clean version generation outside sandbox; retain all prior failures. Next seven host timer/thread cases; broader gate open |
| 2026-10-27 | 0.8 | 1, 3–5, 7–8; AGENTS/inventory/new batch | S-007 build passes, thread_service timing fails twice; retain six passes and both failures. D-014 prioritizes diagnosis over further batches |
| 2026-10-27 | 0.9 | 1,4–5,7–8; AGENTS/diagnosis/test stdout | S-008 measured coarse-phase oversleep, retains assertion failures and diagnostic-only change; D-015 selects focused wait fix |
| 2026-10-27 | 1.0 | 1,3–5,7–8; AGENTS; architecture/diagnosis | S-009 user strategic review defers timer repair/generic inventory, completes P1 static map, defines bounded prototype and P2.1 contract gate; no implementation or runtime claims |

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
