# S-057 — Portable paired acceptance correction

Selected bounded local task only; no renderer wiring, new package or push.

## Contract

UNORM accepts exactly floor/ceil of the independent rational clamped ideal*255,
not +/-1 around nearest. Integer values, zero/clear and saturation stay exact.
Source: [Vulkan normalized fixed-point conversion](https://docs.vulkan.org/spec/latest/chapters/fundamentals.html#fundamentals-fixedconv).
Nearest-different and permitted-set-invalid counts printed separately.

Every normal paired run now also executes the same native stages into RGBA32F
and checks all16384 values against independent unclamped mathematical values.
Absolute budget2^-20 is an explicit conservative fixture admission requirement,
not an assertion that Vulkan guarantees this precision for all drivers. It is
less than1/4096 of a UNORM step and less than1/48 of this fixture's nearest-round
threshold distance. Device exceeding it FAILS; no automatic budget adaptation.
Finite values/channel pairing/exact backgrounds and zero controls remain required.
Per-channel UNORM sets do not enlarge for interpolation error: this intentionally
conservative fixture may reject implementations near an integral boundary.
Separate-format draws cannot prove internal RGBA8 values; both gates are required.

## Actual verification

Input47a11a40 clean branch and14 matching submodules. Full dirty snapshots
`_Build/p0-s012/p3-s057-pre-build-01/` and -02 retained. Two-job focused builds
PASS; no PGO/IPO or memory retry. First early oracle command ran before final
binary completion; final43-case run supersedes it. Evidence `_Build/p3-s057/`.

Final focused CTest43:32 PASS/11 strict1.3 SKIPPED/0 FAIL,8.70s. Independent
regressions cover floor/ceil choices, interior integer, endpoints/saturation,
beyond-bracket errors, clear alpha, NaN/Inf, missing pair, coherent perturbation,
channel swap and spatial shift. Actual Intel1.2 paired triangle/zero run PASS:
full buffers/coverage/permitted colors plus numeric float checks, maxerror
1.17317078e-7, zero errors/warnings through cleanup. Valid wrong bridge rejects
488 pixels under permitted UNORM sets; isolated float mode independently rejects
it numerically (maxerror0.952628968), despite zero structural errors. Both negative
runs have zero validation errors/warnings after cleanup. All44 SPIR-V modules
byte-identical to S-053. Prior RX evidence is not relabeled by this new contract.

Six categories: Compilation scoped PASS; Unit PASS; shader generation/SPIR-V
PASS/44 unchanged; Vulkan Intel1.2 PASS and strict1.3 SKIPPED; new RDNA1 acceptance
NOT RUN; Astro Bot NOT RUN. No emulator/game milestone established.

Next single task: clean committed-source consolidated acceptance package, update
runner/README artifact expectations for mandatory paired float CSVs, verify all43
cases, Unicode relocation/integrity/API/required-RX and valid-wrong-module failures,
then hand off one command for fresh strict RX acceptance. Require all stages,
complete buffers/coverage/permitted UNORM plus numeric float triangle/zero and
zero errors/warnings including cleanup. Keep prior FAIL history; no renderer wiring.
