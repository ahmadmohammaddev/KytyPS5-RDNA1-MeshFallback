# S-018 — P4.2 standalone compute-to-indexed-draw prototype

## Scope and behavior

Extend the existing P4.1 executable, preserving compute-only modes and all nine output-contract cases. New opt-in `--draw` (strict Vulkan 1.3) and `--draw-vulkan12` (explicit auxiliary) select a graphics+compute queue and require RGBA8 color/transfer and float4 vertex formats. No emulator/renderer, guest shader, game, timer or feature-spoofing change.

`tests/MeshOffscreenDraw.h` adds a 64x64 single-sample RGBA8 offscreen render pass and conventional authored vertex/fragment shaders. A clear-only GPU control must return opaque black. A fresh one-workgroup compute dispatch writes the existing guarded 1840-byte contract; the SAME submission binds that output directly as a float4 vertex buffer (byte 0) and uint32 index buffer (byte 1032), then draws three indices (2/0/1). There is no intermediate CPU readback. The bounded fixture uses known counts 3/1; GPU-generated indirect draw counts are not implemented.

Screen vertices (16,16), (48,16), (16,47.5) avoid pixel centers on every edge. An integer edge oracle checks every RGBA byte: exactly 496 opaque white pixels, 3600 opaque black. All 460 output words, including guards/unused data/counts/status, are checked after both control/draw submissions. Host oracle regressions reject missing draw, all-white image, interior-channel corruption and background-alpha corruption.

Synchronization: host writes to compute/transfer; explicit compute shader writes to VERTEX_INPUT / VERTEX_ATTRIBUTE_READ + INDEX_READ; explicit image transitions to color attachment and transfer source; color writes to transfer read; transfer/compute writes to host read; fence (10-second gate), full-allocation flush/invalidate when noncoherent. Single queue, no ownership transfers. This follows [Khronos compute-to-graphics synchronization guidance](https://docs.vulkan.org/guide/latest/synchronization_examples.html#compute-to-graphics-dependencies). Cleanup waits idle and releases graphics/image resources before outer device/instance teardown; final error counters include both cleanups. Existing buffer destruction is ordered before freeing bound memory.

`--image-output FILE` saves actual RGB readback as PPM, outside packages via the runner. Wide Windows argv is explicitly converted to UTF-8 so Unicode result paths work. New shaders compile for both Vulkan 1.3/SPIR-V 1.6 and auxiliary 1.2/SPIR-V 1.5 and validate with pinned SPIRV-Tools.

## Initial development verification

Input `8f9859a95c84c17f76d06554d2a6b939ddc42aa3`, branch `feature/rdna1-mesh-fallback`, clean initial checkout, 14 matching recursive submodules and pinned ancestry. Toolchain/flags from S-015 retained: x64 VS 17.14.42, clang-cl/LLD 19.1.5, CMake 3.31.6-msvc6, Ninja 1.12.1, glslang 16.6.0; Release/testing ON/launcher OFF/PGO OFF/use empty/IPO OFF, two jobs, serial tests. No memory exhaustion/one-job retry.

Complete dirty source patches, raw changed/untracked files, SHA-256 manifests, submodules/cache and reverse-patch checks retained BEFORE builds in ignored `_Build/p4-s018/pre-build-01/` and `pre-build-02/`. First build PASS with a u8path deprecation warning; replaced it while fixing Windows Unicode argv handling. Second build PASS without that warning. Logs/scripts retained; no failed build hidden.

Commands: `_Build/p4-s018/build.cmd` refreshes KytyGitVersion and builds `mesh_output_contract_tests mesh_compute_proto_tests --parallel 2`; `_Build/p4-s018/test.cmd` invokes CTest `--output-on-failure --parallel 1 -V --output-junit` with the exact ten names registered in the package builder.

| Category | Actual development result |
| --- | --- |
| Compilation | PASS focused two-job builds; no core/GUI rebuild |
| Unit tests | PASS contract, host fixtures, new pixel-oracle corruption checks |
| Shader compilation and SPIR-V validation | PASS both compute variants and four authored vertex/fragment modules; existing wrong-binding negative checker PASS |
| Vulkan integration | PASS on native Intel Iris Xe/API 1.2.172, auxiliary mode only: original nine cases plus clear/draw, zero pixel/blob mismatches, zero validation errors/warnings after cleanup. Strict 1.3 compute and draw SKIPPED (no qualifying device) |
| RDNA1 hardware | New P4.2 draw NOT RUN; prior P4.1 native success remains separate |
| Astro Bot compatibility | NOT RUN |

Initial serial suite: **8 PASS, 2 SKIPPED, 0 FAIL**, ten cases, CTest exit 0, 1.32 s. Independent JUnit counts checked. No full-suite claim. Four new registrations bring the total to 435; 72 cases now have passing executions (including prior RX compute), one failed-only deferred timer, one skipped-only new strict draw, 361 never run. Historical failures retained.

Independent saved-image inspection: 64x64 PPM, 12,301 bytes, SHA-256 `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D`; 496 white/3600 black, representative interior/background pixels checked. PPM and PNG preview retained locally; visual inspection matches the right triangle. No GPU timestamp/utilization/FPS/peak VRAM metrics collected; this is buffer/image correctness, not performance evidence. All chosen mapped buffers were coherent; noncoherent runtime remains unexercised.

Two extra probes: a copied executable/modules with a valid constant-black auxiliary fragment shader completes all P4.1 buffers but FAILS exit 1 on exactly 496 triangle pixels, validation errors 0; original sources/modules untouched. An empty explicit-layer directory causes auxiliary draw SKIPPED exit 41, not pass. Logs/exit records under `_Build/p4-s018/`.

## Portable handoff work within this subtask

Extend existing clean-source packager to ten cases, all six SPIR-V files, draw header/shader excerpts and `Run-Draw.cmd`. Runner retains compute-only default, adds explicit `-Draw`, saves/hashes `triangle.ppm`, records implicit-layer override variables, and offers optional `-IsolateImplicitLayers` for the previously observed Steam discovery issue. Isolation is process-local, restores environment and leaves explicit core/synchronization validation required. No registry/driver edits or installers. PS scripts parse successfully.

Commit source/checkpoint first, then build the package from clean source. Final clean package/relocation outcomes are appended below before handoff. Do not transfer the development dirty binary. P4.2 stays IN_PROGRESS until a separate native RX strict draw supplies exact pixels/buffers and zero errors including cleanup. No next phase begins here.
