# thread_service timing diagnosis — S-008

Input `3e7e5bb3175a4a9484b6c32ed73c347a5c4ca116`, branch `feature/rdna1-mesh-fallback`. Required Git status/log/branch/submodules verified: clean input, all 14 revisions match. Diagnostic builds dirty by design; diffs and untracked inventory retained. No renderer changes.

Two diagnostic builds: S-002 x64 VS developer shell, LLVM 19.1.5, CMake/Ninja, cached Release/testing ON/launcher OFF/no PGO/no IPO, two jobs. Commands retained in `_Build/p0-s008/build.cmd`: `cmake --build _Build/windows-core --target thread_service_tests --parallel 2`. Both exit 0. No memory exhaustion/retry. Direct serial invocation `_Build/windows-core/thread_service_tests.exe` outside sandbox, no other suite. First diagnostic adds unbuffered stdout only; second adds temporary component probes. Both fail unchanged assertion (exit -1073740791 = 0xc0000409). Final tracked change retains only `std::setvbuf(stdout, nullptr, _IONBF, 0)` at main entry; temporary probes removed, exact source diff retained locally. Final code equals first diagnostic source; last on-disk binary includes second probe and must not be represented as the final committed binary or distributed.

| Observation | First run | Component-probe run |
| --- | --- | --- |
| Precise 5 ms wait median lateness, three attempts | 10449, 10548, 10564 us | 10971, 10994, 10575 us |
| Coarse-only 5 ms wait median lateness | 10869 us | 10966 us |
| Direct cv.wait_for(2500 us), 25 samples, median elapsed | Not measured | 15959 us |
| Direct Thread::SleepMicro(500), 25 samples, median elapsed | Not measured | 1116 us |
| Result | FAIL, deadline threshold | FAIL, same assertion |

No timing threshold, retry count, assertion or production wait code changed. A 15.959 ms coarse phase exceeds the entire 5 ms deadline; this is sufficient to explain the approximately 10–11 ms lateness. `src/common/condWaitUntil.h` first uses cv.wait_for(remaining - 2500 us) when remaining exceeds 2500 us. It does not enter its fine-sleep/yield path until that wait returns; after an oversleep the loop exits at its deadline check. Therefore this host violates the coarse-margin assumption. The high-resolution path cannot repair time already lost. The 500 us SleepMicro probe also oversleeps and must be considered when designing the fix. This is a measured implementation/host interaction, not proof that every failure is caused by machine load.

Read-only host snapshot: Windows 11 Home 10.0.26300/build 26300; Intel i5-1135G7, 8 logical processors; CIM LoadPercentage 20 (one pre-run snapshot, not peak/concurrent trace). Balanced power scheme. QPC frequency 10,000,000 Hz. NtQueryTimerResolution status 0, min/max/current raw 100 ns units 156250/5000/10000 (15.625/0.5/1 ms). A system-reported current resolution is not proof that this process’s condition-variable waits receive it. No timer-resolution or power/scheduler setting changed. Exact OS internals responsible for 15.96 ms waits not proven; process timer policy, scheduler/power behavior remain bounded hypotheses. No kernel trace taken. Early-signal assertion is not reached after failure, so no claim about it passing.

Source chain: `tests/ThreadServiceTests.cpp:220` measures 25 waits/median and retries three times below 400 us; `src/common/condWaitUntil.h` coarse margin 2500 us, spin margin 300 us, fine slice 500 us; `src/common/threads.cpp:61` implements SleepMicro via spin/high-resolution timer/fallback. Code inspection and measured coarse latency explain why increasing retries alone does not fix this behavior. No assertion weakening or global timeBeginPeriod workaround introduced.

Compilation PASS (diagnostic target); Unit tests FAIL (two diagnostic reproductions); Shader compilation/SPIR-V validation NOT RUN; Vulkan integration NOT RUN; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Existing counts remain 63 distinct passing cases, one failed-only, 361 never run. No new baseline pass.

Local ignored evidence `_Build/p0-s008/`: sha/submodules/status/untracked, cache, diagnostic-source.diff and diagnostic-source-02.diff (dirty source identity), build.cmd/log/results for both variants, diagnostic.log/-result.txt and diagnostic-02.log/-result.txt, timer.json, cpu.json, os.json, power.txt, generated-version.h and diagnostic-hash.txt. The header correctly identifies dirty input. No GPU/driver/ICD, screenshots/FPS/VRAM metrics applicable; no binary/private assets committed.

## Next single task

P0.4 precise CondWaitUntil Windows wait fix only: prevent the coarse condition-variable phase from overshooting the deadline under coarse host scheduling, while preserving wake/lock/dispatch semantics and existing timing assertions. Gate: focused thread_service build at 2 jobs and repeated serial measured runs pass both deadline and early-signal assertions with source/host/provenance evidence; record failures, commit and stop. No global timer/power-policy changes, unrelated batch, GPU, packaging or P1.

S-009 priority update: the recommendation above is historical and superseded by plan D-016. Retain this failure as a known baseline issue; no timer repair without direct evidence blocking mesh development. Next task is the targeted output contract in docs/rdna1-mesh-architecture.md.
