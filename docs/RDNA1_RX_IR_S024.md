# S-024 — Native RX execution of IR-generated compute modules

Reviewed seven user-supplied logs/results, run UTC 2026-10-28T21:41:26Z. Input checkout `280e9bc4a47254a54070d4d20189873535b6fefe`, branch `feature/rdna1-mesh-fallback`, clean with pinned ancestry and all 14 recursive submodules matched. Copies and SHA-256 inventory are retained in ignored `_Build/p3-s024/`; original files were not modified.

Package reports clean source `e74e19779f13e33cf3c3a30fcabb58acbf952ad1`, build `20261028T213608114Z`. Desktop ZIP was not supplied or independently rehashed. Runner records strict mode, required RX, IR mode and process-local implicit-layer isolation. Device selected for each case is AMD Radeon RX 5700 XT, driver 25.6.1, API 1.4.308; loader 1.4.309. Khronos validation and synchronization validation were enabled for each case.

The six reported stages (host contract, fixtures, SPIR-V, IR positive, IR rejection, runtime) all PASS and exit 0. Runtime wall time 0.2841633 seconds. The strict Vulkan 1.3 triangle, permuted and zero generated modules each dispatch; the runtime reports exact complete 1840-byte/460-word buffer comparisons and zero errors and warnings through cleanup. Counts/status are 3/1/0, 3/1/0 and 0/0/0 respectively. Runtime comparison is the evidence: raw readback bytes were not supplied for independent comparison. Both mapped buffers report coherent memory; noncoherent behavior remains untested.

| Category | Actual result |
| --- | --- |
| Compilation | NOT RUN on desktop; prior clean package build evidence retained |
| Unit tests | PASS: packaged host contract, fixtures, IR positive and rejection stages |
| Shader compilation / SPIR-V | Compilation NOT RUN; offline validation PASS for packaged modules and strict generated modules |
| Vulkan integration | PASS: three generated strict modules and full buffers; no desktop Vulkan 1.2 runtime |
| RDNA1 hardware | PASS for this limited native RX synthetic IR subset |
| Astro Bot compatibility | NOT RUN |

GPU timestamp/utilization/FPS/peak VRAM, desktop OS build and ICD DLL path/hash were not provided. No screenshot is expected for this buffer-only run. Host elapsed time is not a GPU performance measurement. P3.2's restricted gate is DONE; broader P3, production renderer integration, real guest translation and game compatibility remain open. Prior Intel Vulkan 1.2 results are separate and prior failures remain recorded.

Next task is P3.3: bounded offline `IAdd32` lane-expression support with finite output coverage and index safety proofs, independent positive/negative expectations, fail-closed unsupported cases, Vulkan 1.3 SPIR-V validation and focused regressions. Runtime/RX validation of expanded semantics remains a separate gate. No next task was implemented in S-024.
