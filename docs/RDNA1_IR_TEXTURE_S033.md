# S-033 — IR-generated textured indexed draw

Input `435f35ce89d888ba9e4acb67bcc85a3f5cea66ab`, branch `feature/rdna1-mesh-fallback`, clean checkout, pinned ancestry and all fourteen matching recursive submodules verified. Selected P3.6 subtask: combine generated geometry and UV output with the existing isolated textured draw. No production guest decoder or renderer call sites changed.

New `--ir-texture` and `--ir-texture-vulkan12` modes prevalidate the actual generated `uv` and `uv_zero` modules. CMake now emits both API variants through `--emit-uv`; two API-specific CTest cases run them. Strict Vulkan 1.3 never substitutes the auxiliary 1.2 module. Missing modules fail before device discovery.

Generated mode adds a 544-byte mapped storage/vertex buffer at descriptor binding 2 alongside the existing 1840-byte geometry buffer. The draw helper borrows that buffer; ownership remains with the parent runtime, which destroys it after draw resources and descriptors and before the device. Texture setup resolves cleanup functions before adopting the borrowed handle. In borrowed mode it never creates or dispatches the authored UV pipeline. Texture upload, sampler setup and UV compute-to-vertex barrier are retained. Existing authored texture modes keep their original generator and boundary tests.

Each generated module runs a clear-only control and then a freshly initialized compute-to-indexed-draw submission. There is no preliminary CPU readback supplying geometry or UVs. The output buffers start with Pending status, guards and unused patterns; the unused host geometry payload is poisoned. One IR compute dispatch writes both geometry and UV, followed by compute-to-vertex/index and UV barriers, conventional sampled drawing, image readback dependencies and a fence. The test checks all 460 geometry words, 136 UV words and 4096 RGBA pixels. The zero module must publish zero counts/Ok, preserve unused slots and render a clear image. Fixture allocation is statically known (0 or 3 vertices), not GPU-generated indirect draw count. Noncoherent flush/invalidate handling is retained; actual selected memory was coherent.

| Category | Actual result |
| --- | --- |
| Compilation | PASS: two focused Release builds, 2 jobs, no PGO/IPO; existing dependency warnings retained |
| Unit tests | PASS: existing IR/UV positive, rejection, ABI/mutation and host/oracle regressions |
| Shader compilation / SPIR-V | PASS: generated UV/geometry modules assembled and validated for strict1.3 and separately auxiliary1.2; existing authored draw/sample stages validated |
| Vulkan integration | PASS on Intel Iris Xe API 1.2.172, driver raw `0x0019255D`, loader 1.4.313, core/synchronization validation; generated UV and zero textured draws exact with zero errors/warnings through both cleanups. Strict1.3 runtime SKIPPED locally |
| RDNA1 hardware | New generated draw/textured UV milestone NOT RUN; prior native RX evidence retained |
| Astro Bot compatibility | NOT RUN |

Build: `cmake --build _Build/windows-core --target mesh_compute_proto_tests --parallel 2` in the VS x64 developer environment. Final serial verbose CTest regex `^(mesh_ir_|mesh_output_contract|mesh_proto_|mesh_compute_proto|mesh_draw_|mesh_offscreen_draw|mesh_texture_|mesh_textured_draw)`: **19 PASS / 7 SKIPPED / 0 FAIL**, exit 0, 3.39 s. Initial full run 19/7/0 (3.60 s) retained. All seven skips are strict1.3 runtime cases. Direct `--ir-texture-vulkan12 --image-output <local path>` saves separate UV and zero images and reports zero errors/warnings through each cleanup. Validation layer discovery uses a process-local explicit path, no global configuration changes.

Independent RGB checks of every PPM pixel match integer triangle edge and nearest/clamp texture expectations. Nonzero image: red/green/blue/yellow/black counts 64/156/156/120/3600, SHA-256 `E613B75B2BBA9047B64F21CB35E919E90376872D13E1105BE811F6E504ECA424`. Zero image: 4096 black, SHA-256 `3CF730E8F850835D0144959C030B85528727CE709853299DE2252625C91C78ED`. PPM stores RGB; alpha is checked by runtime RGBA comparisons.

Four isolated copied-harness probes all produce intended FAIL/exit1:

- Valid wrong UV value module fails the full UV comparison. An authored UV overwrite would hide this error; the failure verifies generated data reaches the oracle.
- Valid module with missing UV success store fails the full UV comparison, including final status.
- Valid white fragment module passes complete geometry/UV checks but fails 496 image pixels.
- Missing strict zero-UV module fails before device discovery, without skip or fallback.

The three submitted negative probes report zero validation errors including cleanup. These are successful negative-check gates, not failures of the normal 26-case suite. Normal build artifacts are untouched by probes. Full dirty snapshots and source/hash/cache/submodule records are in ignored `_Build/p0-s012/p3-s033-pre-build-01/` and `-02/`. Build/test logs/XML, direct runtime log, original PPMs, independent image verification, copied probe artifacts/logs/results and module/executable hashes are under `_Build/p3-s033/`.

P3.6 local gate DONE. No local blocker, new RX strict evidence pending under D-028. No new transfer package or emulator/game success claim. Device-local-only memory, noncoherent execution, real guest NGG/resource semantics and performance remain open. No GPU timestamp/utilization/VRAM measurement is claimed.

Next single task: prepare one clean committed-source package covering the accumulated generated draw and textured-UV milestone. Extend package/runner inventories to all 26 relevant cases, all generated modules and the UV oracle source dependency; record per-mode images/buffers and require native RX identity for every selected strict device. Gate clean provenance, manifest/SHA-256/import/license audit, Unicode relocation, independent full images/blobs and expected integrity/fail/skip/RX-guard outcomes. Then obtain the consolidated native RX run; do not treat Intel auxiliary as that evidence. No next task started or push.
