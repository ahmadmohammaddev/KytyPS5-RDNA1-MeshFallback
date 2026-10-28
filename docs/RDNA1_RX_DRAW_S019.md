# S-019 — Native RX 5700 XT offscreen indexed draw PASS

## Identity and actual execution

User-supplied desktop logs and image, reviewed on 2026-10-28. Package source `ea829bfb8a353f1912ef65d8856d63e7f4e2df99`, build ID `20261028T195020234Z`, matches the final S-018 transfer package. Known original ZIP: 708,420 bytes, SHA-256 `329FD032407AE1123FA9926B6BBE17977FBC1E4B32EF72686E4F9F48D8C30B2C`; no independent desktop ZIP rehash supplied.

Runner records Strict, draw=true, isolateImplicitLayers=true, requireRx5700Xt=true. Runtime arguments: `--draw --image-output <external-results>/triangle.ppm`, equivalent launcher configuration `Run-Draw.cmd -IsolateImplicitLayers`; exact shell/click transcript not supplied. Start UTC `2026-10-28T20:01:20.1934186Z`, duration 0.1295435 s, exit 0 / PASS. All six runner stages PASS, none FAIL or SKIPPED.

Native AMD Radeon RX 5700 XT, DISCRETE_GPU, vendor `0x1002`, device `0x731F`, API 1.4.308, graphics+compute queue family 0. Loader 1.4.309; AMD proprietary driver ID 1, raw `0x00800156`, string `25.6.1 (AMD proprietary shader compiler)`, conformance 1.4.0. Strict Vulkan 1.3 compute/vertex/fragment modules execute as SPIR-V 1.6, without downgrade. ICD DLL path/hash, desktop OS build and measured CPU/RAM inventory not supplied; machine specifications remain user-reported.

Implicit-layer discovery is explicitly isolated to an empty results subdirectory, recorded in both runtime log and JSON. VK_ADD_IMPLICIT_LAYER_PATH, explicit layer overrides and ICD/driver overrides are unset. Khronos core and synchronization validation remain enabled; no missing Steam manifest errors. Private paths/usernames remain in ignored evidence, not this report.

## Results by category

| Category | Actual desktop evidence |
| --- | --- |
| Compilation | NOT RUN this attempt; S-018 scoped build/clean package evidence retained |
| Unit tests | PASS: host-contract, host-fixtures and draw-oracle stages, exit 0 |
| Shader compilation and SPIR-V validation | Compilation NOT RUN; both compute modules and four vertex/fragment modules validate PASS offline, including both 1.3/1.2 targets |
| Vulkan integration | PASS strict Vulkan 1.3: original nine exact full-buffer scenarios, then offscreen clear control and compute-generated indexed triangle. Zero validation errors/warnings after all cleanup |
| RDNA1 hardware | PASS scoped P4.2 untextured conventional draw on native RX 5700 XT; no broader textured/guest/game claim |
| Astro Bot compatibility | NOT RUN |

Original nine scenarios include Pending initialization, zero, triangle, exact capacity, vertex/index/combined overflow, and both true same-buffer recoveries. Eight original compute dispatches plus one new compute-to-draw dispatch; the draw consumes the generated vertex/index buffer in the same submission with no intermediate CPU readback. Clear-only and triangle controls each report **0/4096 RGBA pixel mismatches**, with full output blob checked. Final summary: `scenarios=9 failures=0 validation-errors=0 validation-warnings=0`, followed by `DRAW RESULT: PASS` and strict Vulkan 1.3 PASS.

Large preserved count words printed for rejected overflows are the intentional nonzero snapshot, not emitted geometry. Output/payload/readback memory is coherent; noncoherent runtime remains unexercised. Desktop auxiliary Vulkan 1.2 runtime NOT RUN; validation of the 1.2 modules is offline evidence only.

## Independent image and evidence checks

Supplied `triangle.ppm`: 64x64 RGB P6, 12,301 bytes, SHA-256 `2F1FAA2BE459EA8C0EE7278C4FBB01C8813ED624AE81EE7E7745B8AD33D0956D`. Hash matches result.json and S-018's Intel reference artifact byte for byte. Independently evaluated floating-point barycentric coordinates at every pixel center (different formulation from the harness integer edge oracle): **496 white pixels, 3600 black, zero RGB mismatches**. Visual inspection confirms the white right triangle. PPM has no alpha channel; complete RGBA/alpha validation is evidenced by runtime, not inferred from RGB alone.

Screenshot shows the expected result files and empty implicit-layer directory; it is folder evidence, not an independent GPU utilization measurement. Raw screenshot is retained privately. No FPS, GPU timestamp/utilization trace or peak VRAM measurement supplied; runtime durations are host elapsed time, not GPU performance. Buffer/image correctness establishes native execution but not gameplay, performance or a device-local-only memory strategy. Desktop rollback NOT RUN.

All ten supplied files, including two identically hashed host-contract logs and screenshot, copied unchanged to ignored `_Build/p4-s019/rx-input/`; originals untouched. SHA-256 inventory `input-hashes.json`, programmatic image/JSON/log verification `verification.json`, rendered PNG preview local only. Duplicate host-contract input does not count as another executed test or prove a separate run.

Review input `aad5e2b11234ecbc3aa4c9b2c2636be49c3e8e80`, branch `feature/rdna1-mesh-fallback`, clean input and all 14 matching submodules/pinned ancestry verified. No source changes, builds, local Vulkan reruns, system settings or push. Documentation changes: this report, living plan and AGENTS.

## Status and next recommended single task

**P4.2 DONE (with native hardware evidence)**. No remaining blocker for this bounded acceptance gate. Overall P4 remains IN_PROGRESS: texture/varyings and the broader memory/measurement requirements remain open; P3 guest-IR translation feasibility and P5 integration are not completed by public authored shaders.

Next P4.3 (NOT_STARTED): extend the isolated public synthetic fixture to one textured compute-generated triangle, carrying compute-generated UVs through ordinary vertex-to-fragment interpolation into sampling of a tiny authored texture. Specify/test a bounded separate varying-buffer layout while preserving the existing P4.1 blob; add explicit visibility/image dependencies, deterministic full-image and buffer/guard comparisons, offline SPIR-V validation, clean portable package identity and native RX strict Vulkan 1.3 execution with zero validation errors including cleanup. Preserve plain compute/untextured modes and API evidence split. No emulator wiring, guest shader translation or game launch yet. This is the next bounded part of the existing textured P4 gate, not a claim that broader P4 is complete. No implementation started in this review.
