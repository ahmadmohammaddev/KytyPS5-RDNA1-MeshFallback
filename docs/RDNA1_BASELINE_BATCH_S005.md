# P0.4 focused CPU batch — S-005

Build/test source: `073eefecbc4f708d289439c6e3591d5a39d7a367`, `feature/rdna1-mesh-fallback`. Outside-sandbox Git checks before and after build show clean tracked/untracked source and all 14 matching recursive submodules. Source/tests/CMake/third party match the pinned tag. No renderer or test source changes.

**Result:** build PASS; initial serial CTest **15/16 PASS, 1 FAIL, 0 skipped** (6.55 s, exit 8). `live_switch` alone then PASS outside sandbox (1.55 s case, 1.58 s CTest total, exit 0). This is not a single clean 16/16 run. All 16 have a passing execution, with the failed attempt retained. P0.4 stays open.

## Commands and environment

Same x64 VS 2022 developer shell and local glslang PATH as S-002. CMake 3.31.6-msvc6, Ninja 1.12.1, LLVM/LLD 19.1.5 x64, VS developer environment 17.14.42, MSVC headers 14.44.35207, SDK 10.0.26100.0. Cached Release/testing ON, launcher OFF, PGO generate OFF/use empty, emulator IPO OFF, empty MARCH. No configure rerun, emulator target, full-emulator-test target, shader compiler or validator invocation. No inherited KYTY/TRACY/VK_/SDL_ variables were found in the parent test environment. CTest registration properties retained.

```bat
cmake --build _Build/windows-core --target occlusion_reset_tests ime_dialog_tests image_page_table_tests texel_image_lookup_tests texture_set_cache_tests clear_register_decode_tests eop_timestamp_tests binding_path_tests cp_recorder_tests cp_sequencer_tests live_switch_tests u59_preset_tests shader_vertex_metadata_tests audio_propagation_tests png_enc_tests ces_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --show-only=json-v1 -R '^(occlusion_reset|ime_dialog|image_page_table|texel_image_lookup|texture_set_cache|clear_register_decode|eop_timestamp|binding_path|cp_recorder|cp_sequencer|live_switch|u59_preset|shader_vertex_metadata|audio_propagation|png_enc|ces)$'
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 --output-junit <local-evidence>/results.xml -R '^(occlusion_reset|ime_dialog|image_page_table|texel_image_lookup|texture_set_cache|clear_register_decode|eop_timestamp|binding_path|cp_recorder|cp_sequencer|live_switch|u59_preset|shader_vertex_metadata|audio_propagation|png_enc|ces)$'
# Outside sandbox, only the failed case:
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 --output-junit <local-evidence>/live-switch-retry.xml -R '^live_switch$'
```

Build exit 0, 46.57 s; UTC 2026-10-27T19:12:58.8793562Z to 2026-10-27T19:13:45.4443596Z. 47 Ninja actions, two jobs, no observed memory exhaustion/one-job retry. Nonfatal CRT deprecation/offsetof warnings retained. Selection inventory verified exactly 16, and all 16 executable commands exist after build. Tests run one at a time. No internal skip markers found in the retained initial LastTest.log.

| Case | Initial result | Seconds | Retry |
| --- | --- | ---: | --- |
| `occlusion_reset` | PASS | 0.060 | Not repeated |
| `ime_dialog` | PASS | 0.057 | Not repeated |
| `image_page_table` | PASS | 0.059 | Not repeated |
| `texel_image_lookup` | PASS | 0.186 | Not repeated |
| `texture_set_cache` | PASS | 0.267 | Not repeated |
| `clear_register_decode` | PASS | 0.054 | Not repeated |
| `eop_timestamp` | PASS | 0.079 | Not repeated |
| `binding_path` | PASS | 0.062 | Not repeated |
| `cp_recorder` | PASS | 4.970 | Not repeated |
| `cp_sequencer` | PASS | 0.090 | Not repeated |
| `live_switch` | FAIL | 0.270 | PASS outside sandbox, 1.55 s |
| `u59_preset` | PASS | 0.100 | Not repeated |
| `shader_vertex_metadata` | PASS | 0.058 | Not repeated |
| `audio_propagation` | PASS | 0.050 | Not repeated |
| `png_enc` | PASS | 0.060 | Not repeated |
| `ces` | PASS | 0.051 | Not repeated |

## Failure and provenance limits

- `live_switch`: `LiveSwitchTests: failed: rename over the live file`, exit `0xc0000409`. `tests/LiveSwitchTests.cpp:247` writes a temporary file then uses `std::filesystem::rename` over the watched path; `src/common/liveSwitch.cpp:182` opens the reader with delete/read/write sharing. The assertion reports no OS error code and may occur on any fixture replacement, so the precise failing rename/cause was not established. An unchanged-binary isolated outside-sandbox retry passed. A sandbox temp-file restriction is an inference, not a proven source defect or timing diagnosis. No workaround or source patch was applied; no full-suite rerun.
- The build generated `KYTY_GIT_REVISION` with the full input SHA but `KYTY_GIT_VERSION="unknown"` and short hash `073eefe-dirty`. `src/generate_version.cmake` suppresses Git stderr and treats any nonzero diff-index result as dirty. Outside sandbox, `git diff-index --quiet HEAD --` exited 0 and `git describe --tags --always --dirty` returned `u59-windows-20261024-int21-5-g073eefec`. Restricted submodule metadata is the likely explanation, but the build’s exact suppressed Git error was not captured. Retain the original header; do not relabel these outputs as clean packages. Local empty before/after diffs and untracked inventories establish actual source state. No rebuild merely to change the label; emulator executable remains the older S-002 binary.
- An initial empty environment pipeline produced no JSON file; rewrote it as explicit `[]`, confirming no relevant parent overrides. No test/build failure other than the recorded live-switch attempt. No shader/Vulkan/RDNA1/game evidence or hardware performance inference.

## Six independent categories

| Category | This session |
| --- | --- |
| Compilation | PASS, 16 focused targets, exit 0; generated label discrepancy retained |
| Unit tests | Initial 15/16 PASS, live_switch FAIL; isolated retry 1/1 PASS outside sandbox |
| Shader compilation and SPIR-V validation | NOT RUN |
| Vulkan integration tests | NOT RUN |
| RDNA1 hardware tests | NOT RUN |
| Astro Bot compatibility tests | NOT RUN |

## Local evidence

Ignored `_Build/p0-s005/`: selection.json, build.cmd/build.log/build-result.json, CMakeCache-before.txt, source-sha.txt, submodules.txt, status-before.txt/source.diff/untracked.txt, status-after-build.txt/source-after-build.diff/untracked-after-build.txt, generated-version.h, test-environment-before.json, selected-inventory.json and selected-inventory-after.json, ctest.log/ctest-result.json/results.xml/LastTest.log, live-switch-retry.log/-result.json/-detail.log and retry XML, test-artifact-hashes.csv (SHA-256 for all 16 executables), next-batch.json and documentation verification scripts/results. Paths/hostnames/raw output stay local. No binaries, settings, private assets or packages committed.

## Next single task: remaining 19 focused CPU cases

Use the S-004 source audit for evidence. These are the remaining CPU/offline registrations that avoid full-emulator targets after this 16-case batch. Host stress/topology tests, 13 shader CFG cases, full-emulator-linked CPU tests and 317 conservatively mixed-harness registrations remain separate. Do not run this next batch in S-005.

Build at two jobs (retain memory failure and retry one only on exhaustion); run serially with a 120 s per-test cap. Record actual source/header identity, toolchain/cache flags, clean/dirty provenance, SHA-256s, selected names and all outcomes. Preserve internal skips and sandbox failures; allow a bounded failed-case retry outside sandbox when justified, without rewriting initial results.

```bat
cmake --build _Build/windows-core --target scalar_provenance_tests lane_local_tests draw_prep_tests archive_file_tests trophy_system_tests avplayer_file_tests vdecsw_tests profiler_counter_tests shader_precompile_tests pipeline_list_tests pipeline_snapshot_tests resource_materialization_tests resource_tracking_tests audio_out2_port_tests audio_out_timing_tests ngs2_sampler_tests controller_settings_tests host_input_tests save_data_dialog_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 -R '^(scalar_provenance|lane_local|draw_prep|archive_file|trophy_system|avplayer_file|vdecsw|profiler_counters|shader_precompile|pipeline_list|pipeline_snapshot|resource_materialization|resource_tracking|audio_out2_port|audio_out_timing|ngs2_sampler|controller_settings|host_input|save_data_dialog)$'
```

Gate: exact selection 19, build exit 0, all named outcomes/failures/skips/duration/provenance documented; commit checkpoint and stop. Any recurring failure becomes the next focused task, not an excuse to suppress assertions. No renderer changes, full-emulator-target builds, packaging or P1.
