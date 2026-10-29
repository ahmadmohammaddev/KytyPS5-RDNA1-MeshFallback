# S-061 — Lowering-proven pure execution layout

User cancelled external delegation; resumed implementation in owner repository.
Existing lowerer Result.vertices/primitives already come from allocation/coverage/
index validation and successful SPIR-V compilation. Reused directly: no count
inference from guest input draw, filenames or readback. CompileBundle now publishes
an optional checked ExecutionPlan atomically only on compiler success; missing
plan makes bundle readiness false. Bundle identity domain version raised1->2.

Pure layout includes full guarded allocation sizes, active vertex/index/UV ranges,
strides, uint32 index width, set0 binding1 geometry/binding2 optionalUV, single-group
dispatch, initialization and compute-write to graphics-consumer dependency flags.
Generated compute bakes its values and has NO binding0 input payload; authored
prototype payload must not leak into this generated-module plan.0/0 keeps dispatch/
initialization but has no indexed draw. No command/barrier execution or allocation.

Count/capacity checks precede multiplication (max64/64); uint64 byte arithmetic,
ends constrained to guard boundaries. All rejected layouts nullopt, no partial plan.
CheckedExecutionLayout is an internal arithmetic helper accepting counts, not a
security boundary/proof generator: consume only a successful compiler-owned bundle.
Public structs are trusted CPU-owned data; consumers must not mutate/cross-wire
plans or treat arbitrary caller integers as compiler proof. Allocation lifetime and
reuse must wait for both compute and graphics completion; flags do not enforce it.

Real frontend constant/triangle/permuted/zero/UV/UV-zero fixtures independently
assert3/1/3 indices and0/0/0,48/12 active geometry bytes and24 UV bytes,offset1032
andUV8; boundary64/64,UINT32_MAX/65,missing proof,inconsistent counts,UV undercapacity
and atomic compiler rejection checked. Existing source preservation/key tests kept.
Package staging includes new header; no transfer built.

Two-job focused builds PASS with complete snapshots p3-s061-pre-build-01/-02;
43 cases32 PASS/11 strict1.3 SKIPPED/0 FAIL(6.60s). Final readiness-only addition
rebuilt and affected frontend CTest1PASS(0.14s).44 modules unchanged. Logs under
ignored `_Build/p3-s061/`. Compilation/unit/SPIR-V scopedPASS; existing Intel1.2
Vulkan regressionsPASS; new execution-plan GPU/RX and Astro BotNOT RUN. S-059
native standalone acceptance retained separately. No renderer activation or push.

Next bounded task: consume compiler bundle/plan in the isolated Vulkan harness
with actual generated modules and no hardcoded draw counts/offsets, independently
verify triangle/permuted/zero+UV results, synchronization and lifetime through
cleanup, strict1.3 vs auxiliary1.2. Preserve source/module identity and fail closed
on plan/module mismatch. Production scheduler activation remains later. Continue
related local commits under D-033; batch RX transfer at coherent execution gate.
