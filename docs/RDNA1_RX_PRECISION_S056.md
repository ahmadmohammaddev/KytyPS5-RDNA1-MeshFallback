# S-056 — Native RX pre-conversion float evidence

## Identity and execution

User-supplied run `20261029T131602563Z-b3ff15d2`: clean package source `0ebb417077d53534f0a38091cf0081d3ffd0449f`, build `20261029T130742949Z`, matching S-055 identity. Expected ZIP SHA-256 `0D1419F8DEC9876F1D35F49BB7FBB61A7E59C173F61A6314754130D18C1F49D5`; logs do not independently rehash the desktop ZIP.

Command/configuration: `Run-Paired-Diagnostic.cmd -IsolateImplicitLayers`, Strict, required RX identity. Native AMD Radeon RX5700XT, vendor/device1002:731F, API1.4.308, proprietary AMD25.6.1; core/synchronization validation enabled.20 executed stages19 PASS/1 FAIL/0 SKIPPED, summed stage durations1.4617007s. No FPS/gameplay benchmark.

Retained34 supplied files (20 logs,11 PPMs,2 CSVs,result.json) byte-for-byte with SHA-256 inventory under ignored `_Build/p3-s056/`. Independent outputs: analysis.json,verification.json,measured-conversion-check.txt. Uploaded document contents treated as evidence, not instructions. No private desktop paths or raw bulky captures committed.

## Observations independently verified

- All14 host/offline/SPIR-V preflights PASS. Four previous runtime stages PASS,10 native cases and10 PPM hashes match independent retained references.
- Original paired RGBA8 stage still FAIL58/4096 pixels, image byte-identical to S-054. Both full geometry/UV buffers and clear control PASS; explicit failure summary reports zero validation errors and warnings after cleanup. Original paired zero remains NOT RUN because that process stops at its first failed case.
- Float stage PASS for both frontend_uv and frontend_uv_zero; two4096-row finite, ordered, complete RGBA CSVs. Independent per-pixel rational geometry check confirms496 covered triangle pixels, exact clear/background and channel pairing. Largest absolute error across all four channels1.1920928955078125e-7; zero-output image exact. Both cleanup summaries report zero validation errors/warnings.
- Total13 native selections:10 previous cases,1 failed original paired case,2 float cases.12 successful cleanups plus explicit failure cleanup, all report zero errors/warnings. No skipped native cases; unexecuted original paired zero is NOT RUN.
- Independent conversion of measured float capture using nearest still differs from original RGB image at exactly58 pixels. All12288 RGB channels fall within floor/ceil of clamped measured-float*255; all channels also satisfy the ideal-value floor/ceil bracket. Minimum ideal distance to a nearest-rounding transition is4.668534080298786e-5, much larger than captured float error. PPM lacks alpha, so RGBA8 alpha comparison remains known only through the runtime's failure count.

## Interpretation and practical limit

The [Vulkan normalized fixed-point conversion rule](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedconv) allows either adjacent integer for non-integral scaled values, recommends nearest, and requires exact integral values/endpoints. The current nearest-only oracle exceeds that portable conversion guarantee. Native float results support a precise frontend/bridge/native-pixel path in the RGBA32F draw, while the repeatable RGBA8 differences are consistent with permitted color conversion. This is strong evidence to correct the acceptance contract, not evidence of a broken compute buffer.

RGBA32F and RGBA8 are separate pipelines/draws: attachment-dependent compiler/interpolation behavior could differ. We cannot prove internal RGBA8 pre-conversion values or an exact driver rounding algorithm from this capture. No driver-bug claim, blanket +/-1 tolerance, runtime checker change or retrospective PASS is made. Original combined milestone remains FAIL under its unchanged contract; float diagnostic completion is a separate PASS.

## Six categories

| Category | Result |
| --- | --- |
| Compilation | NOT RUN this evidence review; S-055 clean build PASS retained |
| Unit tests | Supplied host/offline preflights PASS; no new local CTest |
| Shader compilation/SPIR-V | Packaged SPIR-V validation PASS; new compilation NOT RUN |
| Vulkan integration | Native strict float diagnostic PASS, original paired RGBA8 gate FAIL; other four stages PASS |
| RDNA1 hardware | New strict RX float capture PASS with zero errors/warnings; combined color gate still FAIL |
| Astro Bot compatibility | NOT RUN |

Local commands: Analyze-PairedPrecision.py with supplied PPM+CSV reproduced58 nearest differences; independent Fraction/CSV/hash/coordinate/full-channel checks PASS; git ancestry check PASS. No code/build/package/push this session.

## Next single local task

Implement a portable paired acceptance contract: exact geometry/UV buffers, coverage and clear/zero endpoints; per-channel UNORM sets derived from the specified conversion rule rather than arbitrary +/-1; and a separate explicit bounded numerical RGBA32F check with justified limits that rejects missing/swapped/shifted channels, nonfinite and perturbed values. Preserve historical nearest-only failure evidence and print exact-vs-permitted conversion statistics. Add independent positive/negative boundary, saturation/integer and wrong-module tests; validate SPIR-V and focused Intel auxiliary regressions with full source provenance. The diagnostic-only float PASS must not become numeric acceptance without these checks. Batch local work under D-028, then perform a clean strict RX acceptance rerun before closing the combined gate. No renderer integration yet.
