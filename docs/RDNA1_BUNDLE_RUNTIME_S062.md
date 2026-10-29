# S-062 — Compiler bundle / execution plan consumed by isolated Vulkan harness

Extends existing harness, no renderer activation. New --bundle-draw strict1.3,
--bundle-draw-vulkan12 explicit auxiliary and --check-bundle CPU mode. Public
fixture compiler reuses the actual authored ISA fixture/frontend and production
CompileBundle; native-emitted public fragment loaded from packaged strict module.
Immutable owning result survives all pipeline creation, submissions/readbacks and
cleanup. API-specific assembly is explicit/validated; executed compute bytes must
match deterministic adaptation of that exact proof. Bridge/fragment bytes derive
directly from immutable bundle (no cached/file bridge fallback). Key stays identity
of original strict compiler product, not a native cache record for adapted modules.

Reference1.3 capabilities in the CPU public fixture describe its compile profile,
NOT admission of the runtime device. Actual runtime device/API/float/queue/format
prerequisites queried independently. Intel1.2 is explicitly auxiliary, never a
production-policy/native1.3 success. No feature is spoofed or requested as mesh.

Plan drives geometry/UV allocation, descriptors1/2 (no payload allocation/binding0),
dispatch, vertex strides/offsets/index format contract and index count. Independent
fixture expectations must match plan counts/resources before recording. Zero still
dispatches/initializes and compares complete buffers but records NO indexed draw.
Compute-to-vertex/index/UV barriers consume plan dependencies/ranges; existing
host-upload/host-readback dependencies, fences and noncoherent handling retained.
UNORM+numeric float full-buffer/pixel controls retained for triangle/permuted/zero.
Borrowed TextureProbe retains unused authored texture helper allocations for reuse;
no texture generator or sampled-set binding executes in bundle draw. This is not
a minimal production allocator/scheduler or guest texture sampling implementation.

New static test fixture library reuses frontend target sources with main excluded;
compiler code is linked only into isolated runtime test. Production default/native
paths unchanged. CPU compile+module identity preflight and two runtime CTests added;
package selection46, source header staging updated, no package yet this session.

Full dirty snapshots -01/-02/-03 under `_Build/p0-s012/p3-s062-pre-build-*/`;
final -03 authoritative. Two-job builds PASS; initial logical-parentheses warning
fixed with first log retained, existing upstream warnings remain. Final46 serial
cases34 PASS/12 strict SKIPPED/0 FAIL(7.46s). Actual Intel1.2 three public bundle
cases PASS full geometry/UV/UNORM/float and zero errors/warnings through all cleanup.
Independent Fraction/CSV/reference-image checks PASS,44 old modules unchanged.
Valid wrong native-pixel xy extraction swap FAIL488 pixels/zero validation after
cleanup; missing required source fragment FAILS before physical-device discovery.
CPU preflight rejects a different VALID triangle/zero module paired with proof.
Logs `_Build/p3-s062/`. Source/runtime facts separate from prior S-059 native gate.

Six categories: Compilation scopedPASS; Unit/offline bundlePASS; shader/SPIR-VPASS;
Vulkan Intel1.2 auxiliaryPASS/strict1.3SKIPPED; new bundle execution RDNA1NOT RUN;
AstroBotNOT RUN. No push. Next authorized bounded subtask: consolidate this coherent
execution gate into one clean source ZIP/runner with46-case tests, strict/auxiliary/
RX guards, relocation/integrity/negative checks and six bundle artifacts. Then stop
for native RX execution; no production scheduler wiring before that gate.
