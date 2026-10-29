# S-053 clean combined-stage native handoff

Branch feature/rdna1-mesh-fallback. Clean package source `ad57feab7d794b7cedb855a6fb5411ca76881200`, build `20261029T122933634Z`. Final documentation commit does not change archive identity. No push.

S-052 implements paired-draw: actual frontend geometry+UV/zero compute, generated conventional vertex bridge and actual top-level production-compiled public pixel fragment. Exact geometry1840 bytes, UV544 bytes, full RGBA64x64, clear/zero controls and barriers/fence/mapped handling verified. Native pixel produces UV colors EXP(x,y,x,y); it does not sample a texture. Borrowed-UV helper retains unused texture resources but never substitutes authored compute for paired UV generation. Paired mode requires float32 signed-zero/Inf/NaN preservation and rejects all validation warnings/errors through cleanup. Native/default renderer/cache/scheduler unchanged.

S-053 extends existing package tooling with CombinedMilestone plus required-RX strict launcher.14 preflight checks and five runtime stages total19, twelve strict native device selections/cleanups/images. Old launchers preserved. Full compiler test produces/validates native modules during build, not shipped as an emulator executable.90 allowlisted payload files plus manifest/checksum inventories,44 shader modules/public producer/bridge/oracle sources/licenses/import audit. All36 prior modules byte-identical.

## Actual checks

Clean package41 serial cases:31 PASS/10 strict SKIPPED/0 FAIL,5.11s. Build2 jobs PASS, no memory retry/PGO/IPO. Dirty S-052 final suite same counts5.10s; snapshots p3-s052-pre-build-01/-02 preserve exact sources. Initial indentation warning fixed before final dirty build; no compile/test failures this step except intentionally rejected mutations.

After Unicode/space relocation and unrelated-CWD invocation:15 expected package outcomes verified (integrity/offline/auxiliary/strict/RX guard/five legacy/conflict/missing integrity/missing direct/checksum tamper/launcher). Relocated auxiliary all19 stages PASS,12 exact complete images/full buffers and zero validation errors/warnings through12 cleanups. Strict runtime SKIPPED, never downgraded. RX guard on Intel FAILS as intended. Valid bridge x/y-swap from packaged executable validates offline but FAILS488 pixels with zero validation errors; missing strict bridge FAILS before discovery. Including valid wrong bridge gives16 expected verification outcomes. All12 image files hash-match independently checked references (49,152 RGB pixels); paired images additionally independently derived using rational arithmetic in S-052.

| Category | Actual result |
| --- | --- |
| Compilation | PASS clean focused package targets including full-emulator-linked compiler producer; no GUI build |
| Unit | PASS policy/key/bridge/native-pixel/offline/contract/IR/UV/oracle cases scoped |
| Shader / SPIR-V | PASS actual native pixel compiler, generated bridge, source shaders and separately assembled strict1.3/auxiliary1.2 modules |
| Vulkan integration | PASS Intel Iris Xe1.2 auxiliary, complete paired stages and previous regressions; strict1.3 locally SKIPPED(API1.2.172) |
| RDNA1 | New combined actual-stage gate NOT RUN; prior scoped native frontend milestones retained |
| Astro Bot | NOT RUN |

Commands: VS developer shell Build-Package.ps1 -ValidationLayerDirectory _Build/p4-s013/tools/validationlayers -Jobs2 (arguments `-Jobs 2`); builder retains exact configure/version/build/41-case CTest/import/integrity commands. `powershell.exe -NoProfile -ExecutionPolicy Bypass -File _Build/p3-s053/verify.ps1` exit0; isolated paired valid mutation commands and independent image/module checks retained under ignored `_Build/p3-s053/`. Full source/build identity and test provenance in package -evidence directory. No noncoherent/performance/production/game claim.

## Exact next native task

Archive `Kyty-MeshPrototype-Windows-x64-ad57feab7d794b7cedb855a6fb5411ca76881200-20261029T122933634Z.zip`,2,780,495 bytes.
SHA256 `7DC0B74CB00106E63B706B14F15C6311D6C38318A78E652487BF1F98799F6469`, matching .zip.sha256 sidecar.

Extract on RX5700XT, normal user from package folder:

```cmd
.\Run-Combined-Milestone.cmd -IsolateImplicitLayers
```

Acceptance: all19 stages PASS; twelve native RX5700XT strict Vulkan1.3 selections and twelve cleanups with zero validation errors/warnings; all complete geometry/UV/RGBA comparisons and twelve images exact; package source/build match above; return entire result.json/log/image folder. Missing prerequisite SKIPPED is not acceptance. Stop here for native evidence; no next renderer integration phase started.
