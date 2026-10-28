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

## First clean-package verification (retained)

- Source commit `26c85b13344ef38b3a7c706f006b795551ec17dd`; build ID `20261028T194438576Z`. Package built/verified with clean committed source and all 14 clean matching recursive submodules. Incremental build was up to date; no new compile action was required. Compiled source raw SHA-256s independently match the pre-build-02 snapshot and committed working files. KytyGitVersion refreshed to clean package identity; test fixture does not embed that header.
- ZIP `Kyty-MeshPrototype-Windows-x64-26c85b13344ef38b3a7c706f006b795551ec17dd-20261028T194438576Z.zip`, **708,435 bytes**, SHA-256 `4E11DCD336C198DC98CAD8115369CEB9FCB0CC8AE9409A18570DCAEF650B1295`, matching `.zip.sha256` sidecar, under ignored `_Build/mesh-prototype-packages/`. 24 payload files plus manifest/checksum inventory; Windows/MSVC runtime imports only, no bundled driver/layer/game data. External native driver, v14 x64 runtime and Khronos validation prerequisites unchanged.
- Clean-source CTest: **8 PASS / 2 SKIPPED / 0 FAIL**, exit 0, serial **0.91 s**. Strict compute/draw skip on the Intel API 1.2.172 device, independently of S-017's native RX compute success. Auxiliary draw remains Intel only.
- ZIP independently hashed/extracted into a new path with spaces/Arabic characters; runner invoked from unrelated CWD under Windows PowerShell 5.1. Nine expected probe outcomes: verify PASS; offline draw preflight PASS (runtime NOT RUN); strict required-RX draw SKIPPED 41; auxiliary draw + explicit implicit-layer isolation PASS; auxiliary required-RX check FAIL 1 on Intel; tampered graphics module FAIL 1; missing-module integrity FAIL 1; direct missing graphics module FAIL 1 with no build fallback; legacy compute-only auxiliary PASS. Private probes are not additional registered CTest passes.
- Relocated packaged auxiliary run: six stages PASS, runtime 0.1615748 s, all original nine blobs and clear/triangle image exact, zero validation errors/warnings after cleanup. Image path contains Arabic characters. Actual image SHA-256 matches `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D` and JSON inventory; isolation override and draw mode correctly recorded. No software ICD or permanent environment/registry change.
- A local relocation verification controller initially stopped after its seven successful runner probes because an unquoted Unicode result path was parsed as two PowerShell arguments. Original script/log retained; corrected the local script and completed aggregate checks plus remaining direct/legacy probes without overwriting prior results. This was an evidence-controller error, not a product pass/failure; no source change or replacement package needed.
- Evidence: `_Build/p4-s018/final-package.json`, `relocation-results.json`, `finish-relocation.log`, per-probe logs/results/PPM and original controller; package `*-evidence/` contains clean source/toolchain/submodules/cache/commands/build/imports/CTest XML/logs. The checkpoint update changes only Markdown documentation and does not change the package's earlier source identity.

**Next one task:** copy this ZIP and sidecar to the RX desktop, verify SHA-256 and extract to a new directory, then run `Run-Draw.cmd -IsolateImplicitLayers` as a normal user. Require native RX 5700 XT strict Vulkan 1.3, all original nine contract cases, clear/triangle full RGBA and guarded blob matches, zero validation errors including cleanup, exit 0. Return `result.json`, all logs (including draw-oracle/draw-spirv), and `triangle.ppm`; retain failed/skipped attempts. P4.2 remains IN_PROGRESS until that native evidence is reviewed. No next development phase, texture work, guest shader translation or game launch performed.

A local documentation script used the Windows default text encoding; the final documentation check detected and restored affected punctuation to UTF-8 before handoff. This includes the package README title. The initial clean package above is retained as tested evidence; a replacement from the corrected committed documentation will be built and checked before transfer. Runtime/checker source is unchanged.

## Final transfer artifact (supersedes the first package above)

Corrected-documentation source `ea829bfb8a353f1912ef65d8856d63e7f4e2df99`, build ID `20261028T195020234Z`. Final ZIP `Kyty-MeshPrototype-Windows-x64-ea829bfb8a353f1912ef65d8856d63e7f4e2df99-20261028T195020234Z.zip`, **708,420 bytes**, SHA-256 `329FD032407AE1123FA9926B6BBE17977FBC1E4B32EF72686E4F9F48D8C30B2C`, matching sidecar under ignored `_Build/mesh-prototype-packages/`. Source/submodules clean; final containing handoff commit changes documentation only. This source supersedes the first package solely to include corrected UTF-8 README punctuation; all other runtime payload files (executable, modules, checker/shader excerpts, runner and launchers) are byte-identical to the package with nine verified probe outcomes.

Final clean serial CTest: **8 PASS / 2 SKIPPED / 0 FAIL**, exit 0, **0.90 s**. Final ZIP independently hashed/extracted fresh to another space/Arabic path. From unrelated CWD, Windows PS5.1 auxiliary draw+recorded isolation PASS; strict required-RX draw SKIPPED 41 as expected on Intel. Unicode image path/hash unchanged and exact; core/sync errors=0/warnings=0 through cleanup. Payload identity checks and final JSON/results retained in `_Build/p4-s018/transfer-package.json`, `final-relocation-result.json`, `verify-final-package.log`, final results directories and new package evidence directory. Earlier failures/controller issue and initial package retained. Next task remains native RX strict draw of THIS final package; return logs/result/image. No push or next development phase.
