# P4.1 interrupted-work review — S-013, 2026-10-28

## Assessment

Useful partial implementation; P4.1 is not complete and the Vulkan harness is not ready to accept. Preserve and repair this work rather than restart it. The authored shader, shared host contract, offline validator, CMake registrations and provenance snapshot are useful. No production renderer source changed.

Reviewed base: `83edb3e2ab3ed0bfa2cca9be2d3fd68aca8ba09c`, branch `feature/rdna1-mesh-fallback`, plus five unfinished source files. All 14 recursive submodules match and pinned ancestry passed. Review leaves those five files unchanged and uncommitted; only review documentation is committed. No push.

## Verified execution

Before rebuilding, retained full dirty patches, raw modified/untracked source copies, hashes, cache and clean submodule identities under ignored `_Build/review-s013/pre-build/`. OpenCode's earlier `_Build/p4-s013/pre-build/` exists, but its prototype C++ hash differs from current source; older run logs cannot certify the latest edits. Other four source hashes match that earlier snapshot. Original artifacts were preserved.

Commands used the existing x64 VS 17.14.42 environment, clang-cl/LLD 19.1.5, CMake 3.31.6-msvc6, Ninja 1.12.1, Release, launcher OFF, two jobs, no PGO/IPO:

```text
cmake --build _Build/windows-core --target KytyGitVersion --parallel 2
cmake --build _Build/windows-core --target mesh_output_contract_tests mesh_compute_proto_tests --parallel 2
ctest --test-dir _Build/windows-core --output-on-failure --parallel 1 -V -R "^(mesh_output_contract|mesh_proto_spirv|mesh_compute_proto)$"
```

Build exit 0. CTest exit 0: host contract PASS (0.08 s), offline SPIR-V PASS (0.12 s), Vulkan runtime SKIPPED (0.21 s); total 0.50 s. This is **two passes and one skip**, not three completed tests. Both cached shader modules validate using pinned SPIRV-Tools: SPIR-V 1.6/Vulkan 1.3 and SPIR-V 1.5/Vulkan 1.2. Shader compilation was in OpenCode's build history; this review rebuilt C++ and revalidated the existing modules.

Runtime used the locally retained Khronos layer with process-local `VK_LAYER_PATH` and `VK_INSTANCE_LAYERS`; current code discovers its validation-features extension and enables synchronization validation. Device: Intel Iris Xe, vendor `0x8086`, device `0x9A49`, API `1.2.172`, raw driver version `0x0019255D`, driver name Intel Corporation. No ICD override was set. The test stops before logical-device creation at the bad lookup described below. No dispatch or readback occurred. No RDNA1 or Astro Bot run.

The local extracted CPU scenario probe also compiled and passed its diagnostic assertions, confirming `scenario_count=8` and `no_dispatch_initial_status=4294967295 expected_status=0`. It does not execute Vulkan. Logs, source fingerprints, commands, binaries and generated version identity are retained in `_Build/review-s013/`.

## Required fixes

Line numbers refer to the reviewed, uncommitted source snapshot.

1. **[P1] A working Vulkan loader is misclassified as unavailable.** `tests/MeshComputePrototypeTests.cpp:812–819` obtains `vkCreateDevice` through `iproc`, whose definition at line 497 passes a null instance. `vkCreateDevice` is not a global command: query it with the live instance. This directly reproduces the current exit-41 skip despite device/layer discovery. A bad required-function lookup is a harness failure, not evidence of absent hardware. See the [Khronos lookup rules](https://docs.vulkan.org/refpages/latest/refpages/source/vkGetInstanceProcAddr.html).

2. **[P1] Successful buffer allocation still fails its size assertions.** `GpuBuffer::size` is initialized to zero around line 388; `AllocateMappedBuffer` never assigns it. The checks at lines 990–993 therefore fail for both successfully allocated buffers. Record the successfully created buffer's requested extent (and allocation extent separately if needed). Also filter memory candidates by `requirements.memoryTypeBits` before ranking; currently the helper can choose an incompatible preferred type and reject despite another usable type.

3. **[P1] Scenario expectations are internally inconsistent.** `MakeScenarios` adds eight scenarios but line 1179 requires nine. The first is deliberately no-dispatch, yet `FinalizeScenario` runs the CPU writer and changes expected Pending to Ok (lines 237–260). Even a perfect GPU implementation must fail these checks. Define the intended case count; no-dispatch expectation must equal initial storage. Recovery currently uploads a CPU-synthesized previous error rather than retaining actual device output, so describe it as recovery from an injected stored error or implement a genuine two-dispatch reuse test.

4. **[P1] GPU writes lack the host-read visibility dependency.** The only recorded pipeline barrier is HOST_WRITE to COMPUTE (lines 1225–1231). After `vkCmdDispatch` there is no COMPUTE_SHADER/SHADER_WRITE to HOST/HOST_READ barrier before ending/submitting the command buffer. Fence wait and noncoherent invalidation are present, but do not replace that memory dependency. Add it before host readback. Follow the [Khronos compute-to-CPU example](https://docs.vulkan.org/guide/latest/synchronization_examples.html#_cpu_read_back_of_data_written_by_a_compute_shader).

5. **[P1] The final PASS does not enforce all claimed validation conditions.** Lines 615–652 allow missing synchronization-validation support or a missing/failed debug messenger and continue. The final output nevertheless claims a core-and-synchronization-validated pass. Require the requested validation facilities and a working error collector before accepting the run; otherwise report a precise skip/inconclusive result. Do not permit detected setup/assertion errors to be masked by later skip results.

6. **[P2] Resource teardown can generate errors after the pass decision.** `shader_module` is created at lines 850–855 and never destroyed. `cleanup_all` destroys the device without releasing it; the code checks `g_validation_errors` before teardown (lines 1289–1294), so a teardown error could still produce exit 0. Some early error branches also bypass `close_instance` and leak the debug messenger. Use complete cleanup on all paths, ensure submitted work has completed before destroying resources after a fence timeout, and include cleanup validation errors in the result. The old retained layer run already records a messenger-leak validation error; the latest code fixes one route but not all routes.

7. **[P2] Direct offline mode can report PASS after structural checks fail.** `CheckModuleShape` increments `g_failures`, but `RunOfflineValidation` returns only validator success (lines 179–192); `--validate-spirv` then returns zero without checking the assertion counter (line 1316). CTest's inherited failure-output regex can catch printed failures, but direct mode and `--run` must also honor them. Return failure on either shape or validator errors; verify using a valid module deliberately missing one expected binding/workgroup property.

8. **[P2] One documented skip code is not registered as a CTest skip.** Source returns 42 for no qualifying device, but `CMakeLists.txt:1400` recognizes only 41. Normalize genuine unavailable-environment cases to one registered skip code or register both using the supported CTest mechanism, without masking API errors.

## Scope and next acceptance gate

The extra Vulkan 1.2 variant may supply useful auxiliary Intel evidence, but it must be explicitly distinguished from the planned Vulkan 1.3 runtime gate. The discovered Intel driver only advertises Vulkan 1.2.172. Successful offline validation for Vulkan 1.3 does not make dispatch of the 1.2 module a 1.3 dispatch. Record a justified plan decision if retaining that broadened experiment; keep the original 1.3 runtime result pending/skipped where unavailable. Do not infer RDNA1 compatibility.

Next single task: repair and verify this P4.1 harness. Acceptance: required-function lookup works; available-device API failures fail rather than skip; scenario bookkeeping and expected blobs are correct; buffers use compatible memory types and recorded extents; explicit host-read dependency, fence and required cache operations are present; validation prerequisites and teardown errors control the outcome; direct validation failures and all genuine skips have correct exit semantics. Then require exact full-blob comparisons with zero validation errors on the documented runtime environment. Keep Vulkan 1.3, auxiliary Vulkan 1.2, Intel, software ICD and RDNA1 evidence separate. Retain exact source/commands/logs. No renderer integration, guest translation or next phase.
