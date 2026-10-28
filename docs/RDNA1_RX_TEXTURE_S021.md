# S-021 — Native RX textured triangle and guarded UVs PASS

Reviewed 2026-10-29 (Europe/Istanbul). One task: verify and record the user's P4.3 desktop evidence; no new implementation, build, runtime rerun or system change.

## Identity and execution

Package source `90bd8de180ff78fbc374c2947304510022aa500d`, build ID `20261028T204207755Z`, matches final S-020 package. Known original package ZIP 733,420 bytes / SHA-256 `4871EE046779EE292AA650FB4D462662B993E31E3B2D24EC823A9BE9CD009870`; desktop ZIP rehash not separately supplied.

User evidence archive `20261028T210012081Z-fc51dba4.rar`: 5,735 bytes, SHA-256 `8EFB820C11EF37FEC28E2508B4699192CBC5DB1212DB550D16B390D8CA650FB1`. Listed before extraction: eleven expected ordinary result files and an empty implicit-layer directory; no executables or traversal paths. Extracted into a fresh ignored evidence directory. The subsequently supplied uncompressed eleven files are byte-identical to the archive contents; these are the same run, not an additional experiment.

Runner mode Strict, draw=true, texture=true, requireRx5700Xt=true, isolateImplicitLayers=true. Runtime arguments `--texture --image-output <external-results>/triangle.ppm`; equivalent launcher configuration `Run-Textured.cmd -IsolateImplicitLayers`, exact shell/click transcript not supplied. Runtime start `2026-10-28T21:00:12.1670089Z` (2026-10-29 00:00:12.1670089 UTC+03), duration 0.1432311 s, exit 0 / PASS. All **eight stages PASS**, none FAIL/SKIPPED.

Selected native AMD Radeon RX 5700 XT, DISCRETE_GPU, vendor `0x1002`, device `0x731F`, API 1.4.308, queue family 0 graphics+compute. Loader 1.4.309; AMD proprietary driver ID 1, raw `0x00800156`, string `25.6.1 (AMD proprietary shader compiler)`, conformance 1.4.0. Strict 1.3 modules execute as SPIR-V 1.6; no downgrade. Device-local property flags/ICD DLL path/hash, OS build, measured CPU/RAM and GPU utilization/timestamps/peak VRAM not supplied. User-reported machine specifications remain separate.

Implicit-layer search is recorded as an empty results subdirectory; other logged layer/driver/ICD overrides unset. Khronos core/synchronization validation and debug collector enabled; no missing-Steam-manifest errors. Raw private paths remain local.

## Actual results

| Category | This desktop evidence |
| --- | --- |
| Compilation | NOT RUN this attempt; S-020 clean package build evidence retained |
| Unit tests | PASS host-contract, host-fixtures, draw-oracle and texture-fixtures stages |
| Shader compilation and SPIR-V validation | Compilation NOT RUN; all twelve packaged modules validate offline for their explicit 1.3/1.2 targets |
| Vulkan integration | PASS strict Vulkan 1.3: original nine contract cases, untextured clear/draw and textured clear/draw; all image/blobs exact |
| RDNA1 hardware | PASS scoped P4.3 authored texture/compute-UV fixture on native RX; UV zero/capacity/overflow/recovery exact, zero errors/warnings after cleanup |
| Astro Bot compatibility | NOT RUN |

Original Pending/zero/triangle/capacity/three-overflow/two-recovery cases PASS. All four offscreen controls report zero of 4096 RGBA pixels mismatching, original full 460-word blob checked. Textured clear/draw also check every one of 136 UV words, including guards/count/status/unused slots. Actual UV count 0 PASS/Ok, count 64 PASS/Ok, rejected 65 PASS/status1 with full snapshot preserved, then same-buffer count 3 recovery PASS/Ok. Output and UV allocations are type 2/coherent; uploads/readbacks coherent. No noncoherent runtime claim.

Final summary after cleanup: `scenarios=9 failures=0 validation-errors=0 validation-warnings=0`, then `TEXTURE RESULT: PASS`, `DRAW RESULT: PASS`, strict Vulkan 1.3 PASS. Desktop Vulkan 1.2 runtime NOT RUN; its modules' offline validation and prior Intel auxiliary passes stay separate.

## Independent image verification / retained evidence

Both supplied 64x64 P6 RGB files are 12,301 bytes and match JSON and prior reference SHA-256s:

| File | SHA-256 | Independent full RGB check |
| --- | --- | --- |
| triangle.ppm | `E613B75B2BBA9047B64F21CB35E919E90376872D13E1105BE811F6E504ECA424` | 64 red / 156 green / 156 blue / 120 yellow / 3600 black, zero mismatches |
| untextured.ppm | `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D` | 496 white / 3600 black, zero mismatches |

Independent check uses floating-point barycentric UV interpolation/clamped texel lookup, different from harness integer pixel thresholds. Visual inspection confirms the textured triangle. RGB PPM has no alpha; full RGBA/alpha correctness is evidenced by runtime, not inferred from the images alone. Host elapsed time is not GPU performance. No screenshot/performance trace supplied; actual readback images retained instead. Desktop rollback/general emulator deployment remain unverified.

Original archive and eleven extracted files retained unchanged under ignored `_Build/p4-s021/`; originals untouched. `input-hashes.json`, `verification.json`, textured/untextured PNG previews local only. No raw private paths or archive committed. Review input `4300b5d2c10ece70ee9afc348c0e98a499c73c00`, branch `feature/rdna1-mesh-fallback`, clean input/pinned ancestry/all fourteen matching submodules checked. Files changed: this report, plan, AGENTS. No push.

## Status / next recommended one task

**P4.3 DONE (with native hardware evidence)**; no blocker for this bounded gate. P4.1/P4.2/P4.3 now all pass on RX. Overall P4 remains IN_PROGRESS for broader memory/measurement and semantic coverage; public authored shader success does not establish guest mesh translation or Astro Bot compatibility.

Next **P3.1 offline minimal guest Mesh IR-to-compute lowering proof** (NOT_STARTED), under D-024: use an authored synthetic repository IR fixture for position output plus one triangle, not another handwritten GLSL replacement. Verify existing NGG prologue/MeshAllocate/masked-export semantics before selecting the smallest supported subset; isolate the compute backend and retain native behavior by default. Acceptance: explicit supported/unsupported subset and fail-fast tests, deterministic compile of that fixture into SPIR-V GLCompute with the guarded output ABI, no MeshShadingEXT/OpSetMeshOutputsEXT dependency, Vulkan 1.3 SPIR-V validation and focused regression tests, complete provenance. Unsupported lane/LDS/primitive semantics must be rejected, not silently erased. Offline success alone cannot close P3: newly generated modules require a later independent RX dispatch/readback. No renderer/game integration or next-task implementation in this review. Remaining P4 memory/metrics work remains recorded, not declared complete.
