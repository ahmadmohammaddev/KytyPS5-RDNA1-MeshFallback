# P0.4 host timer/thread batch — S-007

Source `40626837f0f76d99096862a6337cf84e4ad28295`, branch `feature/rdna1-mesh-fallback`. Clean before/after execution, all 14 matching recursive submodules; pinned source diff and ancestry checks PASS. No source changes.

Build PASS, exit 0, 20.80 s, UTC start 2026-10-27T19:30:39.8499760Z, 20 Ninja actions, two jobs. Initial serial CTest **6/7 PASS, 1 FAIL, 0 skips**, exit 8, 7.83 s. Isolated thread_service retry also FAIL, exit 8, 2.19 s. Both attempts outside sandbox; no source workaround or third retry.

S-002 x64 VS developer environment/local glslang PATH. Toolchain logged: VS 17.14.42, CMake 3.31.6-msvc6, Ninja 1.12.1, LLVM/LLD 19.1.5 x64, MSVC headers 14.44.35207, SDK 10.0.26100.0. Cache Release/testing ON/launcher OFF/PGO generate OFF/use empty/IPO OFF/empty MARCH. No configure rerun or memory exhaustion/one-job retry. Nonfatal warnings retained. Header full SHA matches input, clean short hash 4062683, version u59-windows-20261024-int21-7-g40626837. No emulator rebuild or package.

```bat
cmake --build _Build/windows-core --target event_queue_lifetime_tests event_queue_semantics_tests thread_service_tests sync_on_address_tests hang_watchdog_tests --parallel 2
```

```powershell
ctest --test-dir _Build/windows-core --show-only=json-v1 -R '^(event_queue_lifetime|event_queue_semantics|thread_service|sync_on_address|hang_watchdog|hang_watchdog_fire|hang_watchdog_fatal_shutdown)$'
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 --output-junit <local-evidence>/results.xml -R '^(event_queue_lifetime|event_queue_semantics|thread_service|sync_on_address|hang_watchdog|hang_watchdog_fire|hang_watchdog_fatal_shutdown)$'
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 --timeout 120 --output-junit <local-evidence>/retry.xml -R '^thread_service$'
```

Existing CTest properties retained: watchdog fire/fatal timeout 15 s. Selection exactly seven.

| Case | Initial result | Seconds |
| --- | --- | ---: |
| `hang_watchdog` | PASS | 0.079 |
| `hang_watchdog_fire` | PASS | 4.538 |
| `hang_watchdog_fatal_shutdown` | PASS | 0.062 |
| `event_queue_lifetime` | PASS | 0.154 |
| `event_queue_semantics` | PASS | 0.189 |
| `thread_service` | FAIL | 2.310 |
| `sync_on_address` | PASS | 0.435 |

## Failure and limits

`thread_service` reports `ThreadServiceTests: failed: a precise timed wait ends within 0.4 ms of its deadline`, exception exit 0xc0000409 in both runs. `tests/ThreadServiceTests.cpp:220` measures 25 waits of 5 ms and takes median lateness; lines 245–252 allow three attempts to achieve median below 400 us. No threshold modified. Buffered diagnostic stdout did not expose actual medians in the retained failure output, so measured lateness and root cause remain unknown. Host load/timer resolution/power policy were not measured; scheduler sensitivity is a hypothesis, not a proven excuse or a pass. Next session must diagnose this failure.

No internal skip markers found in initial detail log. Parent relevant environment captured locally. Compilation PASS; Unit tests FAIL (6 passes/1 failure, retry failure); Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Across sessions 63 distinct cases have passing executions, thread_service has only failures, 361 registered cases never run. S-005 failure/retry retained.

Local ignored `_Build/p0-s007/`: build script/log/timed result, cache, selection/generated CTest JSON, source SHA/submodule list, before/after status/diff/untracked inventories, generated header, environment, initial test log/XML/timing/detail, isolated retry log/XML/result/detail, five executable SHA-256s and documentation checks. No binary/private assets committed; no GPU/driver/ICD selected, screenshots/performance metrics N/A.

## Next single task

P0.4 thread_service timing failure diagnosis only: inspect tests/ThreadServiceTests.cpp:220 and src/common/condWaitUntil.h, capture unbuffered median-lateness/timing evidence and actual host timer/scheduler conditions, and distinguish host scheduling limits from implementation/fixture defects. Gate: reproduce the named assertion with measured values and a source-backed cause or bounded remaining hypotheses; propose a minimal follow-up. Keep thresholds/assertions unchanged in this diagnostic subtask; no other batch, GPU, packaging or P1.
