# S-051 actual production pixel compiler / bridge pairing

Input44095640, branch feature/rdna1-mesh-fallback, clean; all14 matching submodules verified. Adds MeshPixelBridgePairingTests.inc inside existing shaderCfgTests.cpp and dedicated --mesh-pixel-bridge-only / CTest mesh_pixel_bridge_pairing. No production renderer/compiler/bridge implementation changes.

Public authored VINTRP P1/P2 + EXP MRT0 + S_ENDPGM ISA passes the actual top-level ShaderRecompiler::TranslateProgram, resource plan extraction/materialization, CompileProgram and production SPIR-V emitter via existing RecompileForTest. This is no longer a merely native-shaped handwritten fragment. CompileOptions/pixel metadata use one parameter with identity interpolation mapping and explicit target export mode. Plain and parameter0.xy fragments pass BuildVertexBridge; actual parameter z/w readers, remapped parameter location, flat and no-perspective variants reject. Missing UV for actual parameter reader rejects. Unsupported semantics are not rewritten or ignored.

Both actual pixel and generated vertex modules validate strict Vulkan1.3 offline. Independent local binary interface inspector matches pixel Input vs bridge Output location/scalar-width/vector-width signatures and verifies bridge vertex-input float4 position/optional float2 UV ABI. Canonical bridge interface identity matches accepted contract. Artifacts four paired SPIR-V modules + full hashes retained. Existing S-050 independent bridge dataflow/mutation tests remain in the focused suite.

First full shader_cfg_tests target build (248 scheduled actions, two jobs) PASS with existing upstream warnings; no memory exhaustion/retry, no PGO/IPO. Full pre-build source snapshot `_Build/p0-s012/p3-s051-pre-build-01/`. Focused pair test PASS1/1(0.18s); final37 cases28 PASS/9 strict SKIPPED/0 FAIL(4.65s). IR validation plus KYTY_TEST_COMPARE_LINEAR_USES=1 pair run also PASS1/1(0.06s), requiring equal native pixel SPIR-V for the two existing SSA-use bookkeeping modes. No failing attempt this session. Logs/XML/executable and source identities/paired modules/independent interface-check results under ignored `_Build/p3-s051/`.

Commands: developer-shell cmake --build _Build/windows-core --target shader_cfg_tests --parallel 2; serial ctest -R '^mesh_pixel_bridge_pairing$' with process-local KYTY_MESH_PIXEL_BRIDGE_DUMP; python _Build/p3-s051/check_pair.py; serial focused mesh+pixel_interface+unused_pixel_inputs CTest with process-local VK_LAYER_PATH; pair rerun with KYTY_IR_VALIDATE=1 and KYTY_TEST_COMPARE_LINEAR_USES=1. Exact commands and actual output retained locally.

| Category | Result |
| --- | --- |
| Compilation | PASS existing full-emulator-linked offline test target,2 jobs; no GUI rebuild |
| Unit tests | PASS actual pixel pairing/negative cases plus pixel interface and focused host/IR/UV regressions |
| Shader compilation / SPIR-V | PASS actual public pixel ISA compiled by native pipeline, paired bridge strict1.3 validation and independent interface check |
| Vulkan integration | Existing Intel1.2 auxiliary regressions PASS; strict local1.3 SKIPPED. New actual pixel+bridge combined pipeline NOT RUN |
| RDNA1 | New actual emitted pixel/bridge pairing runtime NOT RUN; prior S-045 native geometry/UV evidence retained |
| Astro Bot | NOT RUN |

No new package, RX request or push. This closes only the offline actual-emitter pairing gate. No descriptor sampling, arbitrary pixel shaders, unsupported builtins/interpolation or production draw/cache/scheduler integration claim.

Next single local task: extend the isolated harness to draw actual frontend-generated geometry/UV through the generated conventional bridge and actual production-compiled public pixel fragment. Independent full geometry/UV buffers, clear/zero controls and full RGBA output expectations; correct dependencies and validation through cleanup. Strict1.3 and Intel1.2 module/execution evidence separate; no authored vertex/fragment replacement in the new mode. Consolidate native RX testing at that coherent combined pipeline gate under D-028, not for this offline-only patch.
