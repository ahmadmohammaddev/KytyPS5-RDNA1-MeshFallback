# S-034 — Consolidated generated draw and texture package

Input `0594e60b67734d2ec07cfc4c750cc705707b8902`, clean `feature/rdna1-mesh-fallback`, pinned ancestry/all 14 submodules verified. Selected subtask consolidates S-031 through S-033 into one hardware handoff under D-028.

New `-IrMilestone` mode / `Run-IR-Milestone.cmd` combines three generated untextured cases and two generated-UV textured cases. Host/IR/UV/draw/texture preflights precede both runtime stages; after successful preflight both stages are attempted, even if one skips/fails. FAIL takes precedence over SKIPPED. JSON retains actual runtime status separately from target acceptance and per-stage device evidence, plus five image names/hashes. Every successful stage requires the expected device/API/zero-error-and-warning cleanup count, complete images and, when requested, RX identity for all selections. Existing launchers/modes retained.

Builder includes all 26 scoped tests, all 14 generated API-specific modules, UV oracle source excerpt and launcher. Source/build notices and import audit retained; no driver/layer/runtime installation or proprietary data bundled. Source commit is required before clean packaging. Windows PowerShell 5.1 parse PASS; clean build/test/package/relocation outcomes and exact ZIP identity follow below. No dirty compile build required for these script-only edits.
