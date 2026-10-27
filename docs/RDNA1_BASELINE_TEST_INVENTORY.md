# P0.4 regression inventory — S-003

Source: `53bd13f21416d2dd67af05814a7781e2dc4c05bd`, clean, pinned renderer unchanged. Generated root `CTestTestfile.cmake` contains 425 cases. Source registrations are in root `CMakeLists.txt`; names below retain CTest spelling. This is a partial source audit, not a completed CPU-only classification or full-suite pass.

| Disposition | Count | Execution |
| --- | ---: | --- |
| Confirmed CPU helpers | 20 | 20 PASS |
| Confirmed offline SPIR-V | 2 | 2 PASS |
| Vulkan/mixed executable | 318 | Excluded, NOT RUN |
| Confirmed CPU, full-emulator target deferred | 1 | NOT RUN: `null_texture_desc` |
| Harness/setup | 2 | Excluded, NOT RUN |
| Deferred source/argument-dispatch audit | 82 | NOT RUN; may contain additional CPU/offline cases |

The 318-case exclusion is conservative at executable level: 317 registrations use `ShaderRecompilerComputeTests.cpp` (a Vulkan harness with many argument branches), and one `SparseResidencyStressTests.cpp`. It does not assert every branch requires a GPU. Source/dispatch inspection may later move individual cases into an offline batch. `null_texture_desc` was checked separately: its main calls descriptor helpers and creates no Vulkan device, but CMake compiles the full emulator source set for its target; it is deferred to a separate build batch. The two CMake commands clean a journal and check failure-message matching; do not count them as unit or GPU validation.

## Confirmed batch and evidence

| Case | Source | Reason |
| --- | --- | --- |
| `occlusion_pairs` | `tests/OcclusionPairsTests.cpp` | Synthetic address-pair bookkeeping; no query pool/device. |
| `occlusion_query_state` | `tests/OcclusionQueryStateTests.cpp` | Query-state model and reentrant end accounting; no Vulkan calls. |
| `image_cache_gc_policy` | `tests/ImageCacheGcPolicyTests.cpp` | Budget arithmetic and CPU LRU scan/reuse. |
| `image_clear_range` | `tests/ImageClearRangeTests.cpp` | Synthetic aspect/mip/layer/volume clear-range checks. |
| `image_gpu_keep` | `tests/ImageGpuKeepTests.cpp` | Kept/dirty address-range arithmetic on synthetic lists. |
| `ampr_counter_bank` | `tests/AmprCounterBankTests.cpp` | Synthetic counter storage, lane decoding and integer operations. |
| `host_input_pulse` | `tests/HostInputPulseTests.cpp` | Synthetic key timestamps and callback event vector. |
| `range_set` | `tests/RangeSetTests.cpp` | CPU range semantics and allocation contract, default no-argument mode. |
| `bit_array` | `tests/BitArrayTests.cpp` | Boundary and randomized differential bit/range checks. |
| `lod_stats_report` | `tests/LodStatsReportTests.cpp` | Synthetic descriptor fields, sample counters and report packing. |
| `game_patch_filter` | `tests/GamePatchFilterTests.cpp` | Filter parsing/matching/version selection and local temporary fixture; no game files. |
| `lru_cache` | `tests/LruCacheTests.cpp` | CPU insert/touch/free/reuse bookkeeping. |
| `spirv_local_arrays` | `tests/SpirvLocalArraysTests.cpp` | Synthetic assembly, shrink and validation via pinned SPIRV-Tools, SPV_ENV_VULKAN_1_3. |
| `spirv_cache_salt` | `tests/SpirvCacheSaltTests.cpp` | Synthetic assembly, ID salting, disassembly comparison and validation, SPV_ENV_VULKAN_1_3. |
| `pipeline_compile_queue` | `tests/PipelineCompileQueueTests.cpp` | CPU worker callbacks, admission, priority, drain and error publication; no driver compiler. |
| `shader_code_snapshot` | `tests/ShaderCodeSnapshotTests.cpp` | Synthetic byte vectors, hash identity and stale-read rejection. |
| `pipeline_fast_first` | `tests/PipelineFastFirstTests.cpp` | CPU scheduler, retire list and statistics; no Vulkan pipeline creation. |
| `vram_budget` | `tests/VramBudgetTests.cpp` | Synthetic budget/collection marks; no device budget query. |
| `device_compat` | `tests/DeviceCompatTests.cpp` | Synthetic capabilities/breadcrumb decisions; Vulkan headers only. |
| `audio_diag` | `tests/AudioDiagTests.cpp` | Diagnostic budgets and synthetic readable-address callbacks; no audio device. |
| `audio_mix` | `tests/AudioMixTests.cpp` | CPU mixing/gain/object-panning arithmetic; no audio device. |
| `unresolved_import_log` | `tests/UnresolvedImportLogTests.cpp` | Pure first-call/power-of-ten reporting decisions. |

## Reproduction

Use the S-002 x64 VS 2022 developer shell and project-local glslang PATH. Reuse `_Build/windows-core` with Release, launcher OFF, testing ON, PGO generation OFF/use empty, emulator IPO OFF, portable CPU baseline empty.

```bat
cmake --build _Build/windows-core --target occlusion_pairs_tests occlusion_query_state_tests image_cache_gc_policy_tests image_clear_range_tests image_gpu_keep_tests ampr_counter_bank_tests host_input_pulse_tests range_set_tests bit_array_tests lod_stats_report_tests game_patch_filter_tests lru_cache_tests spirv_local_arrays_tests spirv_cache_salt_tests pipeline_compile_queue_tests shader_code_snapshot_tests pipeline_fast_first_tests vram_budget_tests device_compat_tests audio_diag_tests audio_mix_tests unresolved_import_log_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 60 -R '^(occlusion_pairs|occlusion_query_state|image_cache_gc_policy|image_clear_range|image_gpu_keep|ampr_counter_bank|host_input_pulse|range_set|bit_array|lod_stats_report|game_patch_filter|lru_cache|spirv_local_arrays|spirv_cache_salt|pipeline_compile_queue|shader_code_snapshot|pipeline_fast_first|vram_budget|device_compat|audio_diag|audio_mix|unresolved_import_log)$'
```

Build exit 0 (44 Ninja actions, log UTC 2026-10-27 18:47:40–18:47:57; file timestamps, not precise process timing); no memory exhaustion. CTest exit 0, 22/22 PASS, zero failed/skipped, 1.69 s. No configure rerun or emulator rebuild/launch. Existing S-002 emulator binary retains its older SHA; selected tests were built from the source SHA above. Toolchain rechecked: VS 17.14.42, LLVM 19.1.5 x64, CMake 3.31.6-msvc6, Ninja 1.12.1, MSVC headers 14.44.35207, SDK 10.0.26100.0. Nonfatal CRT deprecation warnings retained.

Local ignored evidence: `_Build/p0-s003/` contains build/toolchain scripts and logs, result files, inventory JSON, selection regex, clean-source SHA/status/diff/untracked inventory and all recursive submodule revisions. CTest detail remains in `_Build/windows-core/Testing/Temporary/LastTest.log`. No device, ICD or driver selected; screenshots/FPS/VRAM metrics N/A for CPU/offline tests.

## All named registrations

`Deferred` means unverified execution requirements, not a skip or a pass. Commands preserve arguments but omit local executable paths; CMake registration lines trace to root source.

| Case | Executable/command | Arguments | Disposition | Registration |
| --- | --- | --- | --- | --- |
| `occlusion_reset` | `occlusion_reset_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1082` |
| `occlusion_pairs` | `occlusion_pairs_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1086` |
| `occlusion_query_state` | `occlusion_query_state_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1092` |
| `emulator_user_name_cli` | `kyty_emulator` | `"--help" "--user-name" "1234567890123456"` | Deferred source/dispatch audit | `CMakeLists.txt:1165` |
| `emulator_user_name_too_long` | `kyty_emulator` | `"--help" "--user-name" "12345678901234567"` | Deferred source/dispatch audit | `CMakeLists.txt:1167` |
| `emulator_present_mode_cli` | `kyty_emulator` | `"--help" "--present-mode" "Immediate"` | Deferred source/dispatch audit | `CMakeLists.txt:1170` |
| `emulator_present_mode_invalid` | `kyty_emulator` | `"--help" "--present-mode" "Invalid"` | Deferred source/dispatch audit | `CMakeLists.txt:1172` |
| `ime_dialog` | `ime_dialog_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1175` |
| `upsync2_shader_ir` | `shader_cfg_tests` | `"--upsync2-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1176` |
| `upsync2_shader_ir_linear_uses` | `shader_cfg_tests` | `"--upsync2-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1179` |
| `wave_reductions_linear_uses` | `shader_cfg_tests` | `"--wave-reduction-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1182` |
| `isa_accuracy_linear_uses` | `shader_cfg_tests` | `"--isa-accuracy-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1185` |
| `upsync2_opcodes` | `shader_recompiler_compute_tests` | `"--new-opcodes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1188` |
| `shader_cfg` | `shader_cfg_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1189` |
| `shader_cfg_cb_shader_mask_exports` | `shader_cfg_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1191` |
| `null_texture_desc` | `null_texture_desc_tests` | `(none)` | CPU confirmed; full-emulator build deferred | `CMakeLists.txt:1193` |
| `sparse_residency_stress` | `sparse_residency_stress_tests` | `"--iterations" "400"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1195` |
| `unused_pixel_inputs` | `shader_cfg_tests` | `"--unused-pixel-inputs-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1196` |
| `pixel_interface` | `shader_cfg_tests` | `"--pixel-interface-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1197` |
| `registered_shader_code_on` | `shader_cfg_tests` | `"--registered-shader-code-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1198` |
| `registered_shader_code_off` | `shader_cfg_tests` | `"--registered-shader-code-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1200` |
| `registered_shader_code_cert` | `shader_recompiler_compute_tests` | `"--draw-prep-code-cert-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1202` |
| `scalar_provenance` | `scalar_provenance_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1204` |
| `lane_local` | `lane_local_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1205` |
| `image_page_table` | `image_page_table_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1206` |
| `texel_image_lookup` | `texel_image_lookup_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1207` |
| `texture_set_cache` | `texture_set_cache_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1208` |
| `image_cache_gc_policy` | `image_cache_gc_policy_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1209` |
| `image_clear_range` | `image_clear_range_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1210` |
| `image_gpu_keep` | `image_gpu_keep_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1211` |
| `clear_register_decode` | `clear_register_decode_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1212` |
| `ampr_counter_bank` | `ampr_counter_bank_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1213` |
| `draw_prep` | `draw_prep_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1214` |
| `eop_timestamp` | `eop_timestamp_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1215` |
| `host_input_pulse` | `host_input_pulse_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1216` |
| `repeat_trace` | `repeat_trace_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1217` |
| `binding_path` | `binding_path_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1218` |
| `cp_recorder` | `cp_recorder_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1219` |
| `cp_sequencer` | `cp_sequencer_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1220` |
| `image_exact_lookup` | `shader_recompiler_compute_tests` | `"--image-exact-lookup-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1221` |
| `image_pressure_retirement` | `shader_recompiler_compute_tests` | `"--image-pressure-retirement-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1223` |
| `present_box_downscale` | `shader_recompiler_compute_tests` | `"--present-box-downscale-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1225` |
| `range_set` | `range_set_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1227` |
| `image_gpu_keep_device` | `shader_recompiler_compute_tests` | `"--gpu-keep-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1228` |
| `memory_tracker` | `memory_tracker_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1230` |
| `page_manager` | `page_manager_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1231` |
| `archive_file` | `archive_file_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1232` |
| `trophy_system` | `trophy_system_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1233` |
| `avplayer_file` | `avplayer_file_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1234` |
| `vdecsw` | `vdecsw_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1235` |
| `bit_array` | `bit_array_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1236` |
| `hang_watchdog` | `hang_watchdog_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1237` |
| `hang_watchdog_fire` | `hang_watchdog_tests` | `"--fire"` | Deferred source/dispatch audit | `CMakeLists.txt:1238` |
| `hang_watchdog_fatal_shutdown` | `hang_watchdog_tests` | `"--fatal-shutdown"` | Deferred source/dispatch audit | `CMakeLists.txt:1239` |
| `profiler_counters` | `profiler_counter_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1241` |
| `cpu_placement` | `cpu_placement_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1242` |
| `cpu_placement_recorder` | `cpu_placement_tests` | `"--mode" "cp+recorder"` | Deferred source/dispatch audit | `CMakeLists.txt:1243` |
| `cpu_placement_vcache_sets` | `cpu_placement_tests` | `"--vcache" "sets"` | Deferred source/dispatch audit | `CMakeLists.txt:1244` |
| `cpu_placement_vcache_affinity` | `cpu_placement_tests` | `"--vcache" "affinity"` | Deferred source/dispatch audit | `CMakeLists.txt:1245` |
| `cpu_placement_vcache_pending` | `cpu_placement_tests` | `"--vcache" "pending"` | Deferred source/dispatch audit | `CMakeLists.txt:1247` |
| `live_switch` | `live_switch_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1249` |
| `guest_probe_retire` | `guest_probe_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1250` |
| `guest_probe_shared` | `guest_probe_tests` | `"--shared"` | Deferred source/dispatch audit | `CMakeLists.txt:1251` |
| `guest_probe_equals` | `guest_probe_tests` | `"--equals"` | Deferred source/dispatch audit | `CMakeLists.txt:1252` |
| `lod_stats_report` | `lod_stats_report_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1254` |
| `game_patch_filter` | `game_patch_filter_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1255` |
| `u59_preset` | `u59_preset_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1256` |
| `shader_codegen` | `shader_recompiler_compute_tests` | `"--codegen-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1257` |
| `lod_stats_codegen` | `shader_recompiler_compute_tests` | `"--lod-stats-codegen-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1259` |
| `gi_probe_codegen` | `shader_recompiler_compute_tests` | `"--gi-probe-codegen-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1261` |
| `gi_probe` | `shader_recompiler_compute_tests` | `"--gi-probe-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1263` |
| `s_memrealtime_clock` | `shader_recompiler_compute_tests` | `"--s-memrealtime-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1266` |
| `dispatcher_cap` | `shader_recompiler_compute_tests` | `"--dispatcher-cap-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1269` |
| `wave32_clusters` | `shader_recompiler_compute_tests` | `"--wave32-clusters-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1274` |
| `wave32_clusters_shader_cases` | `shader_recompiler_compute_tests` | `"--shader-cases-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1276` |
| `wave32_clusters_wave_branch` | `shader_recompiler_compute_tests` | `"--wave-branch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1278` |
| `graphics_buffer_add_tid` | `shader_recompiler_compute_tests` | `"--graphics-add-tid-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1282` |
| `unmapped_buffer_null` | `shader_recompiler_compute_tests` | `"--unmapped-buffer-null-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1285` |
| `lds_device_buffer` | `shader_recompiler_compute_tests` | `"--lds-device-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1291` |
| `lds_device_buffer_shader_cases` | `shader_recompiler_compute_tests` | `"--shader-cases-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1293` |
| `lru_cache` | `lru_cache_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1296` |
| `spirv_local_arrays` | `spirv_local_arrays_tests` | `(none)` | Offline SPIR-V PASS | `CMakeLists.txt:1297` |
| `spirv_cache_salt` | `spirv_cache_salt_tests` | `(none)` | Offline SPIR-V PASS | `CMakeLists.txt:1298` |
| `pipeline_compile_queue` | `pipeline_compile_queue_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1299` |
| `shader_code_snapshot` | `shader_code_snapshot_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1300` |
| `shader_precompile` | `shader_precompile_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1301` |
| `pipeline_fast_first` | `pipeline_fast_first_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1304` |
| `pipeline_list` | `pipeline_list_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1306` |
| `pipeline_snapshot` | `pipeline_snapshot_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1308` |
| `pipeline_journal_clean` | `cmake` | `"-E" "rm" "-f" "pipeline-journal-test.pipelines" "pipeline-journal-test.shaders"` | Harness/setup excluded | `CMakeLists.txt:1313` |
| `pipeline_journal_record` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1317` |
| `pipeline_journal_replay` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1321` |
| `pipeline_fast_first_rendering` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1326` |
| `pipeline_fast_first_draw_run` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1330` |
| `pipeline_prefetch_rendering` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1334` |
| `pipeline_prefetch_programs` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1338` |
| `pipeline_prefetch_draw_run` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1342` |
| `vram_budget` | `vram_budget_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1346` |
| `device_compat` | `device_compat_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1347` |
| `shader_vertex_metadata` | `shader_vertex_metadata_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1348` |
| `resource_materialization` | `resource_materialization_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1349` |
| `resource_tracking` | `resource_tracking_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1350` |
| `event_queue_lifetime` | `event_queue_lifetime_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1351` |
| `event_queue_semantics` | `event_queue_semantics_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1352` |
| `thread_service` | `thread_service_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1353` |
| `kernel_file_system` | `kernel_file_system_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1354` |
| `sync_on_address` | `sync_on_address_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1355` |
| `audio_out2_port` | `audio_out2_port_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1356` |
| `audio_diag` | `audio_diag_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1357` |
| `audio_mix` | `audio_mix_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1358` |
| `audio_out_timing` | `audio_out_timing_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1359` |
| `ngs2_sampler` | `ngs2_sampler_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1360` |
| `audio_propagation` | `audio_propagation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1361` |
| `unresolved_import_log` | `unresolved_import_log_tests` | `(none)` | CPU PASS | `CMakeLists.txt:1362` |
| `pad_haptics` | `pad_haptics_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1363` |
| `controller_settings` | `controller_settings_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1364` |
| `host_input` | `host_input_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1365` |
| `save_data_memory` | `save_data_memory_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1366` |
| `png_enc` | `png_enc_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1367` |
| `ces` | `ces_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1368` |
| `save_data_dialog` | `save_data_dialog_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1369` |
| `http_uri_parse` | `http_uri_parse_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1370` |
| `shader_recompiler_compute` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:1371` |
| `shader_recompiler_tessellation` | `shader_recompiler_compute_tests` | `"--tessellation-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1372` |
| `shader_recompiler_alignbyte` | `shader_recompiler_compute_tests` | `"--alignbyte-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1374` |
| `graphics_pipeline_rasterization` | `shader_recompiler_compute_tests` | `"--polygon-mode-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1376` |
| `graphics_pipeline_rasterization_skip_inactive_ps` | `shader_recompiler_compute_tests` | `"--polygon-mode-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1379` |
| `graphics_pipeline_rasterization_blend_alpha_remap` | `shader_recompiler_compute_tests` | `"--polygon-mode-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1385` |
| `graphics_draw_offsets` | `shader_recompiler_compute_tests` | `"--draw-offset-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1389` |
| `guest_stack_exit` | `virtual_memory_allocation_tests` | `"--guest-stack-exit-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1391` |
| `virtual_memory_allocation` | `virtual_memory_allocation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1393` |
| `virtual_memory_allocation_lazy_backing` | `virtual_memory_allocation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1396` |
| `virtual_memory_allocation_lazy_zero` | `virtual_memory_allocation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1400` |
| `virtual_memory_allocation_clamp_memo_verify` | `virtual_memory_allocation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1406` |
| `virtual_memory_allocation_clamp_memo_off` | `virtual_memory_allocation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:1410` |
| `guest_red_zone_patcher` | `virtual_memory_allocation_tests` | `"--red-zone-patcher-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1414` |
| `command_scheduler_timeline` | `shader_recompiler_compute_tests` | `"--scheduler-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1416` |
| `command_scheduler_ready_ops_wait` | `shader_recompiler_compute_tests` | `"--scheduler-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1419` |
| `gpu_op_profiler_stamps` | `shader_recompiler_compute_tests` | `"--gpu-op-profile-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1424` |
| `command_scheduler_priority_spin` | `shader_recompiler_compute_tests` | `"--scheduler-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1426` |
| `stream_buffer_ring` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1430` |
| `occlusion_batch` | `shader_recompiler_compute_tests` | `"--occlusion-batch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1434` |
| `stream_buffer_ring_small_upload` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1436` |
| `stream_buffer_ring_small_upload_queued` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1440` |
| `gpu_command_lane` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1444` |
| `pm4_context_state` | `shader_recompiler_compute_tests` | `"--context-state-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1446` |
| `gpu_tiler` | `shader_recompiler_compute_tests` | `"--gpu-tiler-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1448` |
| `tiler_scratch_hold` | `shader_recompiler_compute_tests` | `"--tiler-scratch-hold-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1450` |
| `tile_layout` | `shader_recompiler_compute_tests` | `"--tile-layout-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1452` |
| `image_transfer_layout` | `shader_recompiler_compute_tests` | `"--image-layout-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1454` |
| `texture_cache_layered_image` | `shader_recompiler_compute_tests` | `"--layered-image-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1457` |
| `host_image_allocation` | `shader_recompiler_compute_tests` | `"--host-image-allocation-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1459` |
| `texture_cache_image_views` | `shader_recompiler_compute_tests` | `"--image-view-cache-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1461` |
| `texture_cache_storage_sampled` | `shader_recompiler_compute_tests` | `"--storage-sampled-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1463` |
| `texture_cache_depth_readback` | `shader_recompiler_compute_tests` | `"--depth-readback-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1465` |
| `compute_meta_clear_classification` | `shader_recompiler_compute_tests` | `"--compute-meta-clear-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1467` |
| `cpu_copy_page_skip_ranges` | `shader_recompiler_compute_tests` | `"--buffer-cache-range-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1469` |
| `cpu_copy_page_skip_alias_bytes` | `shader_recompiler_compute_tests` | `"--alias-bytes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1471` |
| `cpu_copy_page_skip_dirty_gc` | `shader_recompiler_compute_tests` | `"--buffer-cache-gc-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1473` |
| `buffer_cache_dirty_gc` | `shader_recompiler_compute_tests` | `"--buffer-cache-gc-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1477` |
| `draw_prep_code_cert` | `shader_recompiler_compute_tests` | `"--draw-prep-code-cert-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1479` |
| `draw_prep_code_cert_bytes` | `shader_recompiler_compute_tests` | `"--draw-prep-code-cert-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1481` |
| `buffer_range_memo` | `shader_recompiler_compute_tests` | `"--buffer-range-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1486` |
| `buffer_range_memo_verify` | `shader_recompiler_compute_tests` | `"--buffer-range-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1488` |
| `buffer_range_memo_off` | `shader_recompiler_compute_tests` | `"--buffer-range-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1492` |
| `buffer_range_memo_separate_queries` | `shader_recompiler_compute_tests` | `"--buffer-range-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1497` |
| `buffer_range_memo_relaxed_verify` | `shader_recompiler_compute_tests` | `"--buffer-range-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1503` |
| `buffer_range_memo_locked_queries` | `shader_recompiler_compute_tests` | `"--buffer-range-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1507` |
| `bda_sync_epoch` | `shader_recompiler_compute_tests` | `"--bda-sync-epoch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1513` |
| `bda_sync_epoch_incremental` | `shader_recompiler_compute_tests` | `"--bda-sync-epoch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1515` |
| `bda_sync_epoch_verify` | `shader_recompiler_compute_tests` | `"--bda-sync-epoch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1519` |
| `bda_sync_epoch_off` | `shader_recompiler_compute_tests` | `"--bda-sync-epoch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1523` |
| `bda_sync_per_submission` | `shader_recompiler_compute_tests` | `"--bda-sync-per-submission-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1528` |
| `bda_sync_per_submission_incremental` | `shader_recompiler_compute_tests` | `"--bda-sync-per-submission-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1530` |
| `bda_sync_dirty_log_verify` | `shader_recompiler_compute_tests` | `"--bda-sync-epoch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1535` |
| `bda_sync_dirty_log_off` | `shader_recompiler_compute_tests` | `"--bda-sync-epoch-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1539` |
| `binding_epoch_memo` | `shader_recompiler_compute_tests` | `"--binding-epoch-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1544` |
| `binding_epoch_memo_verify` | `shader_recompiler_compute_tests` | `"--binding-epoch-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1546` |
| `binding_epoch_memo_off` | `shader_recompiler_compute_tests` | `"--binding-epoch-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1550` |
| `binding_epoch_memo_no_sync_epoch` | `shader_recompiler_compute_tests` | `"--binding-epoch-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1554` |
| `binding_epoch_memo_cross_off` | `shader_recompiler_compute_tests` | `"--binding-epoch-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1558` |
| `binding_epoch_memo_touch_lean` | `shader_recompiler_compute_tests` | `"--binding-epoch-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1562` |
| `binding_memo_lookup` | `shader_recompiler_compute_tests` | `"--binding-memo-lookup-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1568` |
| `binding_memo_lookup_bindslots` | `shader_recompiler_compute_tests` | `"--binding-memo-lookup-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1570` |
| `binding_memo_lookup_verify` | `shader_recompiler_compute_tests` | `"--binding-memo-lookup-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1574` |
| `written_sync_skip` | `shader_recompiler_compute_tests` | `"--written-sync-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1579` |
| `written_sync_skip_verify` | `shader_recompiler_compute_tests` | `"--written-sync-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1581` |
| `written_sync_skip_off` | `shader_recompiler_compute_tests` | `"--written-sync-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1585` |
| `false_sharing_writes_off` | `shader_recompiler_compute_tests` | `"--false-sharing-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1590` |
| `false_sharing_writes` | `shader_recompiler_compute_tests` | `"--false-sharing-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1592` |
| `false_sharing_writes_verify` | `shader_recompiler_compute_tests` | `"--false-sharing-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1596` |
| `shader_upload_dedup` | `shader_recompiler_compute_tests` | `"--shader-upload-dedup-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1601` |
| `shader_upload_dedup_table` | `shader_recompiler_compute_tests` | `"--shader-upload-dedup-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1603` |
| `gpu_write_image_skip` | `shader_recompiler_compute_tests` | `"--gpu-write-image-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1608` |
| `gpu_write_image_skip_verify` | `shader_recompiler_compute_tests` | `"--gpu-write-image-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1610` |
| `gpu_write_image_skip_off` | `shader_recompiler_compute_tests` | `"--gpu-write-image-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1614` |
| `dcc_gpu_refresh` | `shader_recompiler_compute_tests` | `"--dcc-gpu-refresh-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1620` |
| `image_lru_skip` | `shader_recompiler_compute_tests` | `"--image-lru-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1624` |
| `image_lru_skip_verify` | `shader_recompiler_compute_tests` | `"--image-lru-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1626` |
| `image_lru_skip_off` | `shader_recompiler_compute_tests` | `"--image-lru-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1630` |
| `image_transit_skip` | `shader_recompiler_compute_tests` | `"--image-transit-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1636` |
| `image_transit_skip_verify` | `shader_recompiler_compute_tests` | `"--image-transit-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1638` |
| `image_transit_skip_off` | `shader_recompiler_compute_tests` | `"--image-transit-skip-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1642` |
| `tracker_gap_detectors` | `shader_recompiler_compute_tests` | `"--tracker-gap-detectors-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1647` |
| `tracker_gap_host_write_tracking_off` | `shader_recompiler_compute_tests` | `"--tracker-gap-detectors-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1649` |
| `mesh_indirect` | `shader_recompiler_compute_tests` | `"--mesh-indirect-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1655` |
| `mesh_indirect_verify` | `shader_recompiler_compute_tests` | `"--mesh-indirect-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1657` |
| `mesh_indirect_on` | `shader_recompiler_compute_tests` | `"--mesh-indirect-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1661` |
| `mesh_indirect_off` | `shader_recompiler_compute_tests` | `"--mesh-indirect-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1665` |
| `mesh_indirect_codegen` | `shader_cfg_tests` | `"--mesh-indirect-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1669` |
| `wave_row_reduction_codegen` | `shader_cfg_tests` | `"--wave-reduction-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1673` |
| `wave_reductions` | `shader_recompiler_compute_tests` | `"--wave-reductions-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1675` |
| `fold_lane_masks_codegen` | `shader_cfg_tests` | `"--fold-lane-masks-only"` | Deferred source/dispatch audit | `CMakeLists.txt:1679` |
| `wave_reductions_fold_lane_masks` | `shader_recompiler_compute_tests` | `"--wave-reductions-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1682` |
| `sampler_memo` | `shader_recompiler_compute_tests` | `"--sampler-memo-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1687` |
| `set_reuse_fresh` | `shader_recompiler_compute_tests` | `"--set-reuse-fresh-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1689` |
| `set_reuse_fresh_off` | `shader_recompiler_compute_tests` | `"--set-reuse-fresh-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1691` |
| `set_reuse_fresh_verify` | `shader_recompiler_compute_tests` | `"--set-reuse-fresh-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1694` |
| `texture_memo_revalidate` | `shader_recompiler_compute_tests` | `"--texture-memo-revalidate-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1698` |
| `texture_memo_revalidate_verify` | `shader_recompiler_compute_tests` | `"--texture-memo-revalidate-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1700` |
| `texture_memo_revalidate_off` | `shader_recompiler_compute_tests` | `"--texture-memo-revalidate-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1704` |
| `draw_sequence` | `shader_recompiler_compute_tests` | `"--draw-sequence-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1712` |
| `draw_sequence_off` | `shader_recompiler_compute_tests` | `"--draw-sequence-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1714` |
| `draw_sequence_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:1718` |
| `cp_commit` | `shader_recompiler_compute_tests` | `"--draw-sequence-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1724` |
| `cp_commit_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:1728` |
| `upload_dma` | `shader_recompiler_compute_tests` | `"--upload-dma-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1734` |
| `upload_dma_host_copy_off` | `shader_recompiler_compute_tests` | `"--upload-dma-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1736` |
| `render_state` | `shader_recompiler_compute_tests` | `"--render-state-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1741` |
| `render_state_off` | `shader_recompiler_compute_tests` | `"--render-state-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1743` |
| `draw_run_off` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1749` |
| `draw_run_inline` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1753` |
| `draw_run_inline_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1757` |
| `draw_run_parallel_bindings` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1761` |
| `draw_run_parallel_bindings_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1765` |
| `draw_prep_engine_parallel_run` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1769` |
| `draw_prep_engine_parallel_run_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1773` |
| `draw_run_inline_acquire` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1778` |
| `draw_run_inline_acquire_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1782` |
| `draw_run_parallel_bindings_acquire` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1786` |
| `draw_run_parallel_bindings_acquire_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1790` |
| `draw_prep_engine_parallel_run_acquire` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1794` |
| `draw_run_inline_targets` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1800` |
| `draw_run_inline_targets_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1804` |
| `draw_run_parallel_bindings_targets` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1808` |
| `draw_run_inline_pipeswap` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1813` |
| `draw_run_inline_pipeswap_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1817` |
| `draw_run_writes_off` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1823` |
| `draw_run_writes_inline` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1827` |
| `draw_run_writes_inline_verify` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1831` |
| `draw_run_writes_parallel_all` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1835` |
| `draw_run_writes_parallel_verify` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1839` |
| `draw_run_recall_off` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1844` |
| `draw_run_recall_inline` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1848` |
| `draw_run_recall_inline_verify` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1852` |
| `draw_run_recall_parallel_all` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1856` |
| `draw_run_recall_parallel_verify` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1860` |
| `draw_run_image_epoch_run_on` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1866` |
| `draw_run_image_epoch_run_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1870` |
| `draw_run_image_epoch_writes_on` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1874` |
| `draw_run_image_epoch_writes_verify` | `shader_recompiler_compute_tests` | `"--draw-run-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1878` |
| `draw_run_image_epoch_recall_on` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1882` |
| `draw_run_image_epoch_recall_verify` | `shader_recompiler_compute_tests` | `"--draw-run-recall-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1886` |
| `draw_run_live` | `shader_recompiler_compute_tests` | `"--draw-run-live"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1891` |
| `draw_run_inline_push` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1896` |
| `draw_run_parallel_bindings_push_verify` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1900` |
| `draw_run_parallel_bindings_all` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1904` |
| `draw_prep_engine_parallel_run_all` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1908` |
| `draw_prep_engine` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1913` |
| `draw_prep_engine_inline` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1915` |
| `draw_prep_engine_parallel` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1919` |
| `draw_prep_engine_parallel_fence` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1923` |
| `draw_prep_engine_parallel_value` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1928` |
| `draw_prep_engine_parallel_value_audit` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1932` |
| `draw_prep_engine_parallel_inplace_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1937` |
| `draw_prep_engine_render_state_off` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1941` |
| `draw_prep_engine_parallel_copied` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1945` |
| `draw_prep_engine_inline_bindings_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1952` |
| `draw_prep_engine_inline_bindings` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1956` |
| `draw_prep_engine_parallel_bindings_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1960` |
| `draw_prep_engine_parallel_bindings` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1964` |
| `draw_prep_engine_parallel_steal_bindings_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1968` |
| `draw_prep_engine_inline_bindings_parts` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1972` |
| `draw_prep_engine_inline_bindings_checks` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1977` |
| `draw_prep_engine_inline_bindings_checks_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1981` |
| `draw_prep_engine_inline_bindings_texturememo` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1987` |
| `draw_prep_engine_inline_bindings_texturememo_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1991` |
| `draw_prep_engine_inline_bindings_texturememo_nosequence` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1995` |
| `draw_prep_engine_parallel_bindings_texturememo_nosequence_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:1999` |
| `shader_recompiler_compute_bindings_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2003` |
| `shader_recompiler_compute_offset_audit` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2008` |
| `draw_prep_engine_parallel_steal` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2015` |
| `draw_prep_engine_parallel_steal_one_worker` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2019` |
| `draw_prep_engine_parallel_cert_ranges_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2026` |
| `draw_prep_engine_inline_cert_ranges_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2030` |
| `draw_prep_engine_parallel_value_audit_cert_ranges_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2034` |
| `draw_prep_engine_parallel_cert_ranges_commit` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2038` |
| `shader_recompiler_compute_cert_ranges_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2042` |
| `cp_seq_inline` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2051` |
| `cp_seq_inline_verify` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2054` |
| `cp_seq_inline_verify_count` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2058` |
| `cp_seq_direct` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2062` |
| `shader_recompiler_compute_cp_seq_inline` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2064` |
| `shader_recompiler_compute_cp_seq_inline_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2068` |
| `shader_recompiler_compute_cp_seq_inline_recorder` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2072` |
| `draw_prep_engine_parallel_cp_seq_inline_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2076` |
| `draw_prep_engine_inline_cp_seq_inline_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2080` |
| `eop_timestamps_gpu_cp_seq_inline_verify` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2084` |
| `eop_timestamps_gpu_deferred_cp_seq_inline_verify` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2088` |
| `cp_seq_thread` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2097` |
| `cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2100` |
| `cp_seq_thread_park_verify` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2106` |
| `draw_prep_engine_parallel_cp_seq_thread_park` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2110` |
| `gpu_command_lane_cp_seq_thread_park` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2114` |
| `gpu_command_lane_cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2118` |
| `draw_prep_engine_parallel_cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2122` |
| `draw_prep_engine_parallel_steal_cp_seq_thread` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2126` |
| `draw_prep_engine_inline_cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2130` |
| `draw_prep_engine_off_cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2134` |
| `draw_prep_engine_parallel_cp_seq_thread_prefetch_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2141` |
| `draw_prep_engine_parallel_cp_seq_thread_prefetch_mismatch` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2145` |
| `draw_prep_engine_parallel_cp_seq_thread_park_prefetch` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2149` |
| `cp_seq_thread_prefetch_verify` | `shader_recompiler_compute_tests` | `"--cp-seq-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2153` |
| `gpu_command_lane_cp_seq_thread_prefetch_verify` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2157` |
| `shader_recompiler_compute_cp_seq_thread_prefetch_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2161` |
| `draw_prep_engine_parallel_cp_seq_thread_submit_interval` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2168` |
| `shader_recompiler_compute_cp_seq_thread_submit_interval` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2172` |
| `shader_recompiler_compute_submit_interval` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2176` |
| `shader_recompiler_compute_cp_seq_thread` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2180` |
| `shader_recompiler_compute_cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2184` |
| `draw_prep_engine_parallel_clean_since_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2190` |
| `draw_run_parallel_clean_since` | `shader_recompiler_compute_tests` | `"--draw-run-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2194` |
| `draw_prep_engine_parallel_cp_seq_thread_dispatch_verify` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2200` |
| `draw_prep_engine_parallel_cp_seq_thread_dispatch_park` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2204` |
| `shader_recompiler_compute_cp_seq_thread_recorder` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2208` |
| `eop_timestamps_gpu_cp_seq_thread_verify` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2212` |
| `eop_timestamps` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2218` |
| `eop_timestamps_gpu` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2220` |
| `eop_timestamps_gpu_verify` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2223` |
| `eop_timestamps_gpu_deferred` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2227` |
| `draw_prep_code_cert_copied` | `shader_recompiler_compute_tests` | `"--draw-prep-code-cert-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2231` |
| `buffer_cache_eager_readback` | `shader_recompiler_compute_tests` | `"--readback-eager-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2235` |
| `buffer_cache_readback_flush_side` | `shader_recompiler_compute_tests` | `"--readback-flush-side-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2238` |
| `buffer_cache_late_storage_fixed` | `shader_recompiler_compute_tests` | `"--late-storage-write-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2260` |
| `buffer_cache_late_storage_control` | `shader_recompiler_compute_tests` | `"--late-storage-write-control"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2260` |
| `buffer_cache_late_storage_queued_fixed` | `shader_recompiler_compute_tests` | `"--late-storage-write-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2260` |
| `buffer_cache_late_storage_queued_control` | `shader_recompiler_compute_tests` | `"--late-storage-write-control"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2260` |
| `buffer_cache_late_storage_no_side` | `shader_recompiler_compute_tests` | `"--late-storage-write-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2260` |
| `buffer_cache_late_storage_no_gpu_side` | `shader_recompiler_compute_tests` | `"--late-storage-write-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2260` |
| `texture_cache_alias_bytes` | `shader_recompiler_compute_tests` | `"--alias-bytes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2267` |
| `texture_cache_alias_bytes_writeback` | `shader_recompiler_compute_tests` | `"--alias-bytes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2269` |
| `texture_cache_alias_bytes_off` | `shader_recompiler_compute_tests` | `"--alias-bytes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2273` |
| `texture_cache_texel_sync_gpu_dirty` | `shader_recompiler_compute_tests` | `"--texel-sync-gpu-dirty-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2277` |
| `texture_cache_texel_sync_gpu_dirty_whole_images` | `shader_recompiler_compute_tests` | `"--texel-sync-gpu-dirty-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2279` |
| `srt_variant_reads` | `shader_recompiler_compute_tests` | `"--srt-variant-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2284` |
| `program_cache` | `shader_recompiler_compute_tests` | `"--program-cache-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2291` |
| `shader_parallel_compile` | `shader_recompiler_compute_tests` | `"--parallel-compile-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2296` |
| `program_cache_gpu` | `shader_recompiler_compute_tests` | `"--program-cache-gpu-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2298` |
| `program_cache_gpu_verify` | `shader_recompiler_compute_tests` | `"--program-cache-gpu-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2300` |
| `program_cache_gpu_background` | `shader_recompiler_compute_tests` | `"--program-cache-gpu-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2304` |
| `program_cache_verify_tamper` | `shader_recompiler_compute_tests` | `"--program-cache-tamper-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2309` |
| `program_cache_verify_tamper_background` | `shader_recompiler_compute_tests` | `"--program-cache-tamper-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2313` |
| `shader_recompiler_compute_program_cache` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2317` |
| `draw_prep_engine_program_cache` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2321` |
| `cp_recorder_gpu` | `shader_recompiler_compute_tests` | `"--cp-recorder-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2333` |
| `cp_recorder_gpu_queued` | `shader_recompiler_compute_tests` | `"--cp-recorder-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2335` |
| `cp_recorder_gpu_inline` | `shader_recompiler_compute_tests` | `"--cp-recorder-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2339` |
| `shader_recompiler_compute_cp_recorder` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2343` |
| `shader_recompiler_compute_cp_recorder_queued` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2347` |
| `shader_recompiler_compute_cp_recorder_inline` | `shader_recompiler_compute_tests` | `(none)` | Vulkan/mixed executable excluded | `CMakeLists.txt:2351` |
| `draw_prep_engine_cp_recorder` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `draw_prep_engine_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `draw_prep_engine_bindings_cp_recorder` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `draw_prep_engine_bindings_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `graphics_pipeline_rasterization_cp_recorder` | `shader_recompiler_compute_tests` | `"--polygon-mode-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `graphics_pipeline_rasterization_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--polygon-mode-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `command_scheduler_timeline_cp_recorder` | `shader_recompiler_compute_tests` | `"--scheduler-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `command_scheduler_timeline_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--scheduler-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `gpu_command_lane_cp_recorder` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `gpu_command_lane_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `pm4_context_state_cp_recorder` | `shader_recompiler_compute_tests` | `"--context-state-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `pm4_context_state_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--context-state-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `stream_buffer_ring_cp_recorder` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `stream_buffer_ring_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `texture_cache_layered_image_cp_recorder` | `shader_recompiler_compute_tests` | `"--layered-image-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `texture_cache_layered_image_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--layered-image-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `compute_meta_clear_classification_cp_recorder` | `shader_recompiler_compute_tests` | `"--compute-meta-clear-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `compute_meta_clear_classification_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--compute-meta-clear-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `buffer_cache_dirty_gc_cp_recorder` | `shader_recompiler_compute_tests` | `"--buffer-cache-gc-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `buffer_cache_dirty_gc_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--buffer-cache-gc-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `buffer_cache_eager_readback_cp_recorder` | `shader_recompiler_compute_tests` | `"--readback-eager-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `buffer_cache_eager_readback_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--readback-eager-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `eop_timestamps_gpu_cp_recorder` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `eop_timestamps_gpu_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `eop_timestamps_gpu_verify_cp_recorder` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `eop_timestamps_gpu_verify_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--eop-timestamps-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `cp_recorder_gpu_cp_recorder_validation` | `shader_recompiler_compute_tests` | `"--cp-recorder-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2406` |
| `cp_recorder_gpu_cp_recorder_inline_validation` | `shader_recompiler_compute_tests` | `"--cp-recorder-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2410` |
| `draw_prep_engine_validation` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2401` |
| `draw_prep_engine_cp_recorder_validation` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2406` |
| `draw_prep_engine_cp_recorder_inline_validation` | `shader_recompiler_compute_tests` | `"--draw-prep-engine-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2410` |
| `gpu_command_lane_validation` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2401` |
| `gpu_command_lane_cp_recorder_validation` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2406` |
| `gpu_command_lane_cp_recorder_inline_validation` | `shader_recompiler_compute_tests` | `"--gpu-command-lane-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2410` |
| `stream_buffer_ring_validation` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2401` |
| `stream_buffer_ring_cp_recorder_validation` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2406` |
| `stream_buffer_ring_cp_recorder_inline_validation` | `shader_recompiler_compute_tests` | `"--stream-buffer-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2410` |
| `texture_cache_layered_image_validation` | `shader_recompiler_compute_tests` | `"--layered-image-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2401` |
| `texture_cache_layered_image_cp_recorder_validation` | `shader_recompiler_compute_tests` | `"--layered-image-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2406` |
| `texture_cache_layered_image_cp_recorder_inline_validation` | `shader_recompiler_compute_tests` | `"--layered-image-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2410` |
| `buffer_cache_eager_readback_validation` | `shader_recompiler_compute_tests` | `"--readback-eager-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2401` |
| `buffer_cache_eager_readback_cp_recorder_validation` | `shader_recompiler_compute_tests` | `"--readback-eager-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2406` |
| `buffer_cache_eager_readback_cp_recorder_inline_validation` | `shader_recompiler_compute_tests` | `"--readback-eager-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2410` |
| `bda_writes` | `shader_recompiler_compute_tests` | `"--bda-writes-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2423` |
| `texture_cache_image_overlap` | `shader_recompiler_compute_tests` | `"--image-overlap-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2428` |
| `texture_cache_htile_clear` | `shader_recompiler_compute_tests` | `"--htile-clear-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2430` |
| `buffer_cache_ranges` | `shader_recompiler_compute_tests` | `"--buffer-cache-range-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2432` |
| `texture_cache_image_overlap_cp_recorder` | `shader_recompiler_compute_tests` | `"--image-overlap-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `texture_cache_image_overlap_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--image-overlap-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `texture_cache_htile_clear_cp_recorder` | `shader_recompiler_compute_tests` | `"--htile-clear-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `texture_cache_htile_clear_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--htile-clear-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `buffer_cache_ranges_cp_recorder` | `shader_recompiler_compute_tests` | `"--buffer-cache-range-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2361` |
| `buffer_cache_ranges_cp_recorder_inline` | `shader_recompiler_compute_tests` | `"--buffer-cache-range-only"` | Vulkan/mixed executable excluded | `CMakeLists.txt:2364` |
| `failure_message_fails_test` | `cmake` | `"-E" "echo" "SelfTests: failed: a check failed, the process exited 0"` | Harness/setup excluded | `CMakeLists.txt:2444` |
| `amd_cpu_emulation` | `amd_cpu_emulation_tests` | `(none)` | Deferred source/dispatch audit | `CMakeLists.txt:2538` |
