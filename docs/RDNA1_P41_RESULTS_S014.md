# P4.1 repairs and Vulkan evidence — S-014, 2026-10-28

The eight S-013 findings are fixed in the existing implementation. The original work was preserved before editing and extended in place. No production emulator/renderer or guest translation code changed. **Vulkan 1.2 auxiliary execution passes on Intel; the strict Vulkan 1.3 runtime test is skipped on this device. P4.1's Vulkan 1.3 runtime gate remains open.**

## Source and build identity

Branch: `feature/rdna1-mesh-fallback`. Build input: `ad25d9ab74dee6de0a710bd32787a41e99d50b95` plus retained dirty source; final source is in the containing commit. Both compilation attempts used the existing VS BuildTools 17.14.42 x64 environment, clang-cl/LLD 19.1.5, CMake 3.31.6-msvc6, Ninja 1.12.1 and locally retained glslang 16.6.0. Cache: Release, BUILD_TESTING ON, launcher OFF, PGO generation OFF/use empty, emulator IPO OFF, empty MARCH. Two build jobs, no memory exhaustion or one-job retry. No merge/rebase/push or package generation.

Before edits, `_Build/p4-s014/pre-edit/` captured the interrupted work. Before each build, `pre-build-01/` and `pre-build-02/` retained complete HEAD/staged binary patches, raw modified/untracked file contents, SHA-256 inventories, full base SHA/branch/UTC time, recursive submodule revisions/cleanliness and CMake cache. All 14 submodules match, pinned ancestry passes. Final test-source hashes match the built snapshot; final documentation snapshot is retained separately. Original S-013 logs/snapshots were not overwritten. Generated version identity is the full input SHA with `ad25d9a-dirty`; standalone tests do not embed the shared version header.

## Fixes

| S-013 finding | Implemented correction | Verification |
| --- | --- | --- |
| 1. Device lookup | Resolve `vkCreateDevice` and `vkGetDeviceProcAddr` with the live instance; missing required entry points fail | Real logical device, queue, pipeline and dispatch created on Intel |
| 2. Buffer bookkeeping | Record requested and allocation extents; rank only memory types allowed by each buffer's requirements | Actual allocations: output 1840 bytes, input 1792 bytes; host regression excludes an incompatible preferred type and tests no compatible type |
| 3. Scenarios | Nine scenarios; no-dispatch expected blob stays Pending; add index-overflow recovery and retain actual GPU output for both recovery cases | Host fixture checks pass; all nine complete with exact full-buffer comparisons |
| 4. Host visibility | Record COMPUTE_SHADER/SHADER_WRITE to HOST/HOST_READ barrier after each dispatch; fence before host access; flush/invalidate when noncoherent | Core + synchronization validation reports zero errors; actual coherent-memory readback matches |
| 5. Validation availability/result | Require Khronos layer, debug-utils collector and synchronization-validation extension; collect instance creation/destruction messages too; API failures fail | Loaded core/sync layer; no-layer experiment correctly skips; normal run has zero validation errors/warnings |
| 6. Cleanup | Destroy shader module, use common instance cleanup, wait for device idle before teardown, check validation errors after cleanup; atomic callback counters | Post-cleanup summary: failures=0, validation-errors=0, validation-warnings=0 |
| 7. Offline false pass | Include shape assertions in exit status; direct single-module validation mode; negative CTest fixture | Valid Vulkan 1.3 SPIR-V with payload binding changed to 2 passes SPIRV-Tools but direct checker exits 1; wrapper test passes only for that expected rejection |
| 8. Skip mismatch | Normalize all genuine unavailable-environment results to CTest skip code 41 | Unsupported Vulkan 1.3 and both absent-layer runtime cases reported as skipped; no code 42 path remains |

## Executed commands and results

After the pre-build capture, an x64 VS developer shell ran:

```text
cmake --build _Build/windows-core --target KytyGitVersion --parallel 2
cmake --build _Build/windows-core --target mesh_output_contract_tests mesh_compute_proto_tests --parallel 2
```

Both builds PASS exit 0. Final build duration 13.23 s including tool versions, CMake regeneration and version refresh. The final build regenerated both authored shader modules with glslang: `--target-env vulkan1.3` (SPIR-V 1.6) and `--target-env vulkan1.2` (SPIR-V 1.5). Both passed pinned SPIRV-Tools validation and structural checks. No full emulator/GUI build or broad suite.

Validation-layer discovery was configured only for the child process, pointing `VK_LAYER_PATH` at the retained local layer directory under `_Build/p4-s013/tools/validationlayers`. `VK_INSTANCE_LAYERS` was unset: the program explicitly enables the layer, avoiding redundant-loader warnings. No `VK_DRIVER_FILES` or `VK_ICD_FILENAMES` override. No system configuration or driver change. Layer manifest API 1.4.366; local archive SHA-256 `0CC7C7275296D4FDD904D664F85E246DEBBADEA10B26BEBA9FFAC7EF4BC3CC96`. Archive/digest are local identity observations, not a new download/authenticity verification.

```text
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 -V --output-junit ../p4-s014/ctest-final.xml -R "^(mesh_output_contract|mesh_proto_spirv|mesh_proto_host_fixtures|mesh_proto_reject_shape|mesh_compute_proto|mesh_compute_proto_vk12)$"
```

| Registered test | Actual result | Duration |
| --- | --- | --- |
| `mesh_output_contract` | PASS | 0.01 s |
| `mesh_proto_spirv` | PASS — both target environments | 0.09 s |
| `mesh_proto_host_fixtures` | PASS — scenario/recovery and memory-selection regressions | 0.03 s |
| `mesh_proto_reject_shape` | PASS — direct negative checker correctly returns FAIL/exit 1 | 0.33 s |
| `mesh_compute_proto` (`--run`) | SKIPPED — no Vulkan 1.3 compute device; exit 41 | 0.18 s |
| `mesh_compute_proto_vk12` (`--run-vulkan12`) | PASS — actual Intel dispatch and exact readback | 0.23 s |

CTest aggregate exit 0, total 0.99 s: **5 PASS, 1 SKIPPED, 0 unexpected FAIL**. The negative module's FAIL is intentional and asserted by its wrapper. The earlier four-test run also passed 3/skipped 1, with two loader warnings from redundant `VK_INSTANCE_LAYERS`; retained in `test-01.log`. Final run has no validation warnings or errors.

Separately, setting only the child process's `VK_LAYER_PATH` to an empty local directory ran the two runtime registrations: **2 SKIPPED**, total 0.16 s, CTest exit 0. This verifies absent-layer skip semantics, not execution success. Restore the normal local layer path for actual dispatch; no persistent environment change was made.

## Actual device execution

Loader API 1.4.313. Selected native device: Intel Iris Xe, integrated GPU, vendor `0x8086`, device `0x9A49`, device API **1.2.172**, raw driver version `0x0019255D`, driver ID 5/name Intel Corporation, conformance 1.2.2. Queue family 0. Output and payload allocations used host-visible coherent memory type 1 (`noncoherent=0`).

Vulkan 1.2 module dispatched in one 1x1x1 workgroup per dispatch. Each of these nine cases compares all 460 words (1840 bytes), including status/counts, unused payload and eight guards:

1. Pending initialization upload, without dispatch.
2. Zero output.
3. Three synthetic positions and indices 0/1/2, counts 3/1.
4. Exact 64-vertex/64-primitive capacity.
5. Vertex overflow, preserving every non-status word of a nonzero snapshot.
6. Index overflow with the same preservation requirement.
7. Recovery to exact capacity on the actual preceding index-overflow GPU buffer, without reuploading output.
8. Combined overflow, reporting vertex overflow first.
9. Recovery to exact capacity on the actual preceding combined-overflow GPU buffer, without reuploading output.

This is eight actual compute dispatches and one initialization/submission check. Fence waits are bounded to 10 s per case; cleanup waits for pending device work before destroying resources. Final summary after all teardown: **9 scenarios, 0 failures, 0 validation errors, 0 validation warnings**. These are synthetic correctness timings under validation, not FPS/throughput benchmarks; GPU utilization, VRAM peak and screenshots were not measured for this buffer-only test.

## Six categories, limits and next task

| Category | Status |
| --- | --- |
| Compilation | PASS — two focused targets; core/GUI not rebuilt |
| Unit tests | PASS — host contract and host fixture checks; no full-suite claim |
| Shader compilation and SPIR-V validation | PASS — both authored variants plus expected valid-but-wrong-shape rejection |
| Vulkan integration | PASS for explicit auxiliary Vulkan 1.2/Intel; strict Vulkan 1.3 SKIPPED |
| RDNA1 hardware | NOT RUN |
| Astro Bot compatibility | NOT RUN |

The driver exposes no Vulkan 1.3 device. Noncoherent flush/invalidation code is present, but runtime noncoherent allocation was not exercised on this device. Failure/timeout cleanup paths are source checked; no deliberate GPU hang/device loss was induced. The deferred timer/baseline issue is unchanged. No renderer integration, guest translation, textured draw, game, deployment or distributable package evidence.

Next single task remains P4.1: execute the strict Vulkan 1.3 case on a qualifying device, preferably the separately identified RX 5700 XT desktop. Use a build from the exact committed source with recorded build identity/toolchain and validation layers; the laptop's dirty-build binary is not a deployment package. Acceptance: actual Vulkan 1.3 module dispatch, all nine exact blob comparisons, zero validation errors including cleanup, native device/driver/ICD identified and commands/logs/artifacts retained. Missing device/layer is a skip, never a pass. P4 rendering and all game/hardware milestones stay open.
