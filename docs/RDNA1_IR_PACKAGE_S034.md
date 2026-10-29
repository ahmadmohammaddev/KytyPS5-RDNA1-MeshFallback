# S-034 — Consolidated generated draw and texture package

Input `0594e60b67734d2ec07cfc4c750cc705707b8902`, clean `feature/rdna1-mesh-fallback`, pinned ancestry/all 14 submodules verified. Selected subtask consolidates S-031 through S-033 into one hardware handoff under D-028.

New `-IrMilestone` mode / `Run-IR-Milestone.cmd` combines three generated untextured cases and two generated-UV textured cases. Host/IR/UV/draw/texture preflights precede both runtime stages; after successful preflight both stages are attempted, even if one skips/fails. FAIL takes precedence over SKIPPED. JSON retains actual runtime status separately from target acceptance and per-stage device evidence, plus five image names/hashes. Every successful stage requires the expected device/API/zero-error-and-warning cleanup count, complete images and, when requested, RX identity for all selections. Existing launchers/modes retained.

Builder includes all 26 scoped tests, all 14 generated API-specific modules, UV oracle source excerpt and launcher. Source/build notices and import audit retained; no driver/layer/runtime installation or proprietary data bundled. Source commit is required before clean packaging. Windows PowerShell 5.1 parse PASS; clean build/test/package/relocation outcomes and exact ZIP identity follow below. No dirty compile build required for these script-only edits.

## Final package and verification

Source `70a80620c583037e1ed4fe372bdb89586dbd895a`, build `20261029T070921174Z`. ZIP `Kyty-MeshPrototype-Windows-x64-70a80620c583037e1ed4fe372bdb89586dbd895a-20261029T070921174Z.zip`, 1,555,283 bytes, SHA-256 `3D78510DF6E474855ACBF89B39AEBF9329CEFB2C1BABE31898FA29A5D0DFBBAD`, matching `.zip.sha256`. 57 payload files plus manifest/checksum inventories, source/license notices and audited imports retained. Final documentation commit does not change package source identity.

Clean two-job build/package PASS, no PGO/IPO, compiler/dependency warnings retained. Clean serial suite: **19 PASS / 7 SKIPPED / 0 FAIL**, exit 0, 3.21 seconds; all skips are strict Vulkan 1.3 runtime on the Intel API 1.2.172 device. Actual generated draw/UV artifacts validate both strict1.3 and auxiliary1.2. No drivers, validation layers or Microsoft runtime components bundled.

Fresh extraction to a Unicode/space directory, Windows PowerShell 5.1 subprocesses, and invocation from unrelated CWD verify **15 intended outcomes**: manifest/inventory PASS; offline eleven preflights PASS with runtime NOT RUN; auxiliary milestone thirteen stages PASS, five selected Intel devices, all full blobs/images exact, zero errors/warnings after all cleanups; strict milestone both stages SKIPPED41 with no downgrade; required-RX auxiliary rejects Intel for both stages while recording actual runtime PASS and target FAIL; all five old compute/draw/texture/IR/arithmetic modes PASS; conflicting modes reject; missing module fails package verification; direct missing strict UV module FAILS before device discovery/no fallback; checksum tampering rejects; actual CMD launcher preserves strict/RX/isolation options and SKIPS41 on Intel. Expected FAIL/SKIP probes are successful verification gates, not normal-suite failures.

Independent RGB checks verify all 20,480 pixels of the five PPMs and each JSON hash. Plain triangle/permuted hashes `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D` (496 white/3600 black each); textured hash `E613B75B2BBA9047B64F21CB35E919E90376872D13E1105BE811F6E504ECA424` (64/156/156/120 red/green/blue/yellow plus 3600 black); both zero hashes `3CF730E8F850835D0144959C030B85528727CE709853299DE2252625C91C78ED` (4096 black each). Runtime also checks alpha and complete geometry/UV buffers. Intel driver raw `0x0019255D`, loader1.4.313; process-local explicit validation path and implicit isolation captured. No GPU timestamps/utilization/VRAM performance claim; coherent memory only.

Initial local verification controller passed all fifteen checks but incorrectly propagated the intentionally skipped launcher exit41. Corrected controller exits0 and reran all fifteen in a fresh directory; both logs/results retained. This did not alter package contents. Evidence: ignored `_Build/p3-s034/` parser/build/verification logs, original and retry extracts/results, verification scripts/JSON and independent image checks; complete clean toolchain/cache/build/import/test provenance in the package evidence directory. No dirty compile or untracked-source build in this script-only session; package compiled only after clean source commit.

| Category | Actual result |
| --- | --- |
| Compilation | PASS, clean focused package targets, 2 jobs |
| Unit tests | PASS, scoped host/IR/UV/oracle/mutation regressions and package checks |
| Shader compilation / SPIR-V | PASS for assembled generated modules and offline stage validation; compiler/version recorded |
| Vulkan integration | PASS on Intel Vulkan1.2 auxiliary including five generated draws/texture cases and old modes; strict1.3 SKIPPED locally |
| RDNA1 hardware | New generated draw/textured milestone NOT RUN; prior RX evidence retained |
| Astro Bot compatibility | NOT RUN |

Next single task: transfer this ZIP and sidecar, verify/extract fresh on RX and run ordinary-user `Run-IR-Milestone.cmd -IsolateImplicitLayers`. Acceptance: package identity matches; all thirteen stages PASS; three plain and two textured generated cases select native RX 5700 XT on strict Vulkan1.3; full geometry/UV/pixels exact and all five cleanup summaries zero errors/warnings; result.json and both runtime logs contain complete per-stage identity and five image hashes. Return the whole results folder (13 logs, result.json, 5 PPMs; empty isolation directory may also be present). Retain skips/failures; no game evidence inferred. No next phase or push.
