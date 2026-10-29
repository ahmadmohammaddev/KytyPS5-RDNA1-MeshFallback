# S-063 — Consolidated compiler-bundle native transfer

Inputd7733424 clean branch; selected authorized next task: consolidate S-060/061/062
into a clean committed-source execution gate. BundleMilestone runner adds one CPU
compiler/proof preflight and three actual bundle cases to existing combined suite.
21 stages,15 actual native strict selections/cleanups,15 PPMs+5 CSV artifacts on
full success. Strict required-RX launcher Run-Bundle-Milestone.cmd. Source committed
before build; final build/package/relocation/negative results appended below.
No renderer/game activation. Intel auxiliary cannot close new native bundle gate.

## Final S-063 verification / Experiment E-063 — stop for native RX

Clean source `b2b9cb4c13f3f770bad0339d31fe33cae0c718d7`, build `20261029T141854777Z`.
ZIP3,267,899 bytes, SHA256 `A025D83E17083EA6756BF217AA2E55CFDFDAEC930A95D20ABEC1F5107962A1AE`, matching sidecar. Under ignored `_Build/mesh-prototype-packages/`; full provenance/flags/toolchain/submodules/import/license/CTest logs in corresponding -evidence directory. Clean2-job build PASS,46 serial cases34 PASS/12 strict1.3 SKIPPED/0 FAIL(7.01s), no memory retry/PGO/IPO. No new emulator activation/build claim.

Relocated Unicode/unrelated-CWD verification:25 expected integrity/legacy/strict/auxiliary/RX-guard/conflict/launcher outcomes PASS. Actual BundleMilestone Intel1.2 auxiliary21 stages/15 physical selections/15 cleanup summaries zero errors/warnings;15 PPMs exact against independent reference hashes,5 complete4096-row ordered finite RGBA CSVs independently checked within2^-20,44 existing shader modules byte-identical. Three bundle cases compile in-process and consume the immutable proof/plan; zero records no indexed draw. Two additional packaged-executable negative probes correctly FAIL: valid xy-swapped native pixel488 pixels/zero validation through cleanup; missing required fragment before device discovery.27 expected package/negative outcomes total. Negative local fixtures are not the distributed archive.

Initial verification script incorrectly compared FileHash command output with a SHA string; retained failure in verification.log, all actual artifacts independently rehashed correctly. Fixed helper uses explicit Get-FileHash.Hash and fresh extraction verification-02; complete rerun PASS. Archive/source unchanged by checker repair. Evidence `_Build/p3-s063/verification-02/`, verification02.log, independent-checks.txt and packaged-negative logs retained. ZIP+sidecar hashes checked again after all tests.

Six categories: Compilation scopedPASS; Unit/offline compiler/proof testsPASS; Shader compilation/SPIR-VPASS/44 unchanged; Vulkan integration Intel1.2 auxiliaryPASS/strict1.3SKIPPED locally; new bundle execution RDNA1NOT RUN; AstroBotNOT RUN. S-059 previous native standalone paired acceptance remains distinct. No performance/FPS measurement; no push.

Next single hardware task: fresh extraction on RX, normal user `Run-Bundle-Milestone.cmd -IsolateImplicitLayers`. Require21 stagesPASS,15 independently identified native RX5700XT strict1.3 selections/zero-error+warning cleanup summaries, complete geometry/UV/coverage/clear/zero checks, permitted UNORM and bounded float triangle/permuted/zero results. Return full folder:21 logs,result.json,15 PPMs,5 CSVs (42 files), including bundle-frontend_uv*, bundle-frontend_uv_permuted*, bundle-frontend_uv_zero*. Missing prerequisites SKIPPED; any actual failure FAIL; unexecuted cases NOT RUN. Compiler reference policy is not runtime-device admission. STOP under D-028/D-033; no production scheduler wiring/game work before independent gate.
