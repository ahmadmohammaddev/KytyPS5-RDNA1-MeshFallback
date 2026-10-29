# S-035 — Native RX consolidated generated draw acceptance

Review only of nineteen supplied files: thirteen logs, result.json and five PPMs. Input checkout `175b737539d5a8994c049408e0db858ed00fb35e`, clean branch `feature/rdna1-mesh-fallback`, pinned ancestry and all fourteen matching recursive submodules verified. Originals preserved; byte-identical copies, SHA-256 inventory, independent verification script/JSON and a texture preview are retained under ignored `_Build/p3-s035/`. No local build/dispatch, driver/system change or push.

Run UTC 2026-10-29T07:28:23Z, package source `70a80620c583037e1ed4fe372bdb89586dbd895a`, build `20261029T070921174Z`. Identity matches the verified S-034 package. The original desktop ZIP or independent desktop ZIP hash was not supplied; successful runner output follows its package-integrity gate, but this review does not independently rehash the desktop executable/modules. Normal-user launcher options recorded: Strict, IrMilestone, required RX and process-local implicit-layer isolation. Driver/ICD override environment variables are unset.

All **13 reported stages PASS**, exit0, no FAIL or SKIPPED stage. Three untextured cases (triangle, permuted, zero) and two generated-UV textured cases (UV, UV-zero) independently select AMD Radeon RX 5700 XT, discrete GPU, vendor/device `0x1002/0x731F`, API1.4.308, AMD proprietary driver25.6.1 (raw `0x00800156`), loader1.4.309, queue family0. All five dispatch-module selections are strict Vulkan1.3; no auxiliary downgrade. Core/synchronization validation and debug messenger recorded for each instance. Every cleanup reports zero errors and warnings.

Runtime reports complete guarded geometry buffers (460 words) and generated UV buffers (136 words) exact. Five clear controls and all five output images have zero RGBA mismatches. Four UV full-buffer comparisons pass (clear and generated draw for each textured variant). The five files independently match every RGB pixel and their JSON/S-034 reference hashes:

| Image | Independent RGB outcome | SHA-256 |
| --- | --- | --- |
| ir-draw-triangle.ppm | 496 white, 3600 black; zero mismatches | `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D` |
| ir-draw-permuted.ppm | Same exact triangle image | `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D` |
| ir-draw-zero.ppm | 4096 black; zero mismatches | `3CF730E8F850835D0144959C030B85528727CE709853299DE2252625C91C78ED` |
| ir-texture-uv.ppm | Red/green/blue/yellow/black 64/156/156/120/3600; zero mismatches | `E613B75B2BBA9047B64F21CB35E919E90376872D13E1105BE811F6E504ECA424` |
| ir-texture-uv_zero.ppm | 4096 black; zero mismatches | `3CF730E8F850835D0144959C030B85528727CE709853299DE2252625C91C78ED` |

Independent verification covers all 20,480 supplied RGB pixels; texture preview visually inspected. PPM lacks alpha, so alpha evidence comes from runtime RGBA checks. Raw geometry/UV readback bytes were not supplied; exact full-buffer evidence is the runtime checker, not an independent binary-buffer recheck.

| Category | Actual result |
| --- | --- |
| Compilation | NOT RUN on desktop/review; clean S-034 build retained |
| Unit tests | PASS: supplied host/IR/UV/oracle preflights, including explicit unsupported rejection and offline mutations |
| Shader compilation / SPIR-V | IR assembly/validation PASS in positive tests and packaged module validation PASS for strict1.3/auxiliary1.2; GLSL source compilation NOT RUN on desktop |
| Vulkan integration | PASS for five generated strict cases, clear controls, complete guarded blobs and exact images; desktop Vulkan1.2 runtime NOT RUN |
| RDNA1 hardware | PASS for bounded P3.4/P3.6 generated untextured and textured milestone |
| Astro Bot compatibility | NOT RUN |

Host elapsed runtime stages: untextured0.4559741 s, textured0.177837 s. These are not GPU timestamps or FPS. GPU utilization/peak VRAM/timestamps, OS build, ICD DLL path/hash, rollback and noncoherent execution were not captured. All mapped outputs report coherent memory. Prior failed experiments, Intel-only results and deferred timer failure remain intact. No new local CTest execution or full emulator regression claim.

Restricted P3.4 and P3.6 native gates are DONE. Broader P3/P4/P5 and game compatibility remain open: current inputs are authored repository IR fixtures, not decoded guest mesh shaders. Source inspection confirms the real frontend injects workgroup and MeshDrawParameter/NGG prologue data (`frontend/translate/Translate.cpp`), and `ShaderRecompiler.cpp` runs SSA/constant/control-flow/identity/dead-code passes after `Frontend::TranslateProgram`. Those semantics are outside the current research subset.

Next single task P3.7: add one offline compatibility fixture through the real mesh frontend and its normal prologue/IR preparation, using public authored guest instructions. Supply the complete resulting IR to the research lowerer without manually constructing/replacing/stripping blocks to conceal unsupported operations. Acceptance: successful frontend translation and a source-backed inventory of required operations; either independently validated compute output for the supported result, or deterministic empty-output rejection naming the first real unsupported semantic with a regression and a bounded next fix. This is a feasibility/bridge test, not production renderer integration. Native defaults remain unchanged; related local development is batched under D-028. No next task implemented in S-035.
