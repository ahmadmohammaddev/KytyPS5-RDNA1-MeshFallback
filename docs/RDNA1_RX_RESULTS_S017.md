# S-017 — Native RX 5700 XT P4.1 PASS (2026-10-28)

The second desktop run completes the P4.1 synthetic compute/output-contract acceptance gate. It does not close the broader P4 rendering gate, guest translation, game compatibility or desktop rollback.

## Source and device identity

- Source `d3cc0bce63b32d8106f098a696646eaba0823f3b`, build ID `20261028T183644317Z`, same S-015 portable package as the failed S-016 attempt. Known original ZIP SHA-256 `9F36E3774F9C99B6E73F95D10216F8AE94E65ECD2A80E8216422360908F2A897`; no independent desktop ZIP hash supplied in these five logs.
- Native AMD Radeon RX 5700 XT, DISCRETE_GPU, vendor `0x1002`, device `0x731F`, API 1.4.308, queue family 0; AMD proprietary driver ID 1, raw `0x00800156`, version string `25.6.1 (AMD proprietary shader compiler)`, conformance 1.4.0; loader 1.4.309. ICD DLL path/hash and desktop OS build not supplied.
- Runner mode Strict, requireRx5700Xt=true; executable command `mesh_compute_proto_tests.exe --run`. Strict Vulkan 1.3 module selected; no downgrade to 1.2. Runtime start UTC `2026-10-28T19:25:34.5902273Z`, duration 0.0887366 s, exit 0 / PASS.
- User supplied these logs after the S-016 process-local implicit-layer isolation instruction. Exact shell transcript/override value is not captured by the runner and was not separately supplied, so isolation configuration is contextual rather than independently verified. Steam loader errors are absent; Khronos validation and synchronization validation remain enabled. Logged driver/explicit-layer override variables are unset.

## Actual desktop results

| Category | Result |
| --- | --- |
| Compilation | NOT RUN this attempt; prior clean S-015 package build retained |
| Unit tests | PASS: host-contract exit 0, 0.0123829 s; host-fixtures exit 0, 0.0133206 s |
| Shader compilation and SPIR-V validation | Compilation NOT RUN; packaged module validation + shape PASS, exit 0, 0.0081297 s. Vulkan 1.3/SPIR-V 1.6 (671 words) and Vulkan 1.2/SPIR-V 1.5 (675 words) validated offline |
| Vulkan integration | PASS: actual strict Vulkan 1.3 compute, all nine exact 460-word/1840-byte full-buffer comparisons, zero errors/warnings including cleanup |
| RDNA1 hardware | PASS for this bounded synthetic compute contract on native RX 5700 XT; rendering and wider hardware milestones remain open |
| Astro Bot compatibility | NOT RUN |

| Scenario | GPU dispatch | Expected result / observed PASS |
| --- | --- | --- |
| pending-init-upload | No | Pending=4294967295, counts 0/0 |
| zero-output | Yes | Ok=0, counts 0/0 |
| prototype-triangle | Yes | Ok=0, counts 3/1, exact positions/indices |
| exact-capacity | Yes | Ok=0, counts 64/64 |
| overflow-vertices | Yes | status 1, all other words unchanged |
| overflow-indices | Yes | status 2, all other words unchanged |
| index-overflow-recovery | Yes | same buffer, Ok=0, counts 64/64 |
| overflow-combined | Yes | status 1, all other words unchanged |
| overflow-recovery | Yes | same buffer, Ok=0, counts 64/64 |

The printed large count values in overflow cases (2768241092/2768241093) are intentional nonzero snapshot contents preserved by rejection. They are not valid geometry counts to consume. Full-buffer comparisons include these words, unused regions, guards and status, not just the printed fields. Output/payload allocations are coherent (1840/1792 bytes); noncoherent runtime handling remains unexercised.

Runtime summary: `scenarios=9 failures=0 validation-errors=0 validation-warnings=0` after cleanup. Eight dispatches plus one Pending initialization were executed. No skip in the four runner stages. Vulkan 1.2 runtime was NOT RUN on this desktop; earlier Intel 1.2 auxiliary evidence stays separate. S-016's real failure remains retained.

## Evidence and limits

Five originals copied unchanged into ignored `_Build/p4-s017/rx-input/`; SHA-256 inventory `_Build/p4-s017/input-hashes.json`. JSON/log identity, four exit codes, native device, scenario/dispatch counts and final validation summary checked programmatically. No binaries, private deployment paths or raw logs committed. Documentation input `6bd7ab9221a0e9a7e293cd34873a64a58fff50ef`, branch `feature/rdna1-mesh-fallback`; no new builds/tests executed by Codex, source changes or push.

The scenario log timings are host-observed elapsed times, not GPU timestamp measurements or FPS. Screenshots are unavailable/not meaningful for this buffer-only test; GPU utilization, peak VRAM and device timing were not collected. Successful compute submission/readback establishes native execution but not a separately measured utilization trace. Standalone deployment now executes successfully; rollback/general emulator packaging remains unverified. No triangle image, mesh-shader emulation, guest shader translation or Astro Bot run is claimed.

## Next single task (recommended, not started)

P4.2: extend the standalone synthetic prototype to consume compute-generated positions/indices with ordinary vertex/fragment stages and draw one triangle to an offscreen image. Keep emulator/native rendering sources unchanged. Add explicit compute-to-vertex/index visibility and image-to-host dependencies; deterministically compare readback against documented coverage/color expectations and preserve contract guards. Validate public shaders offline and run with core/synchronization validation. Gate: clean identified portable package, actual native RX 5700 XT/Vulkan 1.3 indexed draw, expected offscreen pixels and buffers, zero validation errors including cleanup, retained image/log/device/package evidence. Intel results remain separate. This bounded untextured first draw does not by itself complete the broader textured-mesh P4 gate. No guest translation or game testing yet.
