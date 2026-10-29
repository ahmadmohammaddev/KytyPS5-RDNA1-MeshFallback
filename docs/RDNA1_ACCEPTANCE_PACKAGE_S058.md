# S-058 — Clean portable acceptance package

Input4f633f3d clean branch/14 matching submodules. Selected task: consolidate
S-057 acceptance for native handoff; no renderer/game work. Runner now requires
both paired float CSVs as well as PPMs. Source committed before clean build.

Acceptance:43 focused cases; clean manifest/checksums/license/import provenance;
Unicode relocation, integrity/legacy/API/RX guard and wrong-module checks;
Intel auxiliary19 stages/12 selections with zero validation through cleanup,
12 PPMs and2 full float CSVs. Native RX and Astro Bot NOT RUN this session.
Final actual build/package verification and exact artifact identity appended below.

## Final actual results and stop

Clean source `9081c154137440411ba516e38fab984b4b6fd63e`, build `20261029T132623339Z`.
ZIP2,794,884 bytes, SHA-256 `3A0089400C4BAD3367E4A64CA025501C13D4ED9AA8C961E0A9815A006F9238AA`, matching sidecar. Artifact under ignored `_Build/mesh-prototype-packages/`; provenance/toolchain/cache/import/license/test logs in corresponding -evidence directory. Clean2-job package build PASS, no memory retry/PGO/IPO.43 focused CTests32 PASS/11 strict1.3 SKIPPED/0 FAIL(5.58s).

Relocated Unicode path/unrelated CWD verification:20 expected integrity/legacy/API/device/launcher/diagnostic outcomes PASS. Normal combined Intel auxiliary19 stages PASS,12 native Intel selections/cleanups zero validation errors/warnings,12 PPMs independently byte/hash-identical to references plus2 complete ordered4096-row CSVs, independently checked every RGBA value within2^-20. Diagnostic runner20 stages/16 artifacts PASS. Two additional packaged-executable valid-wrong-bridge probes correctly FAIL independently under UNORM and float gates with zero validation errors/warnings after failure cleanup (22 expected verification outcomes total).44 SPIR-V modules byte-identical to S-053; no hidden shader rewrite. SHA-256 sidecar checked. Evidence `_Build/p3-s058/`.

Six categories: Compilation scoped PASS; Unit tests PASS; Shader compilation/SPIR-V PASS/44 unchanged; Vulkan integration Intel1.2 auxiliary PASS/strict1.3 SKIPPED locally; new RDNA1 acceptance NOT RUN; Astro Bot NOT RUN. No local limitation beyond missing strict1.3/RX hardware; native gate requires independent execution. Old native nearest-only failures retained unchanged.

Next single hardware task: transfer this ZIP+sidecar, extract fresh on RX, run `Run-Combined-Milestone.cmd -IsolateImplicitLayers`. Require19 stages PASS,12 independently identified native RX strict1.3 selections and validation-clean cleanup summaries; full geometry/UV buffers, exact coverage/clear/zero, permitted per-channel UNORM sets and bounded numerical float triangle/zero. Return complete result folder with19 logs,result.json,12 PPMs and paired-frontend_uv.csv/paired-frontend_uv_zero.csv. Each CSV must contain4096 ordered pixels. Nearest-different58 may be reported without failure when permitted-set-invalid0 and float gate passes; no blanket tolerance. Any actual failed gate remains FAIL, unavailable prerequisites SKIPPED, unexecuted cases NOT RUN. Stop for RX; no renderer wiring/game work/push.
