# S-037 complete linear IR chains

Input source: `8b43abc5ea1d65b8096bbefa7b3269b7ebd7a928`, branch `feature/rdna1-mesh-fallback`.

The isolated lowerer accepts complete straight-line chains with explicit multi-block terminators. It checks graph edges, unconditional control flow, metadata IDs, SSA ordering and total bounds. Cycles, forks, joins, reordered/foreign blocks, forward SSA and partial metadata reject with empty output. No production renderer changes.

The real constant frontend probe now lowers and validates for Vulkan 1.3. The live vertex-ID probe still explicitly rejects `GetBuiltin`; this is an offline constant-shader result, not useful geometry or general NGG support.

Two two-job builds passed. Initial positive regression failed exit321 from a dangling cross-block use in test object destruction; fixed the test lifetime and retained the failure. Final serial focused CTest suite: 27 cases, 20 PASS, 7 strict1.3 SKIPPED, 0 FAIL, 3.45 seconds. Existing Intel1.2 compute/draw/texture regressions pass with validation through cleanup. New frontend module GPU execution, RDNA1 and Astro Bot NOT RUN.

Commands: existing capture.ps1 with names p3-s037-pre-build-01/-02; developer-shell cmake build targets mesh_ir_frontend_tests and mesh_compute_proto_tests --parallel 2; mesh_ir_frontend_tests --dump; serial CTest regex covering mesh_ir_, mesh_output_contract, mesh_proto_, mesh_compute_proto, mesh_draw_, mesh_offscreen_draw, mesh_texture_, mesh_textured_draw. Full commands and outputs retained in `_Build/p3-s037/`; complete dirty source snapshots under `_Build/p0-s012/`.

Next: declared non-indexed single-group specialization of real frontend prologue, exact invocation-index lowering and fail-closed context tests. Indexed resources, arbitrary CFG, LDS, dynamic counts, renderer integration and game remain open. No push.
