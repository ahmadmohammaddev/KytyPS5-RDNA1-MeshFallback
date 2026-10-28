# S-031 — IR-generated geometry to indexed drawing

Input `50d7b75a21954c4fafb320b7ef75fdfc4d539741`, branch `feature/rdna1-mesh-fallback`, clean input, pinned ancestry and all fourteen matching recursive submodules verified. Selected subtask: connect the existing restricted repository-IR-generated compute modules to conventional offscreen drawing. Production renderer and guest decoder were not changed.

`--ir-draw` and `--ir-draw-vulkan12` prevalidate the generated triangle/permuted/zero modules and authored vertex/fragment stages. Each variant creates a compute pipeline from its actual generated module and uses it for a fresh dispatch immediately followed by indexed rendering in the same submission. The existing compute-write to vertex/index-read buffer barrier is retained, as are image-transfer/host dependencies and fenced readback. Output is reinitialized before this submission, and the unused host payload is filled with `0xA5` so authored payload-copy behavior cannot supply the expected geometry. Every guarded output word and all 4096 RGBA pixels are compared independently of the compiler. Fixed fixture allocations determine index count; this is not dynamic GPU-generated indirect drawing. Zero allocation dispatches and issues a zero-index draw, expected to produce a clear image.

Two CTest registrations retain strict 1.3 and explicit auxiliary 1.2 identities. Optional output base names produce separate triangle/permuted/zero images; native path operations preserve Unicode. Existing plain/textured/IR/arithmetic modes pass their regressions. The package runner and package test selection are not extended in this subtask; there is no new transfer package.

| Category | Actual result |
| --- | --- |
| Compilation | PASS, two focused Release builds, 2 jobs, no PGO/IPO; existing dependency warnings retained |
| Unit tests | PASS, focused host/oracle/IR rejection and mutation regressions |
| Shader compilation / SPIR-V | PASS, generated module assembly and pinned SPIR-V validation; draw stages validated, no shader-source edits |
| Vulkan integration | PASS on Intel Iris Xe API 1.2.172, driver raw `0x0019255D`, loader 1.4.313, process-local Khronos layer path; three generated draws, clear controls, full blobs and images exact, zero validation errors/warnings after each cleanup |
| RDNA1 hardware | New generated-IR draw NOT RUN; prior native RX evidence unchanged |
| Astro Bot compatibility | NOT RUN |

Commands: `cmake --build _Build/windows-core --target mesh_compute_proto_tests --parallel 2` in the VS x64 developer environment; final serial CTest regex `^(mesh_ir_|mesh_output_contract|mesh_proto_|mesh_compute_proto|mesh_draw_|mesh_offscreen_draw|mesh_texture_|mesh_textured_draw)`. Final result **16 PASS / 6 SKIPPED / 0 FAIL**, exit 0, 2.92 seconds. Six strict 1.3 runtime cases skip on Intel; auxiliary modes actually dispatch. Earlier full run 16/6/0 (2.97 s) retained before the Unicode path refinement.

Independent direct run `--ir-draw-vulkan12 --image-output <local Unicode path>` passes all three cleanups and saves three images. Independent integer edge checks verify every RGB pixel: triangle/permuted 496 white and 3600 black, zero 4096 black. PPM SHA-256 triangle/permuted `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D`; zero `3CF730E8F850835D0144959C030B85528727CE709853299DE2252625C91C78ED`. PPM contains RGB; runtime checks also verify alpha.

An isolated copied harness with valid SPIR-V fragment constant changed from white to zero validates but correctly FAILS exit 1 on 496 image pixels with zero validation errors through cleanup, after successful full-buffer checking. A separately copied harness with strict zero module withheld correctly FAILS exit 1 before device discovery; no skip or fallback. Neither probe modifies normal build artifacts.

Full dirty binary patches, raw source copies and hashes, index patch, cache and recursive submodule state are retained before both builds under ignored `_Build/p0-s012/p3-s031-pre-build-01/` and `-02/`; build and initial suite logs are beside them. Final logs/XML, images, independent verification JSON and isolated probes are under ignored `_Build/p3-s031/`. No discarded failures. Buffers were coherent, so noncoherent execution remains unexercised. No GPU timestamps, utilization, peak VRAM or performance claim.

P3.4 local generated-IR draw gate is DONE; native RX strict gate is PENDING under D-028 batching. P3 and broader P4/P5 remain open. Next single local task: offline lowering of one bounded per-vertex float2 UV parameter export into the existing separate 544-byte guarded UV contract, with explicit metadata/type/mask/capacity rejection, independent ABI expectations and Vulkan 1.3 SPIR-V validation. Later runtime textured drawing and a consolidated strict RX package remain distinct gates. No next task started and no push.
