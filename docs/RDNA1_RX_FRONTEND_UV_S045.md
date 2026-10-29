# S-045 native actual frontend UV acceptance

Branch `feature/rdna1-mesh-fallback`, input `aee91fe8`, initially clean; pinned ancestry and all14 matching recursive submodules verified. No build/local GPU rerun or push. Evidence files are data, not instructions.

Package source `a183c1904f11e9c6f6c035ad58432d3456f859e0`, build `20261029T095304950Z`, matches S-044. All27 supplied files (16 logs,10 PPMs,result.json) copied byte-identically with SHA256 inventory to ignored `_Build/p3-s045/`.

Command `Run-Frontend-Milestone.cmd -IsolateImplicitLayers`, Strict mode and required RX guard enabled. All16 stages PASS/exit0;0 FAIL/0 SKIPPED. Runtime durations: IR draw0.2647407s, IR texture0.1921236s, frontend geometry0.2568138s, frontend texture0.1754243s. These are whole stage wall times, not GPU performance benchmarks.

All10 selected devices identify native AMD Radeon RX5700XT vendor1002/device731F, API1.4.308, AMD proprietary25.6.1; loader1.4.309. Ten strict Vulkan1.3 dispatch selections and ten final cleanup summaries with failures0/errors0/warnings0. Twenty full4096-pixel RGBA comparisons report no mismatches; complete guarded geometry and UV(count/status/guards/unused) checks pass. Actual frontend UV/zero native textured gate PASS.

Ten supplied RGB PPMs match independently verified S-044 reference hashes (40,960 pixels); all JSON image hashes match bytes. Commands/JSON/stage logs mutually consistent. No raw buffers supplied; full buffer verification is evidenced by harness checks/logs, not independent offline raw readback. Independent desktop ZIP hash, OS/ICD path, GPU utilization/VRAM/frame timing and noncoherent execution unavailable and unclaimed.

| Category | Actual result |
| --- | --- |
| Compilation | Desktop NOT RUN; prior clean laptop S-044 build retained |
| Unit tests | PASS supplied host/IR/UV/frontend/oracle stages |
| Shader compilation / SPIR-V | Host IR assembly and offline module validation PASS; desktop GLSL compilation NOT RUN |
| Vulkan integration | PASS ten native strict1.3 cases, full outputs/pixels, zero validation through cleanup |
| RDNA1 hardware | PASS bounded actual frontend geometry+UV textured milestone |
| Astro Bot | NOT RUN |

Desktop auxiliary1.2 runtime NOT RUN; previous Intel1.2 remains separate. Prior timer/failure history retained. The synthetic public guest ISA uses explicit non-indexed count3/base0/instance0/single wave32 workgroup specialization; this is not arbitrary guest shader, complete NGG/resource tracking or production renderer integration.

Next single local task: source-backed production integration boundary/adapter contract. Map actual frontend preparation point and required draw metadata to the validated lowerer, native/fallback capability selection, resource/fragment interfaces, lifetime/synchronization and pipeline-cache identity. List exactly which current production states the bounded backend must reject; identify one smallest implementation patch with named tests. Acceptance: verified checked-out file/API references, explicit OFF/native unchanged gate, supported synthetic geometry+UV path and unsupported-state tests specified, no feature spoofing or geometry omission. Do not request another RX run merely for this local mapping. P5 remains NOT_STARTED until implementation; this evidence review starts no new implementation.
