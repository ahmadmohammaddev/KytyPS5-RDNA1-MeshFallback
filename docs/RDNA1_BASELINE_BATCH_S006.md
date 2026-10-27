# P0.4 focused CPU batch — S-006

Source `4ead9cbe9143d4ebb59f6c688e7ffcef5e965091`, branch `feature/rdna1-mesh-fallback`, clean before/after execution. All 14 recursive submodules match; pinned source diff and ancestry checks passed. No source changes.

Build PASS, exit 0, 131.95 s, start UTC 2026-10-27T19:19:03.0751678Z; 85 Ninja actions, 2 jobs, no memory exhaustion/retry. Serial CTest **19/19 PASS, 0 failures/skips**, exit 0, 67.74 s. No retries. draw_prep took 65.18 s within 120 s cap.

Both build and test ran outside sandbox to avoid the known Git/temp-file restrictions from S-005. Generated header: full input SHA, short 4ead9cb without dirty suffix, version u59-windows-20261024-int21-6-g4ead9cbe. Old S-005 discrepancy/failure remains historical evidence; old emulator binary remains S-002. No package created.

S-002 x64 VS developer shell/local glslang PATH; rechecked CMake 3.31.6-msvc6, Ninja 1.12.1, LLVM/LLD 19.1.5 x64, VS 17.14.42, MSVC headers 14.44.35207, SDK 10.0.26100.0. Cached Release, testing ON, launcher OFF, PGO generate OFF/use empty, IPO OFF, empty MARCH. No configure rerun, full-emulator target, shader compiler/validator or GPU invocation. Nonfatal deprecation warnings retained. No peak memory measurement.

```bat
cmake --build _Build/windows-core --target scalar_provenance_tests lane_local_tests draw_prep_tests archive_file_tests trophy_system_tests avplayer_file_tests vdecsw_tests profiler_counter_tests shader_precompile_tests pipeline_list_tests pipeline_snapshot_tests resource_materialization_tests resource_tracking_tests audio_out2_port_tests audio_out_timing_tests ngs2_sampler_tests controller_settings_tests host_input_tests save_data_dialog_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --show-only=json-v1 -R '^(scalar_provenance|lane_local|draw_prep|archive_file|trophy_system|avplayer_file|vdecsw|profiler_counters|shader_precompile|pipeline_list|pipeline_snapshot|resource_materialization|resource_tracking|audio_out2_port|audio_out_timing|ngs2_sampler|controller_settings|host_input|save_data_dialog)$'
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 --output-junit <local-evidence>/results.xml -R '^(scalar_provenance|lane_local|draw_prep|archive_file|trophy_system|avplayer_file|vdecsw|profiler_counters|shader_precompile|pipeline_list|pipeline_snapshot|resource_materialization|resource_tracking|audio_out2_port|audio_out_timing|ngs2_sampler|controller_settings|host_input|save_data_dialog)$'
```

| Case | Result | Seconds |
| --- | --- | ---: |
| `scalar_provenance` | PASS | 0.078 |
| `lane_local` | PASS | 0.186 |
| `draw_prep` | PASS | 65.177 |
| `archive_file` | PASS | 0.163 |
| `trophy_system` | PASS | 0.149 |
| `avplayer_file` | PASS | 0.162 |
| `vdecsw` | PASS | 0.044 |
| `profiler_counters` | PASS | 0.589 |
| `shader_precompile` | PASS | 0.283 |
| `pipeline_list` | PASS | 0.273 |
| `pipeline_snapshot` | PASS | 0.045 |
| `resource_materialization` | PASS | 0.064 |
| `resource_tracking` | PASS | 0.045 |
| `audio_out2_port` | PASS | 0.116 |
| `audio_out_timing` | PASS | 0.079 |
| `ngs2_sampler` | PASS | 0.071 |
| `controller_settings` | PASS | 0.046 |
| `host_input` | PASS | 0.053 |
| `save_data_dialog` | PASS | 0.050 |

Internal skip-marker scan: 0 lines. Parent relevant KYTY/TRACY/VK_/SDL_ environment retained locally; no overrides found. Selection exactly 19. No build/test failures in this session.

Compilation PASS; Unit tests PASS (19-case subset); Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Across recorded sessions 57 distinct cases have passing executions, 368 never run. This is not a single 57-case run; S-005 initial failure retained.

Local ignored evidence: `_Build/p0-s006/` contains build.cmd/build.log/build-result.json, selection.json, selected.json, cache.txt, sha.txt/submodules.txt, before/after status/diff/untracked inventories, generated-version.h, environment.json, ctest.log/test-result.json/results.xml/LastTest.log, hashes.csv (19 executable SHA-256s) and documentation verification. No device/driver/ICD selected; screenshots/performance metrics N/A.

## Next single task

P0.4 host timer/thread batch only: build event_queue_lifetime_tests, event_queue_semantics_tests, thread_service_tests, sync_on_address_tests and hang_watchdog_tests at 2 jobs (1 only after retained memory exhaustion). Run exactly the seven cases listed in docs/RDNA1_BASELINE_BATCH_S006.md serially, 120 s cap with existing CTest properties retained. Gate: selection 7, build exit 0, all named outcomes/internal skips/failures/duration/provenance recorded; commit and stop. No VM/JIT/topology/GPU/full-emulator targets, packaging or P1.

```bat
cmake --build _Build/windows-core --target event_queue_lifetime_tests event_queue_semantics_tests thread_service_tests sync_on_address_tests hang_watchdog_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 -R '^(event_queue_lifetime|event_queue_semantics|thread_service|sync_on_address|hang_watchdog|hang_watchdog_fire|hang_watchdog_fatal_shutdown)$'
```

Use S-004 source audit for main/dispatch evidence. Watchdog fire/fatal cases retain their CMake 15-second timeout; preserve internal skips and native inspection/host permission failures. Use x64 VS/no PGO/no IPO, clean Git/submodule/toolchain identity, serial execution and retained hashes/logs. This task remains unexecuted in S-006.
