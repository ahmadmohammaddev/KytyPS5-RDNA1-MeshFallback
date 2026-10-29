# S-040 frontend milestone package and native handoff

Branch: `feature/rdna1-mesh-fallback`. Clean package source: `87e912bd9786abd206b635ff7c68a832562f9b8a`; build `20261029T081156341Z`. Documentation commit after packaging does not alter package identity. No push.

Sequential local commits: S-036 actual decoder/frontend probe; S-037 complete validated linear graphs; S-038 bounded non-indexed draw specialization and LocalInvocationIndex; S-039 actual frontend triangle/permuted/zero conventional draw; S-040 consolidated packaging.

## Changes and limits

The public authored ISA passes the real decoder, CFG builder and mesh frontend, SSA and normal pre-resource optimization prefix. Both blocks remain. Specialization substitutes explicit count3/base0/instance0/index-width0/address0/single-group/first-group0 constants, and ordinary propagation/DCE removes inactive index reads. Unknown draw parameters and unsupported contexts reject before any mutation. Native/default production compiler/renderer remain unchanged. This does not implement arbitrary shaders, indexed draws, dynamic groups/counts, guest bindings, LDS or general CFG, and is not an emulator build.

Generated triangle/permuted/zero execute with independent expected full 1840-byte buffers and complete 64x64 images. Valid wrong module rejects, missing strict module fails before discovery. Existing generated geometry/UV milestone remains included. One new required-RX launcher combines 15 stages and eight selections/images/cleanups. Both API module sets are included and individually validated; no strict downgrade.

## Actual results

| Category | Result |
| --- | --- |
| Compilation | PASS: clean focused two-job build; no PGO/IPO; no memory retry |
| Unit tests | PASS: scoped host/IR/UV/frontend/oracle tests; deferred historical timer untouched |
| Shader compilation / SPIR-V | PASS: generated strict1.3 plus separately assembled auxiliary1.2 modules, authored shaders |
| Vulkan integration | PASS Intel Iris Xe1.2 auxiliary: actual frontend and prior generated draw/texture, all blobs/pixels exact; zero errors/warnings through all eight cleanups. Strict1.3 SKIPPED (Intel API1.2.172) |
| RDNA1 hardware | New frontend gate NOT RUN; older native milestones retain their prior independent PASS |
| Astro Bot | NOT RUN |

Clean package CTest: 29 cases, 21 PASS, 8 strict SKIPPED, 0 FAIL, 3.89 s. S-039 dirty focused suite had same counts, 4.00 s. Retain all earlier failures and complete pre-build snapshots; detailed S-037/S-038/S-039 handoffs in living plan.

Relocated archive under space/Unicode path and invoked from unrelated CWD. Fifteen expected outcomes verified: integrity, offline, actual auxiliary milestone, strict skip, Intel RX guard rejection, five previous runtime modes, conflicting modes, missing-file integrity, missing strict direct module, checksum tamper and strict launcher. Offline runtime reports NOT RUN; actual unavailable strict reports SKIPPED; guard/tamper/missing probes FAIL as expected. No validation failure is reclassified PASS. Eight package images independently match either direct frontend integer edge oracle or retained independently verified regression hashes (32,768 pixels).

Commands: developer-shell `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/mesh-prototype/Build-Package.ps1 -ValidationLayerDirectory _Build/p4-s013/tools/validationlayers -Jobs 2`; builder records full configure/version/build/29-case serial CTest/import/integrity commands. `powershell.exe -NoProfile -ExecutionPolicy Bypass -File _Build/p3-s040/verify.ps1` exits0. Windows PowerShell parser checks PASS. Independent image controller exits0.

Raw logs, verification controller/results and images inventory: ignored `_Build/p3-s040/`. Build manifest, submodule/toolchain/cache/commands/CTest/import logs: ignored package `-evidence/` directory. Fourteen recursive submodules clean/matching; source pinned ancestry retained. Package has 67 payload files plus manifest and checksum inventories; no game data/system installers included. External validation layer and native Vulkan driver remain prerequisites.

## Exact next task: native frontend milestone

ZIP: `Kyty-MeshPrototype-Windows-x64-87e912bd9786abd206b635ff7c68a832562f9b8a-20261029T081156341Z.zip`, 2,586,529 bytes.
SHA-256: `336303B17E837E97E5CEFF4EEE458460AECDB5C9AFDE22DB377ECDCB7FD06D89`; matching `.zip.sha256` sidecar.

Extract on RX5700XT machine; as normal user from package folder run:

```cmd
.\Run-Frontend-Milestone.cmd -IsolateImplicitLayers
```

Acceptance: all 15 stages PASS, eight explicitly native RX5700XT strict Vulkan1.3 selections, exact full guarded outputs and eight complete images, zero validation errors/warnings through eight cleanups, source/build identity above, retained result.json/all logs/images. Return the complete result folder. SKIPPED does not close the gate. Stop here; no subsequent integration or game phase started.
