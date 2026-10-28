# S-016 — First native RX package attempt (2026-10-28)

Scope: review the five user-supplied desktop logs, preserve them locally, and record the first native-device attempt. No source changes, build, local runtime rerun, registry edits, driver changes or next-phase development.

## Identity and execution

- Package source: `d3cc0bce63b32d8106f098a696646eaba0823f3b`; build ID `20261028T183644317Z`, matching S-015.
- Known original ZIP: 681,008 bytes; SHA-256 `9F36E3774F9C99B6E73F95D10216F8AE94E65ECD2A80E8216422360908F2A897`. Desktop archive checksum was not supplied independently; these logs do not remeasure it.
- Runner reports `Strict`, `requireRx5700Xt=true`; executable arguments `--run`. Exact shell/click invocation was not supplied. Equivalent launcher is `Run-Strict.cmd`.
- Runtime start UTC `2026-10-28T19:10:27.7815343Z`; duration 0.2630157 s; exit 1 / **FAIL**, not SKIPPED.
- Selected native device: AMD Radeon RX 5700 XT, DISCRETE_GPU, vendor `0x1002`, device `0x731F`, compute queue family 0. Loader 1.4.309; device API 1.4.308; AMD proprietary driver ID 1, raw version `0x00800156`, driver string `25.6.1 (AMD proprietary shader compiler)`, conformance 1.4.0.
- User-reported target machine specifications remain as in the plan; OS build, measured system RAM/CPU and ICD DLL path/hash were not supplied. Driver identity is reported above, without inventing an ICD-file audit.
- Recorded driver/layer override variables were unset. The existing runner does not record `VK_IMPLICIT_LAYER_PATH` or `VK_ADD_IMPLICIT_LAYER_PATH`; their original values are unknown.

## Actual results

| Category | This desktop evidence |
| --- | --- |
| Compilation | NOT RUN; existing S-015 clean build evidence retained |
| Unit tests | PASS: host-contract exit 0 (0.0144098 s), host-fixtures exit 0 (0.00719 s) |
| Shader compilation and SPIR-V validation | Compilation NOT RUN; validation + shape PASS for both packaged modules, exit 0 (0.0080949 s): Vulkan 1.3 / SPIR-V 1.6, 671 words; auxiliary Vulkan 1.2 / SPIR-V 1.5, 675 words |
| Vulkan integration | Strict Vulkan 1.3 attempt FAIL before scenarios: two loader ERROR messages, counted through cleanup. Vulkan 1.2 runtime NOT RUN on desktop; earlier Intel auxiliary result remains separate |
| RDNA1 hardware | Native device discovery/selection succeeds; full compute/readback gate FAIL before dispatch. None of the nine scenarios executed |
| Astro Bot compatibility | NOT RUN |

Two instance-creation loader errors name missing Steam manifests:

- `SteamOverlayVulkanLayer64.json`
- `SteamFossilizeVulkanLayer64.json`

Both are reported under the standard Program Files (x86) Steam directory. Khronos validation is available, synchronization validation is enabled, and the strict 1.3 module loads. Output/payload buffers are allocated (1840/1792 bytes, both coherent). The source error-counter gate before the scenario loop (`tests/MeshComputePrototypeTests.cpp`) aborts on the earlier loader errors. There are no scenario/submission/readback lines. Final result: `failures=0 validation-errors=2 including cleanup`.

This establishes a loader/environment blocker, not shader correctness or a mesh-emulation failure. Stale Steam layer registrations are a plausible explanation, not a registry inspection result. Do not ignore these messages or relax acceptance. GPU duration/utilization/FPS/VRAM metrics and screenshots are unavailable because execution stops before GPU work; desktop rollback was NOT RUN. Launch and module discovery are partial deployment evidence only.

## Next single task: isolated strict rerun on the RX desktop

Use the same package, a normal non-Administrator PowerShell in the extracted package directory, and a process-local empty implicit-layer search directory. Keep explicit Khronos core/synchronization validation required. Do not change registry entries or select a software ICD.

The loader's [version 1.4.309 documentation](https://github.com/KhronosGroup/Vulkan-Loader/blob/v1.4.309/docs/LoaderLayerInterface.md#windows-layer-discovery) specifies that `VK_IMPLICIT_LAYER_PATH` overrides implicit-layer registry discovery, independently of explicit layers; elevated processes ignore the override. This is a diagnostic rerun, not an already verified repair of this desktop. If errors persist, retain the new failure and inspect layer discovery/registrations read-only before choosing a repair.

```powershell
$oldImplicit = $env:VK_IMPLICIT_LAYER_PATH
$emptyImplicit = Join-Path $env:TEMP ('KytyMeshImplicit-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $emptyImplicit | Out-Null
try {
    $env:VK_IMPLICIT_LAYER_PATH = $emptyImplicit
    .\Run-Strict.cmd
} finally {
    $env:VK_IMPLICIT_LAYER_PATH = $oldImplicit
}
```

Record this command and override alongside the new logs (the current runner does not include this variable in result.json). Preserve the original failure. Acceptance: native RX 5700 XT, strict Vulkan 1.3 module, all nine full-buffer comparisons (eight dispatches plus Pending initialization), zero validation errors including cleanup, exit 0 / PASS, retained source/package/device/driver identity and logs. Missing prerequisite means SKIPPED; API/integrity/validation/comparison errors mean FAIL. No renderer integration or next phase yet.

## Evidence preservation

The five original inputs (`host-contract.log`, `host-fixtures.log`, `spirv.log`, `runtime.log`, `result.json`) are copied unchanged to ignored `_Build/p4-s016/rx-input/`; SHA-256 inventory is `_Build/p4-s016/input-hashes.json`. Originals are untouched; raw private deployment paths are not committed. This sanitized report is the shareable record. Documentation input HEAD: `a974e595412a35e2a4a072f42d452dfcc8dabade`; branch `feature/rdna1-mesh-fallback`. No push.
