# S-055 — Paired color precision diagnosis and capture

## Findings and limits

Independent exact rational analysis reproduces all58 RX RGB differences from S-054 (22 red/blue,34 green,2 all RGB, all -1). Every observed channel is within the floor/ceil bracket of its ideal clamped value. This is descriptive, not proof of a correct shader or driver: actual interpolated RX float values have not been captured. Nearest rounding, floor rounding and simple half-float conversion models do not by themselves establish the cause.

The [Vulkan fixed-point conversion specification](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedconv) permits either of the two closest integers, recommends nearest, and requires exact integer endpoints. Therefore S-052's nearest-only oracle is stronger than the portable UNORM conversion guarantee. This observation does not justify accepting arbitrary +/-1 errors or closing the RX gate; interpolation and the actual pre-conversion output must be distinguished. Original RGBA8 checker remains unchanged and S-054 remains FAIL.

## Changes

- Explicit --paired-float-diagnostic / --paired-float-diagnostic-vulkan12 modes reuse the same generated frontend geometry/UV, bridge and native production pixel. Only color attachment/readback changes to RGBA32F. Required format features queried before allocation; mapped allocation/poison length uses full65536-byte image. Same barriers/fences/cache handling/full geometry+UV comparisons retained.
- Every4096-pixel float readback is examined for finite values, channel pairing and exact clear/background; mathematical interpolation error is reported without a numeric acceptance tolerance. Raw unclamped RGBA and rational-reference UVs saved as4096-row CSVs for triangle and zero controls. Diagnostic completion never prints paired color acceptance. Oracle regressions detect NaN/clear/channel corruption and prove unclamped expectations.
- Failure paths now report warning and error counts after cleanup, including unchanged RGBA8 comparison failures.
- Independent Python analysis uses Fraction geometry, validates capture dimensions/coordinates, reports ideal nearest differences, ideal floor/ceil bracket and optional pre-conversion float correlation. Separate-format draws may interpolate differently; correlation alone is not proof.
- Strict required-RX Run-Paired-Diagnostic.cmd runs original combined tests then float capture, even if original paired stage fails. Preserves original FAIL in result.json.20 stages/14 selections and cleanups/12 PPM+2 CSV expected on full success. Legacy combined launcher retains its19-stage contract. Builder includes43 focused tests and diagnostic sources/launcher.

## Actual local validation

Complete dirty snapshots p3-s055-pre-build-01/-02 retained under `_Build/p0-s012/`; logs, binaries identity, analyses and captures under `_Build/p3-s055/`. Two-job first focused build PASS (existing upstream warnings); final incremental/package verification recorded below. No PGO/IPO or memory retry. Initial CTest invocation failed to locate ctest on PowerShell PATH; configured CMAKE_CTEST_COMMAND used successfully, not a test failure.

43 focused serial tests:32 PASS/11 strict1.3 SKIPPED/0 FAIL,6.57s. New Intel Vulkan1.2 actual RGBA32F triangle and zero draws complete; zero errors/warnings through both cleanups, max ideal error1.1731707849627071e-7, clear/zero exact. Independent float-to-nearest correlation matches all Intel RGB pixels; no RX conclusion.44 modules byte-identical to S-053. Valid swapped-bridge negative still FAILS488 pixels, zero errors/warnings including explicit failure cleanup summary. Offline analyzer rejects truncated image and out-of-bracket corruption. RX analysis reproduces58 differences; no tolerance change.

Six categories: Compilation PASS scoped; Unit tests PASS; Shader compilation/SPIR-V PASS and44-module byte compatibility; Vulkan integration PASS Intel1.2 auxiliary diagnostic and retained regressions, strict1.3 SKIPPED locally; RDNA1 new diagnostic NOT RUN (S-054 combined remains FAIL); Astro Bot NOT RUN.

## Acceptance and next single task

Transfer one clean verified diagnostic package and run `Run-Paired-Diagnostic.cmd -IsolateImplicitLayers` on RX. Retain all logs/JSON/PPMs and both4096-row float CSVs. Require independently identified RX/API/driver, exact full compute buffers, clear/zero controls, and zero errors/warnings through cleanup. A repeated original paired FAIL is evidence, not permission to stop before collecting the float stage; capture success never closes the color gate. Analyze RX pre-conversion values against the independent ideal and UNORM rules, then define/test a justified correction or precision contract with negative coverage. No production renderer integration or push.

### Final clean package verification / stop for RX

Clean source `0ebb417077d53534f0a38091cf0081d3ffd0449f`, build `20261029T130742949Z`. ZIP2,789,604 bytes, SHA-256 `0D1419F8DEC9876F1D35F49BB7FBB61A7E59C173F61A6314754130D18C1F49D5`, matching sidecar under ignored `_Build/mesh-prototype-packages/`. Source commit followed by clean2-job package build PASS,43 serial cases32 PASS/11 strict SKIPPED/0 FAIL(5.73s). Package/import/license/provenance audits PASS. Initial clean-source rejection caused by generated Python __pycache__ retained in package-initial-dirty-cache.log; cache moved into ignored evidence, no source discarded. Initial bare CTest command failure and all build attempts retained.

Unicode/unrelated-CWD relocation:20 expected integrity/API/RX-guard/legacy/launcher/diagnostic outcomes verified. Actual auxiliary20 stages PASS,14 device selections/zero-error+warning cleanups,12 PPMs exact against independent references plus2 complete4096-row finite coordinate-ordered float CSVs.44 shader modules byte-identical to S-053. An explicitly mutated local package fixture adds a21st outcome: valid wrong bridge produces original pairedFAIL while float stagePASS saves both CSVs, overall remainsFAIL. Fixture is not the distributed artifact and does not establish source provenance for a hardware run. Logs `_Build/p3-s055/verification/`, diagnostic-negative-results/ and package -evidence retained. Final diagnostic package SHA checked against sidecar; no new RX/game evidence or push.

STOP: next only native RX `Run-Paired-Diagnostic.cmd -IsolateImplicitLayers` from a fresh extraction. Return complete folder, especially paired-float-frontend_uv.csv and paired-float-frontend_uv_zero.csv. If original paired fails first case,13 actual selections rather than14 are possible; float stage must still complete both captures, prior failures remainFAIL, zero case in original stage remainsNOT RUN. Capture completion is not paired color acceptance. Do not change renderer or declare numeric cause solved before examining native pre-conversion evidence.
