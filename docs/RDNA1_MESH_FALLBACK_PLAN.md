# RDNA1 Mesh Fallback — Living Project Plan

> **This is the project's durable memory.** It is a living plan, not a frozen specification. Keep it current in every Codex session, commit it alongside code changes, and do not pretend untested work is complete.
>
> **Last edited:** 2026-10-27 (initial scaffold). **Plan revision:** 0.1. **Overall status:** NOT STARTED — user must fork/clone and verify baseline.

## 0. Mission and boundaries

**Goal:** Investigate and, if feasible, implement a correctness-first optional Vulkan mesh-draw fallback for an AMD Radeon RX 5700 XT (RDNA1, 8 GiB VRAM), enabling KytyPS5 to use the real AMD GPU for ordinary graphics and compute rather than falling back to a CPU-only Vulkan ICD. Ultimate target: get meaningful Astro Bot (2024) game frames, then improve compatibility and performance. This is experimental: playable FPS is **not guaranteed**.

**Upstream:** `https://github.com/Jetsku/KytyPS5`

**Pinned starting point:** release `u59-windows-20261024-int21`, short commit `744705e` (verify exact full SHA locally). A user's GitHub fork and development branch `feature/rdna1-mesh-fallback` will be created from this tag, not from moving `main`. Do not silently rebase to a newer release.

**License:** Preserve GPL-2.0-only and all notices; respect dependency licenses.

**Developer test hardware:** Ryzen 5 7500F / AMD Radeon RX 5700 XT (8 GB) / 32 GB DDR5 / Windows. Local legally owned game files, title `PPSA21567`, Astro Bot version reported by launcher `01.019.000`, extracted to ~148.88 GB in `C:\Games\...`. Exact game path is *local-only* and should not be hardcoded or committed.

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
| Current subtask | P0.1 — user creates the fork, clones recursively, checks out pinned tag on feature branch |
| Status | `NOT_STARTED` |
| Last verified git commit | `unknown` — verify local full SHA at tag |
| Working branch | `feature/rdna1-mesh-fallback` (planned) |
| Build status | Not attempted |
| Native RDNA1 smoke test | Not run in this fork |
| Passing tests | None verified |
| Known blockers | GPU lacks `VK_EXT_mesh_shader`; architecture for correct fallback not yet confirmed |
| Next single actionable task | Verify repo, remotes, tag, branch, submodules, then inspect the Windows build instructions. Make NO renderer edits yet. |

### Definition of status
`NOT_STARTED` / `IN_PROGRESS` / `BLOCKED` / `DONE (with evidence)` / `ABANDONED (with reason)`.

Never change a phase to `DONE` without a command/test/log and its actual result. A compile passing is not the same as the game rendering.

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

## 3. Phases and acceptance gates

### P0 — Manual fork, pinned checkout, reproducible baseline
Status: **NOT_STARTED**

**Tasks:**
- P0.1: User creates a GitHub fork of Jetsku/KytyPS5; clones with submodules; adds `upstream`; fetches tags; creates/pushes feature branch from `u59-windows-20261024-int21`.
- P0.2: Record full `git rev-parse HEAD`, remotes, branch and submodule status, and verify license and original attribution.
- P0.3: Inventory toolchain (Git, CMake, Ninja, VS/Build Tools with clang-cl, Qt 6 MSVC x64 incl. Widgets/Network/Concurrent, glslang, Python). Use repo README and release workflow, not guessed flags.
- P0.4: Build an **unmodified pinned baseline**, ideally `launcher` / `kyty_emulator` and available tests. Use Release/Ninja/clang-cl and an isolated build directory. Prefer no PGO for the first correctness build. Stage/install executable and copy matching `tools/u59-preset.json` if required.
- P0.5: On RX 5700 XT verify GPU name, expected mesh-draw failure and capture sanitized last log lines. No gameplay success claimed.

**Gate:** Full SHA, clean submodules, build commands+results, exact GPU limitation, and rollback to the original official executable recorded. **No fallback code until P0 passes.**

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
- Validate generated SPIR-V with SPIRV-Tools and run a smoke test on RX 5700 XT if available.
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

**Gate:** A small representative guest mesh workload works with flag ON, OFF unchanged, all relevant automated tests passing.

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

## 5. Experiments and evidence

| ID | Environment / git SHA | Hypothesis and test | Actual observed result | Outcome / artifact |
| --- | --- | --- | --- | --- |
| E-000 | Pre-fork user test, Oct 2026 | Run on RDNA1 native with U59 | Error: `VK_EXT_mesh_shader` required | Baseline context only; needs local reproduction |
| E-001 | Pre-fork user test, Oct 2026 | Use Lavapipe Vulkan ICD | Splash visible; no confirmed menu/gameplay; CPU ~85% | CPU-only reference; not native GPU solution |

New experiments MUST include real date, commit SHA, driver, device, exact command/config, time budget, result (PASS/FAIL/INCONCLUSIVE), and logs/artifact location. Avoid adding game code/assets to git.

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

### Template for S-001 onward
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

## 9. Quick commands (reference; verify before running)

```powershell
# On user Windows PC, after manual GitHub fork:
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
