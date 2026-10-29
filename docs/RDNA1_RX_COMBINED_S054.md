# S-054 — Native RX combined-stage evidence: FAIL

Reviewed 2026-10-29. Evidence review only; no implementation or new build.

## Identity and execution

Package source `ad57feab7d794b7cedb855a6fb5411ca76881200`, build `20261029T122933634Z`; supplied result identity matches S-053. Expected ZIP SHA-256 `7DC0B74CB00106E63B706B14F15C6311D6C38318A78E652487BF1F98799F6469`; uploaded results alone do not independently rehash the desktop ZIP.

Command/configuration: `Run-Combined-Milestone.cmd -IsolateImplicitLayers`, strict Vulkan 1.3, required RX identity. Device: AMD Radeon RX 5700 XT, vendor/device `1002:731F`, API 1.4.308, AMD proprietary driver 25.6.1. Khronos validation and synchronization validation enabled. Stage durations sum to 1.0691 s; paired stage 0.0994 s. No FPS/gameplay benchmark; correctness harness only.

Raw 19 logs, result.json and 11 PPMs retained byte-for-byte with SHA-256 inventory under ignored `_Build/p3-s054/`. Independent image/hash checks retained in image-verification.json; all differing RGB coordinates/values in pixel-differences.json. No private desktop paths copied into this report.

## Actual result

- 19 executed stages: **18 PASS / 1 FAIL / 0 SKIPPED**. All 14 host/offline/SPIR-V preflights PASS.
- Four prior runtime stages PASS: generated draw (3), generated texture (2), frontend draw (3), frontend texture (2). Ten exact images match independently retained references; their ten cleanup summaries report zero errors/warnings.
- New runtime-paired FAIL, exit 1: first frontend_uv case selects native RX and passes full geometry/UV checks and clear control, but complete RGBA comparison reports **58/4096 mismatching pixels**. Final failure line reports zero validation errors including cleanup. No warning callback messages present; failing path lacks the normal final warning-count summary, so do not substitute a successful cleanup summary.
- Eleven native selections occurred against twelve expected. Paired frontend_uv_zero **NOT RUN** after first-case failure; its twelfth image is absent. Overall milestone remains FAIL.

Independent RGB comparison of paired-frontend_uv.ppm confirms exactly 58 differing pixels: 22 have delta (-1,0,-1), 34 (0,-1,0), and 2 (-1,-1,-1), actual minus reference. All other RGB pixels match. PPM does not retain alpha; complete RGBA mismatch count comes from runtime logging. Small downward color differences motivate investigating interpolation/conversion precision, but neither a driver bug nor an oracle defect is established by these results. Do not silently relax tolerance or replace the expected image with the RX output.

## Six categories

| Category | Result |
| --- | --- |
| Compilation | NOT RUN this review; clean S-053 build PASS retained |
| Unit tests | Desktop host/offline preflights PASS; no new local CTest run |
| Shader compilation / SPIR-V | Packaged module validation PASS; no new shader compilation |
| Vulkan integration | Strict RX combined gate FAIL; four prior stages PASS; Intel auxiliary evidence separate |
| RDNA1 hardware | Combined actual-stage gate FAIL (58 pixels); earlier scoped gates retained |
| Astro Bot compatibility | NOT RUN |

## Next single task and acceptance

Diagnose the 58 native paired color mismatches locally using the retained exact pixel coordinates, actual native fragment/bridge and independent interpolation/UNORM expectations. Establish a source/specification-backed explanation and add focused discriminating controls; preserve the failure evidence and wrong-module detection. If device measurements are necessary, prepare one clean committed diagnostic package with pre-conversion values/control cases and cleanup warning/error summaries, then request RX execution under D-028. Acceptance: explain observed channel values, test the correction or justified precision contract independently, retain full buffer/image coverage and negative regressions; do not close native combined gate until a qualifying strict RX rerun passes. No production renderer wiring or next phase yet.
