# P0.4 source/dispatch audit — S-004

Audit source: `226ea5adb896fcd610b82d8e34b1657e0392b16c`, branch `feature/rdna1-mesh-fallback`, clean before documentation edits. Renderer/test/CMake source still matches pinned `744705ee1df4b0acb1af382ff5c32ca4a20aa5f7`. This resolves the 82 S-003 deferred registrations by source inspection only. **No build, CTest execution, shader validator execution, Vulkan or game run occurred.** Existing 22/22 passes remain S-003 evidence.

| Resolved requirements | Cases | New execution |
| --- | ---: | --- |
| CPU/offline logic, generated fixtures or fake backend | 37 | NOT RUN |
| CPU with host VM, processes, scheduling, timing or dummy SDL dependencies | 31 | NOT RUN |
| Offline shader/IR/codegen modes | 13 | NOT RUN |
| Vulkan/window required | 1 | NOT RUN |

All 82 dispositions are resolved; all 82 executions remain pending. The separate 318 S-003 conservative Vulkan/mixed exclusions (317 compute-harness registrations plus sparse residency), two harness cases and confirmed CPU `null_texture_desc` retain their previous status. Thus 104 registrations are now known CPU/offline candidates including the 22 previously run, while 319 are Vulkan-required or conservatively mixed and two are harness/setup. This is not a complete per-branch audit of the 317-case mixed executable.

## Findings and run requirements

- `kernel_file_system` has no arguments in CTest, so it passes the `--mutex-only` early return and creates a hidden Vulkan window/device. Do not include it in a CPU-only regex merely because of its name.
- The four emulator CLI cases return before `Run`, but Windows first calls optional `InitCudaEarly`. For CPU-only execution require `KYTY_CUDA_EARLY_INIT` unset or `0`; record host address-space initialization. Preserve CTest WILL_FAIL for invalid options.
- `shader_cfg_tests` initializes Config/Log and a CPU shader map. Its registered modes operate offline; shared validation uses `SPV_ENV_VULKAN_1_2` (not S-003 fixtures’ Vulkan 1.3). A mode may only test IR/registration logic; do not report validator coverage without its actual checks/results. The full target links all emulator sources.
- `cpu_placement`/recorder can skip live scheduling when no core is reserved; V-cache sets/affinity require applicable dual-CCD X3D topology for that section. The pending scenario checks delayed activation. Inspect output for internal skips; exit 0 alone does not prove all scheduling assertions ran. These are CPU topology requirements, not RDNA1 requirements.
- Guest probes and AMD CPU emulation use executable memory and native exception handling. Page/memory/virtual-memory tests use real mappings/protection, child processes and/or large address reservations. Run serially in a separate host-resource batch; any sandbox/OS or memory failure must be retained rather than relabeled as a GPU failure.
- AudioOut2 supplies fake AudioInternal/time methods; audio timing/controller/input tests include production implementations behind fake host callbacks. Pad haptics keeps real in-memory SDL streams and Opus but uses fake HID/gamepad devices and asks SDL for the dummy audio backend. It requires that backend, not a physical pad/speaker.
- Archive/media/trophy/save/pipeline-journal fixtures are generated locally or embedded synthetic test data. Preserve temporary cleanup and serial isolation; no local game/archive/save is an input. `save_data_memory` launches child write/read processes. URI tests resolve registration functions and test strings; no network request is executed.
- Some CPU-only cases (`shader_cfg`, `repeat_trace`, URI parsing, virtual memory, `null_texture_desc`) use `add_kyty_full_emulator_test`. Their source/build cost does not turn their execution into GPU integration. Keep expensive target builds in later batches.

## Audited target paths and behavior

Source references are one-based at the audit SHA. Main/dispatch plus initialization/fake-backend evidence is linked below; per-case CMake registration and exact arguments remain in [the 425-case inventory](RDNA1_BASELINE_TEST_INVENTORY.md).

| Target | Cases | Source evidence | Execution requirements |
| --- | ---: | --- | --- |
| `occlusion_reset_tests` | 1 | [`tests/OcclusionResetTests.cpp:14`](../tests/OcclusionResetTests.cpp#L14) | Synthetic reset windows and staged live switches; no query pool or device. |
| `kyty_emulator` | 4 | [`src/main.cpp:751`](../src/main.cpp#L751); [`src/main.cpp:698`](../src/main.cpp#L698); [`src/main.cpp:320`](../src/main.cpp#L320) | Main initializes virtual memory/threads, parses all arguments, then --help returns before Run. Keep KYTY_CUDA_EARLY_INIT unset/0: its optional pre-parser path loads nvcuda.dll and calls cuInit. Invalid option cases use WILL_FAIL. |
| `ime_dialog_tests` | 1 | [`tests/ImeDialogTests.cpp:311`](../tests/ImeDialogTests.cpp#L311) | In-memory IME text/filter/status lifecycle; linked imeCommon/imeDialog use containers and locks, no window/backend. |
| `shader_cfg_tests` | 13 | [`tests/shaderCfgTests.cpp:15855`](../tests/shaderCfgTests.cpp#L15855); [`tests/shaderCfgTests.cpp:1124`](../tests/shaderCfgTests.cpp#L1124); [`tests/shaderCfgTests.cpp:262`](../tests/shaderCfgTests.cpp#L262); [`src/graphics/shader/shader.cpp:104`](../src/graphics/shader/shader.cpp#L104); [`tests/shaderCfgTests.cpp:15879`](../tests/shaderCfgTests.cpp#L15879); [`tests/shaderCfgTests.cpp:15905`](../tests/shaderCfgTests.cpp#L15905); [`tests/shaderCfgTests.cpp:15893`](../tests/shaderCfgTests.cpp#L15893); [`tests/shaderCfgTests.cpp:15859`](../tests/shaderCfgTests.cpp#L15859); [`tests/shaderCfgTests.cpp:15873`](../tests/shaderCfgTests.cpp#L15873); [`tests/shaderCfgTests.cpp:15889`](../tests/shaderCfgTests.cpp#L15889); [`tests/shaderCfgTests.cpp:15898`](../tests/shaderCfgTests.cpp#L15898); [`tests/shaderCfgTests.cpp:15915`](../tests/shaderCfgTests.cpp#L15915) | EnsureConfigInitialized sets Config/Log and ShaderInit (CPU shader map). All registered branches operate on synthetic shader/IR/SPIR-V data, no Vulkan device. Shared validation helper uses SPV_ENV_VULKAN_1_2; not every branch calls it. Full-emulator target makes compilation expensive. |
| `scalar_provenance_tests` | 1 | [`tests/ScalarProvenanceTests.cpp:837`](../tests/ScalarProvenanceTests.cpp#L837) | Constructed IR/SRT values and optimizer passes; synthetic memory readers, no renderer initialization. |
| `lane_local_tests` | 1 | [`tests/LaneLocalTests.cpp:119`](../tests/LaneLocalTests.cpp#L119) | No arguments selects built-in instruction arrays and CFG/lane analysis. External binary-file mode is not registered. |
| `image_page_table_tests` | 1 | [`tests/ImagePageTableTests.cpp:173`](../tests/ImagePageTableTests.cpp#L173) | Synthetic address ownership/index data, no mapped image or device. |
| `texel_image_lookup_tests` | 1 | [`tests/TexelImageLookupTests.cpp:118`](../tests/TexelImageLookupTests.cpp#L118) | Synthetic CPU page table and candidate lookups. |
| `texture_set_cache_tests` | 1 | [`tests/TextureSetCacheTests.cpp:190`](../tests/TextureSetCacheTests.cpp#L190) | Synthetic cache bindings, generation/eviction bookkeeping, no descriptor allocation. |
| `clear_register_decode_tests` | 1 | [`tests/ClearRegisterDecodeTests.cpp:85`](../tests/ClearRegisterDecodeTests.cpp#L85) | Packed register words decoded to vk::ClearColorValue values; no Vulkan calls. |
| `draw_prep_tests` | 1 | [`tests/DrawPrepTests.cpp:1975`](../tests/DrawPrepTests.cpp#L1975) | CPU dirty/read-set certificates and concurrent worker gates; no-argument registration avoids optional measurement branch. High iteration counts require a realistic timeout. |
| `eop_timestamp_tests` | 1 | [`tests/EopTimestampTests.cpp:274`](../tests/EopTimestampTests.cpp#L274) | Synthetic clock conversion and CPU query-ring concurrency; no GPU timestamp query. |
| `repeat_trace_tests` | 1 | [`tests/RepeatTraceTests.cpp:201`](../tests/RepeatTraceTests.cpp#L201) | Synthetic register snapshots and prepared-resource hashes, no device/guest allocation; full-emulator-linked target. |
| `binding_path_tests` | 1 | [`tests/BindingPathTests.cpp:247`](../tests/BindingPathTests.cpp#L247) | Layout-signature hashing and fake descriptor handles; dispatcher storage exists but no Vulkan call. |
| `cp_recorder_tests` | 1 | [`tests/CpRecorderTests.cpp:1239`](../tests/CpRecorderTests.cpp#L1239) | Encoder/ring/replay against fake command sink, worker threads and corruption checks; no Vulkan device; optional --bench not registered. |
| `cp_sequencer_tests` | 1 | [`tests/CpSequencerTests.cpp:339`](../tests/CpSequencerTests.cpp#L339) | CPU op/snapshot rings and touched-page accounting with threads and fake command sink. |
| `memory_tracker_tests` | 1 | [`tests/MemoryTrackerTests.cpp:2933`](../tests/MemoryTrackerTests.cpp#L2933) | Host fixed mappings, page protection, fault handler stress and child death cases; GPU ownership is CPU bookkeeping, no device. Requires host VM/exception/process support. |
| `page_manager_tests` | 1 | [`tests/PageManagerTests.cpp:1088`](../tests/PageManagerTests.cpp#L1088) | Host mapped pages/protection, concurrent watchers/deferred release and child death cases; no device. Requires fixed-address VM and child-process support. |
| `archive_file_tests` | 1 | [`tests/ArchiveFileTests.cpp:259`](../tests/ArchiveFileTests.cpp#L259) | Creates synthetic payload/ZArchive in a temporary directory; Unicode file I/O, no game archive required. |
| `trophy_system_tests` | 1 | [`tests/TrophySystemTests.cpp:349`](../tests/TrophySystemTests.cpp#L349) | Generated temporary trophy/event/progress fixtures and localization/persistence; no game content. |
| `avplayer_file_tests` | 1 | [`tests/AvPlayerFileTests.cpp:168`](../tests/AvPlayerFileTests.cpp#L168) | Generated native/archive payload and callback AVIO, test pthread/file mapping stubs, read-ahead threads. No playback/rendering. |
| `vdecsw_tests` | 1 | [`tests/VdecswTests.cpp:381`](../tests/VdecswTests.cpp#L381); [`tests/VdecswTests.cpp:28`](../tests/VdecswTests.cpp#L28); [`src/libs/videoDec2Decoder.cpp:95`](../src/libs/videoDec2Decoder.cpp#L95) | Embedded generated 16x16 AVC fixtures decoded by software FFmpeg decoder, guarded CPU output buffers; no hardware decoder/device. |
| `hang_watchdog_tests` | 3 | [`tests/HangWatchdogTests.cpp:207`](../tests/HangWatchdogTests.cpp#L207) | Default tests include Windows fault/context probes; --fire uses timed worker/report generation; --fatal-shutdown records a mock terminal error. Temporary reports/native thread inspection; no GPU. CMake gives fire/fatal cases 15 s. |
| `profiler_counter_tests` | 1 | [`tests/ProfilerCounterTests.cpp:340`](../tests/ProfilerCounterTests.cpp#L340) | Thread-local counters, timed CPU benchmarks and instrumentation (default runs benchmarks); no device. |
| `cpu_placement_tests` | 5 | [`tests/CpuPlacementTests.cpp:1191`](../tests/CpuPlacementTests.cpp#L1191); [`tests/CpuPlacementTests.cpp:697`](../tests/CpuPlacementTests.cpp#L697); [`tests/CpuPlacementTests.cpp:484`](../tests/CpuPlacementTests.cpp#L484) | Synthetic layouts plus real Windows affinity/CPU-set scheduling. Default/cp+recorder may skip live section when no core reserved; sets/affinity V-cache branches skip on non-dual-CCD-X3D; pending checks delayed activation. Capture internal skips even on exit 0. |
| `live_switch_tests` | 1 | [`tests/LiveSwitchTests.cpp:339`](../tests/LiveSwitchTests.cpp#L339) | Staged text switches, CPU spin and file-watching thread fixture; no renderer/device. |
| `guest_probe_tests` | 3 | [`tests/GuestProbeTests.cpp:128`](../tests/GuestProbeTests.cpp#L128) | Windows executable memory patch/probe retirement/shared/equality tests with threads. Non-Windows main prints no-op; Windows host only for meaningful execution. |
| `u59_preset_tests` | 1 | [`tests/U59PresetTests.cpp:237`](../tests/U59PresetTests.cpp#L237) | Parser, generated temporary load fixture and shipped tools/u59-preset.json (CMake compile definition); no launcher/device. |
| `shader_precompile_tests` | 1 | [`tests/ShaderPrecompileTests.cpp:513`](../tests/ShaderPrecompileTests.cpp#L513) | Synthetic shader journal file I/O, background writer and replay callbacks; no driver compiler. |
| `pipeline_list_tests` | 1 | [`tests/PipelineListTests.cpp:507`](../tests/PipelineListTests.cpp#L507) | Fake shader words/handles, pipeline snapshot/list serialization/merge/resolve and temporary journal; no real shaders/game files/device. |
| `pipeline_snapshot_tests` | 1 | [`tests/PipelineSnapshotTests.cpp:147`](../tests/PipelineSnapshotTests.cpp#L147) | Fake Vulkan handles/create-info values, capture/serialize/deserialize/refusal checks; no pipeline creation. |
| `shader_vertex_metadata_tests` | 1 | [`tests/ShaderVertexMetadataTests.cpp:111`](../tests/ShaderVertexMetadataTests.cpp#L111) | Synthetic vertex code/metadata and host-memory helper; no driver/device. |
| `resource_materialization_tests` | 1 | [`tests/ResourceMaterializationTests.cpp:771`](../tests/ResourceMaterializationTests.cpp#L771) | Constructed IR plans and synthetic SRT memory readers, concurrent evaluation and live-switch modes; no GPU. |
| `resource_tracking_tests` | 1 | [`tests/ResourceTrackingTests.cpp:3343`](../tests/ResourceTrackingTests.cpp#L3343) | Synthetic IR/resource descriptors/SRT walks and binding/write analysis; no GPU dispatch. |
| `event_queue_lifetime_tests` | 1 | [`tests/EventQueueLifetimeTests.cpp:554`](../tests/EventQueueLifetimeTests.cpp#L554) | Host threads/timers and callback lifetime/race checks; no graphics device. |
| `event_queue_semantics_tests` | 1 | [`tests/EventQueueSemanticsTests.cpp:387`](../tests/EventQueueSemanticsTests.cpp#L387) | Host event/timer queues and synthetic graphics/video counters; no renderer. Registered no-argument mode avoids optional hitch benchmark. |
| `thread_service_tests` | 1 | [`tests/ThreadServiceTests.cpp:288`](../tests/ThreadServiceTests.cpp#L288) | Host service priorities, scheduler yield, condition waits and pending signals; Windows priority inspection, no device. |
| `kernel_file_system_tests` | 1 | [`tests/KernelFileSystemTests.cpp:2331`](../tests/KernelFileSystemTests.cpp#L2331); [`tests/KernelFileSystemTests.cpp:2330`](../tests/KernelFileSystemTests.cpp#L2330); [`tests/KernelFileSystemTests.cpp:2347`](../tests/KernelFileSystemTests.cpp#L2347); [`tests/KernelFileSystemTests.cpp:2354`](../tests/KernelFileSystemTests.cpp#L2354) | Registered no-argument main proceeds past optional --mutex-only branch: SDL video initialization, hidden SDL_WINDOW_VULKAN window and CreateVulkan. Requires Vulkan/display driver; later local sockets/file/VM tests do not make it CPU-only. |
| `sync_on_address_tests` | 1 | [`tests/SyncOnAddressTests.cpp:290`](../tests/SyncOnAddressTests.cpp#L290) | Host wait/wake/timeout/race primitives on CPU addresses; no device. |
| `audio_out2_port_tests` | 1 | [`tests/AudioOut2PortTests.cpp:791`](../tests/AudioOut2PortTests.cpp#L791); [`tests/AudioOut2PortTests.cpp:652`](../tests/AudioOut2PortTests.cpp#L652) | AudioInternal and kernel-time implementations are test stubs; fake device handles/captured PCM and threads, no audio output device. |
| `audio_out_timing_tests` | 1 | [`tests/AudioOutTimingTests.cpp:611`](../tests/AudioOutTimingTests.cpp#L611); [`tests/AudioOutTimingTests.cpp:59`](../tests/AudioOutTimingTests.cpp#L59); [`tests/AudioOutTimingTests.cpp:128`](../tests/AudioOutTimingTests.cpp#L128) | Production audio.cpp included behind Fake SDL init/open/queue functions and deterministic clock/thread replacements; CPU vectors, no audio device. |
| `ngs2_sampler_tests` | 1 | [`tests/Ngs2SamplerTests.cpp:1696`](../tests/Ngs2SamplerTests.cpp#L1696) | Production ngs2.cpp with generated PCM/ATRAC9 fixtures and software decoder/reference comparisons; no audio output or GPU. |
| `audio_propagation_tests` | 1 | [`tests/AudioPropagationTests.cpp:142`](../tests/AudioPropagationTests.cpp#L142) | CPU handle state and direct/silent sound buffers; no output device. |
| `pad_haptics_tests` | 1 | [`tests/PadHapticsTests.cpp:1313`](../tests/PadHapticsTests.cpp#L1313); [`tests/PadHapticsTests.cpp:66`](../tests/PadHapticsTests.cpp#L66); [`tests/PadHapticsTests.cpp:262`](../tests/PadHapticsTests.cpp#L262); [`tests/PadHapticsTests.cpp:1314`](../tests/PadHapticsTests.cpp#L1314) | Production modules with fake gamepad/HID endpoints and real in-memory SDL streams/Opus. Main requests dummy audio driver; needs SDL dummy backend, no physical controller/audio/GPU. |
| `controller_settings_tests` | 1 | [`tests/ControllerSettingsTests.cpp:624`](../tests/ControllerSettingsTests.cpp#L624); [`tests/ControllerSettingsTests.cpp:71`](../tests/ControllerSettingsTests.cpp#L71) | Production controller.cpp wrapped in Fake SDL time/gamepad/rumble/effect functions; Config lifecycle, no physical controller. |
| `host_input_tests` | 1 | [`tests/HostInputTests.cpp:193`](../tests/HostInputTests.cpp#L193); [`tests/HostInputTests.cpp:44`](../tests/HostInputTests.cpp#L44) | Production host input with Fake gamepad/axis/cursor/mouse/time callbacks and test Controller functions; Config lifecycle, no physical input/window. |
| `save_data_memory_tests` | 1 | [`tests/SaveDataMemoryTests.cpp:649`](../tests/SaveDataMemoryTests.cpp#L649) | Synthetic save data under temporary CWD; launches write/read child processes, Config lifecycle, persistence and memory tests; no game save/device required. |
| `png_enc_tests` | 1 | [`tests/PngEncTests.cpp:241`](../tests/PngEncTests.cpp#L241) | Production PNG encoder plus memory-only stb PNG decode of synthetic pixels; no image files or device. |
| `ces_tests` | 1 | [`tests/CesTests.cpp:118`](../tests/CesTests.cpp#L118) | Production character conversion with symbol/timer stubs; string buffers, no network/device. |
| `save_data_dialog_tests` | 1 | [`tests/SaveDataDialogTests.cpp:128`](../tests/SaveDataDialogTests.cpp#L128) | Save dialog state-machine/status polling with fake timer; dialog.cpp performs no GUI initialization. |
| `http_uri_parse_tests` | 1 | [`tests/HttpUriParseTests.cpp:167`](../tests/HttpUriParseTests.cpp#L167); [`src/libs/libNet.cpp:706`](../src/libs/libNet.cpp#L706) | InitNet_1_Http only registers symbols; main resolves URI parse/build functions, string-only cases; no HTTP request or socket. |
| `virtual_memory_allocation_tests` | 7 | [`tests/VirtualMemoryAllocationTests.cpp:5121`](../tests/VirtualMemoryAllocationTests.cpp#L5121); [`tests/VirtualMemoryAllocationTests.cpp:158`](../tests/VirtualMemoryAllocationTests.cpp#L158); [`tests/VirtualMemoryAllocationTests.cpp:5171`](../tests/VirtualMemoryAllocationTests.cpp#L5171); [`CMakeLists.txt:1393`](../CMakeLists.txt#L1393) | InitSubsystems initializes host virtual memory/threads, Config/Log, generated param JSON and kernel Memory lifecycle only. Registered full runs/guest-stack/red-zone branches use real host VM, JIT/fibers, exception and memory operations, no Vulkan initialization. Large virtual reservations/physical commitments require separate memory-conscious batch. |
| `amd_cpu_emulation_tests` | 1 | [`tests/AmdCpuEmulationTests.cpp:845`](../tests/AmdCpuEmulationTests.cpp#L845) | Host exception handler and executable buffers, CPUID-aware SSE4a/JIT/patch/reference tests; Intel host is meaningful for emulation, no AMD GPU required. |

## All 82 resolved registrations

Environment/timeout/WILL_FAIL values below are from the retained generated CTest inventory, cross-checked with root registrations. `(inherit)` means CTest sets no environment override, so future runs must record/control inherited test-affecting variables. This is a source requirement table, not execution evidence.

| Case | Disposition | Environment override | CTest timeout / expected nonzero |
| --- | --- | --- | --- |
| `occlusion_reset` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `emulator_user_name_cli` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `emulator_user_name_too_long` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=true |
| `emulator_present_mode_cli` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `emulator_present_mode_invalid` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=true |
| `ime_dialog` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `upsync2_shader_ir` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `upsync2_shader_ir_linear_uses` | Offline shader NOT RUN | `KYTY_TEST_COMPARE_LINEAR_USES=1` | default; WILL_FAIL=false |
| `wave_reductions_linear_uses` | Offline shader NOT RUN | `KYTY_TEST_COMPARE_LINEAR_USES=1` | default; WILL_FAIL=false |
| `isa_accuracy_linear_uses` | Offline shader NOT RUN | `KYTY_TEST_COMPARE_LINEAR_USES=1` | default; WILL_FAIL=false |
| `shader_cfg` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `shader_cfg_cb_shader_mask_exports` | Offline shader NOT RUN | `KYTY_CB_SHADER_MASK_EXPORTS=1` | default; WILL_FAIL=false |
| `unused_pixel_inputs` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `pixel_interface` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `registered_shader_code_on` | Offline shader NOT RUN | `KYTY_REGISTERED_SHADER_CODE=1` | default; WILL_FAIL=false |
| `registered_shader_code_off` | Offline shader NOT RUN | `KYTY_REGISTERED_SHADER_CODE=0` | default; WILL_FAIL=false |
| `scalar_provenance` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `lane_local` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `image_page_table` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `texel_image_lookup` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `texture_set_cache` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `clear_register_decode` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `draw_prep` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `eop_timestamp` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `repeat_trace` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `binding_path` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cp_recorder` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cp_sequencer` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `memory_tracker` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `page_manager` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `archive_file` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `trophy_system` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `avplayer_file` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `vdecsw` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `hang_watchdog` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `hang_watchdog_fire` | CPU host-dependent NOT RUN | `(inherit)` | 15s; WILL_FAIL=false |
| `hang_watchdog_fatal_shutdown` | CPU host-dependent NOT RUN | `(inherit)` | 15s; WILL_FAIL=false |
| `profiler_counters` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cpu_placement` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cpu_placement_recorder` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cpu_placement_vcache_sets` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cpu_placement_vcache_affinity` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `cpu_placement_vcache_pending` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `live_switch` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `guest_probe_retire` | CPU host-dependent NOT RUN | `(inherit)` | 60s; WILL_FAIL=false |
| `guest_probe_shared` | CPU host-dependent NOT RUN | `(inherit)` | 60s; WILL_FAIL=false |
| `guest_probe_equals` | CPU host-dependent NOT RUN | `(inherit)` | 60s; WILL_FAIL=false |
| `u59_preset` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `shader_precompile` | CPU offline NOT RUN | `(inherit)` | 60s; WILL_FAIL=false |
| `pipeline_list` | CPU offline NOT RUN | `(inherit)` | 60s; WILL_FAIL=false |
| `pipeline_snapshot` | CPU offline NOT RUN | `(inherit)` | 30s; WILL_FAIL=false |
| `shader_vertex_metadata` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `resource_materialization` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `resource_tracking` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `event_queue_lifetime` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `event_queue_semantics` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `thread_service` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `kernel_file_system` | Vulkan required NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `sync_on_address` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `audio_out2_port` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `audio_out_timing` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `ngs2_sampler` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `audio_propagation` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `pad_haptics` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `controller_settings` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `host_input` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `save_data_memory` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `png_enc` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `ces` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `save_data_dialog` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `http_uri_parse` | CPU offline NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `guest_stack_exit` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `virtual_memory_allocation` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `virtual_memory_allocation_lazy_backing` | CPU host-dependent NOT RUN | `KYTY_GUEST_BACKING_LAZY_COMMIT=1` | default; WILL_FAIL=false |
| `virtual_memory_allocation_lazy_zero` | CPU host-dependent NOT RUN | `KYTY_GUEST_BACKING_LAZY_COMMIT=1; KYTY_GUEST_BACKING_LAZY_ZERO=1` | default; WILL_FAIL=false |
| `virtual_memory_allocation_clamp_memo_verify` | CPU host-dependent NOT RUN | `KYTY_CLAMP_RANGE_MEMO_VERIFY=exit` | default; WILL_FAIL=false |
| `virtual_memory_allocation_clamp_memo_off` | CPU host-dependent NOT RUN | `KYTY_CLAMP_RANGE_MEMO=0` | default; WILL_FAIL=false |
| `guest_red_zone_patcher` | CPU host-dependent NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `mesh_indirect_codegen` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `wave_row_reduction_codegen` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `fold_lane_masks_codegen` | Offline shader NOT RUN | `(inherit)` | default; WILL_FAIL=false |
| `amd_cpu_emulation` | CPU host-dependent NOT RUN | `(inherit)` | 120s; WILL_FAIL=false |

## Next single task: 16-case CPU build/run batch

This batch uses 16 confirmed CPU targets that do not compile the full emulator source set. It excludes topology-specific placement, exception/JIT/fixed-address stress, physical-device integration and the expensive full-emulator-linked tests. Source evidence is complete for selection; compilation and runtime behavior are still unverified. **Do not execute these commands during S-004.**

Use the S-002 x64 VS 2022 developer shell and project-local glslang PATH, cached Release/Ninja/clang-cl, launcher OFF, testing ON, PGO generation OFF/use empty, IPO OFF, empty MARCH. Record clean commit/submodule/toolchain/flag identity before building; retain local logs, hashes and dirty diff/untracked source inventory if applicable. Build one batch at two jobs; only on observed memory exhaustion retain failure log and retry one job. Run CTest serially, preserving registration properties.

```bat
cmake --build _Build/windows-core --target occlusion_reset_tests ime_dialog_tests image_page_table_tests texel_image_lookup_tests texture_set_cache_tests clear_register_decode_tests eop_timestamp_tests binding_path_tests cp_recorder_tests cp_sequencer_tests live_switch_tests u59_preset_tests shader_vertex_metadata_tests audio_propagation_tests png_enc_tests ces_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 -R '^(occlusion_reset|ime_dialog|image_page_table|texel_image_lookup|texture_set_cache|clear_register_decode|eop_timestamp|binding_path|cp_recorder|cp_sequencer|live_switch|u59_preset|shader_vertex_metadata|audio_propagation|png_enc|ces)$'
```

Acceptance: exact selection inventory is 16 named cases; build exits 0; serial run records all 16 results, failures/skips/internal exclusions and duration. Explain any failure without broadening into renderer changes; leave P0.4 incomplete while other confirmed CPU/offline cases and mixed-branch audit remain. Stop after committing evidence/checkpoint; do not package or begin P1. `null_texture_desc` stays in a later full-emulator-target batch.
