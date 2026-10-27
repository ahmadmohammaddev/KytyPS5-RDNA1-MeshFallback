# RDNA1 Mesh Fallback — Living Project Plan

> **This is the project's durable memory.** It is a living plan, not a frozen specification. Keep it current in every Codex session, commit it alongside code changes, and do not pretend untested work is complete.
>
> **Last edited:** 2026-10-27. **Plan revision:** 0.2. **Overall status:** P0 IN_PROGRESS — local checkout verified; two-machine workflow documented; baseline build and hardware validation not attempted.

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
| Current subtask | P0.2a — document two-machine workflow and baseline validation gates; DONE (documentation evidence in S-001) |
| Status | `IN_PROGRESS` for P0; no build or hardware gate complete |
| Last verified git commit | Session input `d1df5ff71d670174fe88191f182cfbf971dec94d`; S-001 documentation is identified by its containing commit (`git log -1` after commit) |
| Working branch | `feature/rdna1-mesh-fallback` (verified) |
| Checkout evidence | Clean initial status; pinned tag is an ancestor; all 14 recursive submodules match recorded commits; local origin/upstream URLs verified (S-001) |
| Build status | Not attempted |
| Native RDNA1 smoke test | Not run in this fork |
| Passing tests | None verified |
| Known blockers / limits | No toolchain inventory or baseline build; desktop tests pending; prior RX mesh failure needs reproduction; fallback architecture unconfirmed; remote GitHub fork/push state not checked |
| Next single actionable task | P0.3 only: inventory the laptop toolchain. Gate: record versions/availability of Git, CMake, Ninja, x64 VS/Build Tools, clang-cl, lld-link, llvm-lib, Python, glslang and Qt 6.8+ MSVC x64 (or explicit Qt-free choice), plus missing prerequisites. Do not build or edit renderer yet. |

### Definition of status
`NOT_STARTED` / `IN_PROGRESS` / `BLOCKED` / `DONE (with evidence)` / `ABANDONED (with reason)`.

Never change a phase to `DONE` without a command/test/log and its actual result. A compile passing is not the same as the game rendering.

| Validation category | Current evidence in this fork |
| --- | --- |
| Compilation | NOT RUN |
| Unit tests | NOT RUN |
| Shader compilation and SPIR-V validation | NOT RUN |
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

These are instructions for future P0 builds, **not executed build results**. Use an x64 Visual Studio developer shell with Ninja and clang-cl as documented in README. Default to **2 jobs**, preserve incremental outputs, close unnecessary memory-heavy applications and run only one build/test suite at a time. Intel Iris Xe shares system memory; 16 GB system RAM is not a dedicated GPU budget. On memory exhaustion, retain the failure log and retry the same build with **1 job**. If a single compile/link still exhausts memory, record the blocker and reassess configuration.

Source checks at S-001 input HEAD:

- Root `CMakeLists.txt` defines `KYTY_BUILD_LAUNCHER` (default ON), `KYTY_PGO_GENERATE` (OFF), `KYTY_PGO_USE` (empty), `KYTY_EMULATOR_IPO` (OFF), `KYTY_MARCH` (empty), `BUILD_TESTING` via CTest, and targets `kyty_emulator` and `kyty_tests`.
- The root only enters `src/launcher/CMakeLists.txt` when the launcher is enabled; that file requires Qt **6.8+** Concurrent/Network/Widgets. `KYTY_BUILD_LAUNCHER=OFF` therefore supports a Qt-free core/CLI build on Windows as well as the README's Linux example. This does not remove Vulkan/runtime dependencies or make all tests headless.
- Root CMake requires `glslangValidator` or `glslang` on PATH. `3rdparty/CMakeLists.txt` forces `SPIRV_SKIP_EXECUTABLES=ON`: do not assume a `spirv-val` executable is produced by this build; inventory a separate validator where needed.
- `src/generate_version.cmake` and `src/kytyGitVersion.h.in` already generate full `KYTY_GIT_REVISION` plus short/dirty labels. Verify generated identity for each build; do not invent a build-ID flag.

Example initial Qt-free configure/build recipe (verify tool availability in P0.3 first):

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
| Unit tests | Laptop / future CI, verified CPU-only subset | Inventory with `ctest --test-dir <build-dir> -N`; inspect registrations/source before selecting cases; run with `--output-on-failure --parallel 1`, record named tests/counts/failures/skips. No CPU-only selection verified yet. |
| Shader compilation and SPIR-V validation | Offline synthetic fixtures on laptop / future CI | Record compiler/SPIRV-Tools versions, fixture identity, exact Vulkan/SPIR-V target environment and validator results. Building glslang shaders alone does not validate all generated recompiler SPIR-V. |
| Vulkan integration tests | Available device; laptop explicitly Intel; desktop explicitly AMD | Record physical device, driver/ICD, features, validation-layer/synchronization settings and messages, correctness assertions and skips. The aggregate `kyty_tests` includes GPU-related executables; a full CTest run is not automatically a CPU unit-test result. |
| RDNA1 hardware tests | **RX 5700 XT desktop only** | Confirm native AMD ICD and selected physical device; archive phase-specific synthetic/rendering correctness, validation and performance evidence. Intel, hosted CI and Lavapipe cannot close this gate. |
| Astro Bot compatibility tests | **RX 5700 XT desktop only**, local installation | Record title/version, package identity, settings, duration and separate app-start/splash/new-frame/title/menu/gameplay outcomes, logs and screenshots. Synthetic RDNA1 success does not imply game compatibility. |

Hardware-dependent gates in P0.5 and P3–P8 remain incomplete until required tests run on RX 5700 XT. Generic Intel Vulkan integration is independent evidence. No RDNA1 milestone can be completed from laptop tests alone.

### 2.3 Build identity, future packaging and reproducible PC deployment

**Planned after the baseline is stable; no packaging script/workflow is implemented or validated this session.** Stable means P0.4 compilation passes and CPU/offline baseline results have failures/skips explained. P0.5 hardware baseline remains separate. Every local build, including failed attempts, records full `git rev-parse HEAD`, dirty/untracked state, submodule revisions, UTC build time, configuration and toolchain. Dirty builds retain a local diff plus untracked-source inventory/content; do not distribute them as clean commit builds.

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
Status: **IN_PROGRESS** — S-001 verifies local checkout and documents workflow; build/hardware work remains pending.

**Tasks:**
- P0.1: User creates a GitHub fork of Jetsku/KytyPS5; clones with submodules; adds `upstream`; fetches tags; creates/pushes feature branch from `u59-windows-20261024-int21`.
- P0.2: Record full `git rev-parse HEAD`, remotes, branch and submodule status, and verify license and original attribution.
- P0.2a: Document two-machine roles, laptop memory policy, validation categories and future packaging/deployment/CI gates. **DONE in S-001 (docs only).** P0.1/P0.2 local evidence verified; remote fork/push state unchecked.
- P0.3: Inventory laptop toolchain (Git, CMake, Ninja, x64 VS/Build Tools, clang-cl, lld-link, llvm-lib, Qt 6.8+ MSVC x64 incl. Widgets/Network/Concurrent or explicit Qt-free choice, glslang, Python and separate SPIR-V validator availability). Use inspected source/README/release workflow, not guessed flags.
- P0.4: Build a baseline with **unmodified pinned renderer/source** on the laptop (documentation commits allowed). Use Release/Ninja/clang-cl, no PGO, emulator IPO off, 2 jobs with 1-job memory retry; optionally Qt-free core/CLI. Record compilation, inspected CPU unit-test selection and offline shader/SPIR-V results separately; report unrun categories. Stable build does not imply hardware success.
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

## 5. Experiments and evidence

| ID | Environment / git SHA | Hypothesis and test | Actual observed result | Outcome / artifact |
| --- | --- | --- | --- | --- |
| E-000 | Pre-fork user test, Oct 2026 | Run on RDNA1 native with U59 | Error: `VK_EXT_mesh_shader` required | Baseline context only; needs local reproduction |
| E-001 | Pre-fork user test, Oct 2026 | Use Lavapipe Vulkan ICD | Splash visible; no confirmed menu/gameplay; CPU ~85% | CPU-only reference; not native GPU solution |
| E-002 | 2026-10-27; local checkout `d1df5ff71d670174fe88191f182cfbf971dec94d`; no GPU selected | Documentation/source audit: verify checkout and proposed flags/targets | PASS for local clean checkout, pinned ancestry, 14 matching recursive submodules, flags/targets and version helper; no configure/build/test execution | S-001 summaries and section 2.1 source references; driver/device/performance N/A (read-only audit) |

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

### Template for S-002 onward
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
