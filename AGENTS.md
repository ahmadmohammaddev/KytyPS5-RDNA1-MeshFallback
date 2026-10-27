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

## Current next task (checkpoint S-003)
S-003 retained the clean pinned source on `feature/rdna1-mesh-fallback` at build input `53bd13f21416d2dd67af05814a7781e2dc4c05bd`. The lightweight CPU/offline batch built at 2 jobs and passed 22/22 serial CTest cases (20 CPU helpers + 2 offline SPIR-V), zero failures/skips. All 425 registrations are named in `docs/RDNA1_BASELINE_TEST_INVENTORY.md`; 403 were not run. P0.4 remains incomplete: 82 cases need source/dispatch audit, one additional CPU case needs a full-emulator target build, and device/harness cases are excluded. See D-011/S-003 for the evidence-based split. Packaging and all GPU/game gates remain pending; remote fork/push state unchecked.

Next is **P0.4 source/dispatch audit only**: resolve the 82 deferred cases in the named inventory using actual registrations, main/argument branches and initialization paths; record CPU/offline vs device/host requirements and select the next bounded build batch. Acceptance: zero unresolved dispositions among those 82, exact source/dispatch evidence, and a named next batch. Keep already-confirmed CPU `null_texture_desc` pending its later full-emulator build. No build/tests, renderer/Mesh Shader changes, game runs, packaging or P1 in that audit subtask. Later builds retain 2 jobs (1 on memory exhaustion), serial tests, x64 VS 2022 shell and S-002 tool setup. Do not re-clone/recreate the branch. If checkout is missing, return to P0.1/P0.2 and provide precise missing setup commands.
