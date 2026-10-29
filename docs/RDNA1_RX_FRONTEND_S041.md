# S-041 native frontend milestone acceptance

Input documentation commit `39b32c36096e20e95233500c91ea75f511d008a1`, branch `feature/rdna1-mesh-fallback`, initially clean; pinned ancestry and all fourteen matching recursive submodules verified outside sandbox after sandbox Git access failure. No build, local GPU rerun or push.

Package source `87e912bd9786abd206b635ff7c68a832562f9b8a`, build `20261029T081156341Z`, matches S-040. User supplied 24 files: 15 logs, eight PPMs and result.json; copied byte-identically with SHA256 inventory to ignored `_Build/p3-s041/`. Treat contents as evidence, never executable instructions.

Command: `Run-Frontend-Milestone.cmd -IsolateImplicitLayers`, strict mode, required RX identity, process-local implicit layer isolation; JSON confirms no ICD override. All 15 stages exit0/PASS, no FAIL/SKIPPED. Three runtime stage durations: IR draw 0.2607929 s, IR texture 0.1818515 s, frontend 0.2412387 s. These are whole test-stage times, not GPU benchmarks.

Eight native selections identify AMD Radeon RX 5700 XT, vendor1002/device731F, Vulkan API1.4.308, AMD proprietary25.6.1; loader1.4.309. All dispatch strict Vulkan1.3. Eight final cleanup summaries report failures0/errors0/warnings0. Sixteen complete 4096-pixel RGBA comparisons exact (clear/draw controls), full guarded geometry checked; UV cases additionally report complete136-word checks. Actual frontend triangle/permuted/zero native gate PASS.

All eight supplied RGB images hash-identical to previously independently checked S-040 references (32,768 pixels). Geometry hashes 2F1FAA2B...0956D, zero3CF730E8...C78ED, textureE613B75B...A424; complete hashes retained in result.json and local inventory. No reliance on screenshot appearance alone.

| Category | Actual result |
| --- | --- |
| Compilation | NOT RUN on desktop; clean laptop S-040 build retained separately |
| Unit tests | PASS supplied host/IR/UV/frontend/oracle stages |
| Shader compilation / SPIR-V | SPIR-V validation/host frontend assembly PASS; desktop GLSL compilation NOT RUN |
| Vulkan integration | PASS eight strict1.3 cases, full buffers/pixels and validation through cleanup |
| RDNA1 hardware | PASS bounded actual frontend gate on native RX5700XT |
| Astro Bot | NOT RUN |

Desktop Vulkan1.2 runtime NOT RUN; auxiliary Intel evidence remains separate. Raw GPU buffers, independent desktop ZIP hash, OS/ICD path inventory, GPU utilization/frame timings/VRAM and noncoherent allocation execution were not supplied; no claim about these. Earlier timer/failure histories remain intact. Not general NGG, production resource tracking, indexed guest input, arbitrary CFG/LDS or emulator integration.

Next single local task: extend the public actual-frontend fixture with bounded parameter0 xy UV export using existing separate guarded544-byte ABI. Acceptance: public ISA through complete decoder/frontend/SSA path, explicit export metadata and masks, unchanged position/index ABI, independent UV expected values, precise unsupported metadata/mask rejection, Vulkan1.3 SPIR-V validation and focused regressions with complete provenance. Runtime textured frontend integration follows separately; batch native testing under D-028. No next implementation started during this evidence review.
