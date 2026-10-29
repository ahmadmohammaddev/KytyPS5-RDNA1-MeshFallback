# S-044 actual frontend UV native handoff

Branch `feature/rdna1-mesh-fallback`. Clean package source `a183c1904f11e9c6f6c035ad58432d3456f859e0`, build `20261029T095304950Z`. Final documentation commit does not alter package identity. No push.

## Completed local changes

S-042 authored ISA now exports parameter0.xy via actual decoder/frontend/SSA, declared non-indexed specialization and full linear graph lowerer. Values .25/.25,1.25/.25,.25/1.25 and zero output are checked by the independent SPIR-V136-word guarded UV oracle. Unsupported opt-in/target/index/component mask/compression/valid-mask/capacity/type and undefined allocated UV predicate reject empty output. Negative mutations restore byte-identical outputs; valid wrong-UV SPIR-V is detected. Source mapping verified in frontend Attribute.cpp.

S-043 actual generated frontend geometry/UV feed the isolated sampled draw. No authored UV compute replacement. Both complete geometry1840-byte and UV544-byte buffers, guards/unused/count/status, clear/draw images and cleanup validation checked. Added explicit strict and auxiliary CLI/CTest modes; CMake generates four new API-separated modules. Source/native/default renderer unchanged. Wrong-but-valid UV module FAILS complete136-word comparison with zero validation errors; missing strict module FAILS before device discovery. Both expected failures retained.

S-044 existing frontend milestone package now includes both new textured cases,31 CTests,36 SPIR-V modules and source/license/import inventories. All32 legacy modules byte-identical to prior package.71 payload files plus manifest/checksum inventories. No game data or driver/layer installer bundled.

## Actual checks

| Category | Result |
| --- | --- |
| Compilation | PASS focused2-job dirty builds and clean package build, no memory retry/PGO/IPO |
| Unit tests | PASS focused host/IR/UV/frontend/oracle suite; historical timer remains deferred |
| Shader compilation / SPIR-V | PASS strict1.3 and separately assembled auxiliary1.2 modules |
| Vulkan integration | PASS actual Intel Iris Xe1.2 frontend UV/zero sampled draws and full regressions, exact complete buffers/pixels; zero errors/warnings through cleanup. Strict1.3 SKIPPED (API1.2.172) |
| RDNA1 | New actual frontend UV texture gate NOT RUN; S-041 native geometry gate remains PASS |
| Astro Bot | NOT RUN |

S-042 suite29:21 PASS/8 strict SKIPPED/0 FAIL,4.48s. S-043 suite31:22 PASS/9 strict SKIPPED/0 FAIL,4.42s. Clean S-044 suite31:same counts,4.38s. Complete dirty source snapshots `_Build/p0-s012/p3-s042-pre-build-01/` and `p3-s043-pre-build-01/`; raw build/CTest/dumps/mutation logs under ignored session folders.

Package verification after Unicode/space relocation and unrelated-CWD invocation:15 expected integrity/offline/auxiliary/strict/RX guard/five legacy/conflict/missing integrity/missing direct/checksum tamper/launcher outcomes PASS. Actual relocated auxiliary16 stages PASS, ten device selections/cleanups and ten expected images. Missing prerequisite strict41 is SKIPPED, RX guard on Intel is FAIL, offline runtime is NOT RUN. Ten complete RGB images match independently verified references (40,960 pixels), plus runtime full RGBA comparisons. No performance/noncoherent/production/game claims.

Commands: standard capture.ps1 before each dirty build; developer-shell cmake --build _Build/windows-core --target mesh_ir_frontend_tests mesh_compute_proto_tests --parallel2 (normal CLI uses separate `--parallel 2` arguments); serial verbose focused CTest with XML; isolated copied wrong-UV/missing-module runtime probes. Clean package command: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/mesh-prototype/Build-Package.ps1 -ValidationLayerDirectory _Build/p4-s013/tools/validationlayers -Jobs 2`. Relocation verifier `powershell.exe -NoProfile -ExecutionPolicy Bypass -File _Build/p3-s044/verify.ps1` exit0. Full exact command logs retained under ignored session/package-evidence folders. Windows PowerShell parser and git diff --check PASS.

## Next mandatory native gate

ZIP `Kyty-MeshPrototype-Windows-x64-a183c1904f11e9c6f6c035ad58432d3456f859e0-20261029T095304950Z.zip`,2,600,296 bytes.
SHA256 `6447AE462F7869E2A6CCBDF1AADA661C68288EBF30ADC8989BBF021DA3010F1A`, matching sidecar.

Extract on RX5700XT; normal-user command from package directory:

```cmd
.\Run-Frontend-Milestone.cmd -IsolateImplicitLayers
```

Acceptance: all16 stages PASS, ten native RX5700XT strict Vulkan1.3 selections/cleanups, exact full geometry/UV buffers and ten images, zero errors/warnings including cleanup; package source/build match above. Return complete result.json/log/image folder. A SKIP is not acceptance. Stop for this coherent bounded frontend gate before proceeding toward a production adapter; no new integration phase started.

Remaining gaps: only non-indexed count3/base0/instance0/single workgroup context tested. General indexed/dynamic/CFG/LDS/guest bindings, resource tracking, production scheduling/cache/capability selection and game integration remain open. Further planning must preserve fail-closed behavior and native default path.
