# Standalone RX-test package — S-015, 2026-10-28

## Result

Portable synthetic package preparation is complete and locally verified. This is **not an emulator/game patch**. Native RX 5700 XT execution, strict Vulkan 1.3 execution, desktop deployment/rollback and Astro Bot compatibility remain unverified.

Package source: clean committed `d3cc0bce63b32d8106f098a696646eaba0823f3b` on `feature/rdna1-mesh-fallback`. Build ID: `20261028T183644317Z`.

Artifact (ignored, local):

```text
_Build/mesh-prototype-packages/Kyty-MeshPrototype-Windows-x64-d3cc0bce63b32d8106f098a696646eaba0823f3b-20261028T183644317Z.zip
```

Size: **681,008 bytes**. ZIP SHA-256:

```text
9F36E3774F9C99B6E73F95D10216F8AE94E65ECD2A80E8216422360908F2A897
```

Matching `.zip.sha256` sidecar is beside the ZIP. Package manifest, file checksums, complete retained licenses/notices, launcher and source excerpts are inside. The generated Git header is clean `d3cc0bc`/full source SHA. The final handoff commit changes documentation only and is not the package's build identity.

## What changed

- Shader lookup uses the executable's directory plus `gpu_mesh_proto_shaders`, through Unicode-capable Windows path discovery; no working-directory dependence or fallback to the old build tree. Removed compile-time absolute shader-path definitions.
- `tools/mesh-prototype/Build-Package.ps1` refuses dirty source/submodules, verifies pinned ancestry, configures no-PGO/no-IPO portable x64 flags, builds two targets with two jobs, runs the six focused tests serially, audits PE imports and stages an explicit allowlist. It records clean source/recursive submodules/toolchain/flags/test statuses in a sanitized manifest, writes all file hashes and the ZIP hash, and verifies the staged package before archiving. All outputs/evidence stay under `_Build`; no deletion, push, installer or system change.
- `Run-Prototype.ps1` verifies exact inventory and SHA-256 before native execution, saves logs/JSON outside the package, uses explicit Offline/Strict/Vulkan12 modes and supports a required RX 5700 XT identity guard. Exit 0=PASS, 41=SKIPPED, other nonzero=FAIL; unrequested runtime=NOT RUN. `Run-Strict.cmd` defaults to strict Vulkan 1.3 plus the RX guard. Optional validation-layer path is process-local and restored.
- Hashing and ZIP creation use .NET APIs for reliable Windows PowerShell 5.1 operation; the runner avoids a PS5 JSON-array nesting difference. Package instructions/notices preserve upstream Kyty/KytyPS5, SPIRV-Tools and SPIRV-Headers credit/licenses.

## Actual verification

Input `3f498e8c7a2743eecc8b655884638011c604ebc8` was clean; branch, pinned ancestry and all 14 matching recursive submodules verified. Dirty portability build retained its full patch/raw source/untracked inventory/hash/cache/submodule snapshot under `_Build/p4-s015/pre-build-dirty/`; focused CTest 5 PASS/1 SKIPPED, exit 0, 1.28 s. The packager's dirty-source rejection was tested before its first clean-source build.

Clean package command (x64 VS developer environment, local glslang on child PATH):

```powershell
tools\mesh-prototype\Build-Package.ps1 -ValidationLayerDirectory "<local-layer-directory>"
```

The final clean-source attempt at `d3cc0bce` PASSed. Toolchain: VS 17.14.42, clang-cl/LLD 19.1.5, CMake 3.31.6-msvc6, Ninja 1.12.1, glslang 16.6.0, MSVC headers 14.44.35207, SDK 10.0.26100.0; Release/launcher OFF/BUILD_TESTING ON/PGO OFF/use empty/IPO OFF/empty MARCH. Incremental two-job builds, serial tests, no memory exhaustion/retry. Native source is unchanged from the tested portability build; cache reuse is recorded, not a fresh full-emulator build claim.

Build-time CTest: **5 PASS, 1 SKIPPED, 0 failures**, exit 0, **0.65 s**. Names: `mesh_output_contract`, `mesh_proto_spirv`, `mesh_proto_host_fixtures`, `mesh_proto_reject_shape`, `mesh_compute_proto` (strict 1.3 skipped), `mesh_compute_proto_vk12` (auxiliary Intel pass). No broad baseline suite or core/GUI build. Existing 431-case inventory/counts remain unchanged; packaging probes are not extra registered passes.

PE import audit found only Windows UCRT/KERNEL32 and MSVCP140/VCRUNTIME140/VCRUNTIME140_1 dependencies. Therefore the package needs a suitable **external Microsoft Visual C++ v14 x64 runtime**. It also requires the target GPU's native Vulkan driver and external Khronos validation layers; no driver/loader/layer/runtime DLLs or installers are bundled. Shader compilation tools, Qt, SDL, game assets, firmware, keys and proprietary shaders are absent. The package has 16 payload files plus manifest/checksum inventory. Manifest checked for absolute local paths; none found.

The final ZIP hash was checked before extraction. It was extracted into a new path containing **spaces and Arabic characters**, then invoked from an unrelated working directory under Windows PowerShell 5.1. Eight probes matched their intended exit statuses:

| Probe | Actual result |
| --- | --- |
| Package inventory/hash verification | PASS, exit 0 |
| Packaged host fixtures + both offline SPIR-V validators | PASS, exit 0; runtime NOT RUN |
| Packaged strict 1.3 + RX guard | SKIPPED, exit 41; no qualifying 1.3 device on laptop |
| Packaged explicit Vulkan 1.2 | PASS, exit 0; all nine 460-word blobs equal, eight dispatches, zero validation errors/warnings after cleanup |
| Require RX identity during an Intel auxiliary run | Expected FAIL, exit 1; Intel is not accepted as RX evidence |
| Alter one public SPIR-V byte in a copied package | Expected FAIL, exit 1, checksum mismatch before execution |
| Remove a shader from a copied package | Expected FAIL, exit 1, integrity check before execution |
| Invoke raw copied executable with the shader absent while original build files remain available | Expected FAIL, exit 1, module missing; proves no build-directory fallback |

The native device remains Intel Iris Xe vendor 0x8086/device 0x9A49, API 1.2.172, raw driver 0x0019255D/Intel Corporation; loader API 1.4.313. Core + synchronization validation used the existing local layer directory, not a bundled layer. Coherent type 1 memory exercised; noncoherent runtime path NOT RUN. No screenshots/FPS/utilization/VRAM peaks for this buffer-only correctness test.

Failed packaging attempts are retained separately: clean `82836d3c` failed because the child PowerShell lacked `Get-FileHash`; clean `087c03f3` failed staged verification because PS5 wrapped the checksum JSON array as one object. Both were fixed in subsequent commits; neither produced the handed-off ZIP. A local, non-transferable runner fixture verified the PS5 fix before the final attempt. The relocation controller initially inherited the last intentionally failing probe's exit 1; its explicit aggregate exit was corrected and saved eight-probe results/archive identity were independently verified with aggregate exit 0. No product test failure or old log was erased.

All local proof is under `_Build/p4-s015/` (attempt logs/exits, relocation script/results, expected-failure logs and runtime results). The final package's separate `*-evidence/` directory retains configure/build/tool/version/import/CTest logs, full cache, source identity/submodule state and package-result JSON. Earlier partial staging directories remain evidence, not transfer artifacts. Source/tooling commits: `82836d3c`, `087c03f3`, `d3cc0bce`; final handoff is documentation only.

## Six categories and next task

Compilation PASS (focused clean-source incremental package build); Unit tests PASS; shader/SPIR-V validation PASS; Vulkan integration PASS only for auxiliary Intel 1.2, strict 1.3 SKIPPED; RDNA1 hardware NOT RUN; Astro Bot compatibility NOT RUN. Portable-package local verification PASS; independent desktop extraction/deployment/rollback NOT RUN. P0 baseline and broad emulator packaging remain incomplete under D-016/D-020.

Next single task: copy **only the final ZIP and matching sidecar** to the RX machine, verify archive and file checksums, extract into a new directory, satisfy external runtime/validation-layer prerequisites, then execute `Run-Strict.cmd`. Acceptance: native RX 5700 XT/driver/ICD identified, Vulkan 1.3 module actually dispatched, all nine full-blob checks pass with zero validation errors including cleanup, and `result.json`/logs retained with this package identity. Missing prerequisites are SKIPPED, not PASS; an unexpected API/checksum/validation failure is FAIL. Do not launch Astro Bot or advance renderer integration in that task.
