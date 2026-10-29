# Kyty standalone mesh-compute prototype — Windows x64

This package is a synthetic GPU-buffer correctness test, **not an emulator or an Astro Bot patch**. It uploads public synthetic data, runs compute and checks every word of a fixed 1840-byte output blob. No game files, firmware, keys or proprietary shaders are required or accessed.

## On the RX 5700 XT machine

1. Copy the ZIP and its `.zip.sha256` sidecar. Compare the ZIP's `Get-FileHash -Algorithm SHA256` result with that sidecar before extraction. Checksums detect corruption; they are not a digital signature.
2. Extract to a **new directory**. Do not overwrite an older package. Open the directory containing `Run-Strict.cmd`.
3. Prerequisites: Windows 10/11 x64, native AMD GPU driver exposing Vulkan 1.3, Microsoft Visual C++ v14 **x64** runtime at least as recent as the build tool version in `manifest.json`, and the Khronos validation layer with synchronization validation. Runtime/driver/layer installers are not included or run automatically. Official resources: [Microsoft runtime](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist/) and [LunarG Vulkan SDK](https://vulkan.lunarg.com/sdk/home).
4. Double-click `Run-Strict.cmd`, or run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Run-Prototype.ps1 -Mode Strict -RequireRx5700Xt
```

If validation layers are not registered, supply the directory containing their JSON and matching DLL (for a Vulkan SDK installation this is normally its `Bin` directory):

```powershell
.\Run-Strict.cmd -ValidationLayerDirectory "C:\VulkanSDK\<installed-version>\Bin"
```

The script adjusts `VK_LAYER_PATH` only for its own process and restores it afterwards. It never installs drivers/layers, changes registry settings, forces a software ICD or launches a game. Existing ICD overrides are recorded in results; confirm that the selected device/driver is native AMD.

## Results and limits

The runner checks the package inventory and all file hashes first. Keep its contents intact, including notices and source excerpts. Unexpected files or changed/missing files are rejected before execution. Results are written **outside the package**, by default under `%LOCALAPPDATA%\KytyMeshPrototypeResults\<commit>\<run-id>`. The console prints the exact location. An explicit `-ResultsDirectory` must be a new directory outside the package.

- Exit `0` / PASS: all requested checks passed. Strict launcher also requires an RX 5700 XT device identity.
- Exit `41` / SKIPPED: required Vulkan version/device/validation facility is unavailable; no runtime pass.
- Other nonzero exit / FAIL: integrity, process startup, API, shader, comparison, validation or target-device check failed.
- `NOT RUN` in the JSON: a runtime test was not requested or preflight failed.

Retain `result.json` and all logs, including failures/skips. A successful strict test requires all nine full-buffer scenario comparisons and **zero validation errors after cleanup**. Nine scenarios contain eight actual one-workgroup dispatches and one no-dispatch Pending initialization check. The compute-only mode produces no image/window/gameplay/FPS. Results prove only this synthetic workload on the recorded device.

Other explicit modes:

```powershell
.\Run-Prototype.ps1 -Mode Offline       # host checks + both offline shader validators; runtime NOT RUN
.\Run-Prototype.ps1 -Mode Vulkan12      # auxiliary only; never Vulkan 1.3 or RDNA1 certification
.\Run-Prototype.ps1 -VerifyOnly         # inventory/hashes only; no native executable runs
```

Do not treat Intel/software Vulkan 1.2 results as RX or Vulkan 1.3 evidence. Keep the previous extracted package to roll back: run its own launcher from its own directory; no shared package files or automatic updater exist. Desktop deployment and rollback still need independent verification.

`manifest.json` identifies the exact clean source commit, submodules, toolchain, flags, build-time test statuses and PE imports. `SHA256SUMS.json` covers payload files and the manifest. The authored fixture/checker source excerpts are in `source/`; they are not the complete upstream source tree. Full development source, dependency revisions and build instructions remain in the originating repository at the recorded commit; preserve the supplied licenses/notices. This is a private experimental test artifact, not a general release.

## Rebuilding from the repository

Use the repository's x64 VS developer shell with `clang-cl`, CMake, Ninja, `dumpbin`, glslang and initialized pinned submodules. From **clean committed source**, run:

```powershell
tools\mesh-prototype\Build-Package.ps1 -ValidationLayerDirectory "<local-validation-layer-directory>"
```

The default cached build is `_Build/windows-core`. Outputs go under ignored `_Build/mesh-prototype-packages`, named with full commit and UTC build ID. Two build jobs, serial focused tests, no PGO/IPO. Use `-Jobs 1` only after retaining an actual memory-failure log. Configure/build/import/test logs and full local cache stay in a separate evidence directory, not the transfer ZIP. The script refuses dirty source/submodules and does not delete existing outputs, push, publish or change the branch.

## P4.2 offscreen triangle

Run `Run-Draw.cmd` for strict Vulkan 1.3 plus native RX identity. It retains all nine
P4.1 full-buffer cases, then runs a clear-only control and a compute-generated
indexed draw. The same output buffer supplies float4 vertex positions and uint32
indices at byte 1032, without an intervening CPU readback. All 4096 RGBA pixels
must match exactly (496 white triangle pixels, remaining pixels black; alpha 255),
alongside the full 1840-byte output including guards. Core/sync validation must
report zero errors through cleanup. No window opens. `triangle.ppm` is saved outside
the package alongside logs; its SHA-256 is recorded in result.json. Host oracle and
all four vertex/fragment module validations are additional preflight checks.

If the earlier missing Steam layer manifests recur, use normal, non-admin execution:

```powershell
.\Run-Draw.cmd -IsolateImplicitLayers
```

This explicit option temporarily points implicit-layer discovery at a new empty
results subdirectory, records the override and restores it on return. Explicit
Khronos validation remains mandatory. No registry/driver change occurs. Elevation
can cause the Vulkan loader to ignore search-path overrides; errors still FAIL.
The option is also available on the compute-only runner. Keep previous logs/packages.

For laptop auxiliary evidence only: `Run-Prototype.ps1 -Mode Vulkan12 -Draw`.
Strict unavailable Vulkan remains SKIPPED, never silently downgraded. Offline draw
preflight: `Run-Prototype.ps1 -Mode Offline -Draw`. This fixture is untextured and
synthetic, with no guest shader translation or emulator integration. It does not
complete the broader textured-mesh or Astro Bot gate.

## P4.3 textured triangle

Run `Run-Textured.cmd -IsolateImplicitLayers` on the RX desktop (normal user).
Strict Vulkan 1.3 and RX identity remain required. This explicit mode preserves
all nine P4.1 cases and the P4.2 untextured control/draw, then checks a clear
control and textured triangle using separately compute-generated UVs. A tiny
public authored 2x2 RGBA8 texture (red/green/blue/yellow) uses nearest sampling and
clamp-to-edge. No game assets or emulator integration.

The original 1840-byte blob is unchanged. A separate 544-byte UV output contains
64 float2 slots, six guard words, count and Pending/Ok/overflow status; every word
is compared after readback. Extra actual UV dispatches test zero, capacity 64,
reject 65 without partial writes, and same-buffer recovery. UV and position
compute work feeds vertex attributes directly in the draw submission. Every
RGBA pixel must match, including opaque background and all four texture colors.

Results outside the package include `triangle.ppm` (textured), `untextured.ppm`
(reference mode), each with a SHA-256 in result.json, plus `texture-fixtures.log`
and `texture-spirv.log` and the earlier preflight/runtime logs. Return all result
files; retain failures/skips. No window opens. Offline preflight uses `-Mode
Offline -Texture`; laptop-only auxiliary rendering uses `-Mode Vulkan12 -Texture`.
These do not certify native RX strict execution. Existing `Run-Strict.cmd` and
`Run-Draw.cmd` retain their previous compute/untextured behavior. Texture work does
not implement guest mesh translation or game compatibility.

## P3.2 generated IR modules

Run `Run-IR.cmd -IsolateImplicitLayers` on the RX desktop as a normal user.
This runs positive/rejection IR fixtures and three actual generated modules:
triangle, permuted indices and zero output. Each dispatch compares the complete
1840-byte output against an independently scripted host expectation; validation
errors including cleanup fail the run. The Vulkan 1.2 auxiliary modules are
assembled from the same IR-generated SPIR-V assembly and stay separate from the
strict 1.3 gate. Missing generated modules fail without a handwritten fallback.
Return result.json and all logs; no image is expected. Existing modes remain intact.

The IR fixture executable additionally imports `MSVCP140_ATOMIC_WAIT.dll` from the Microsoft Visual C++ v14 x64 redistributable; the matching runtime is an external prerequisite.

## IR arithmetic

Run `Run-IR-Add.cmd -IsolateImplicitLayers` on the RX desktop. This runs the add and wrap generated modules with strict Vulkan 1.3, full-buffer checks and validation including cleanup. Keep all result logs and result.json. This is synthetic research, not emulator or game compatibility evidence.

## Consolidated generated-IR draw milestone

Run `Run-IR-Milestone.cmd -IsolateImplicitLayers` as a normal user on RX. One run
checks host/IR/UV/draw/texture preflights, then three untextured generated modules
(triangle, permuted, zero) and two generated-UV textured modules (UV, zero). All
five strict Vulkan 1.3 selections must be RX 5700 XT, with complete buffers, all
pixels and zero validation errors/warnings through cleanup.

Return the entire results folder: result.json, all logs (including
runtime-ir-draw.log and runtime-ir-texture.log), and five PPM images. JSON records
per-stage device/status evidence and image SHA-256 values. No window opens.
Offline mode: `Run-Prototype.ps1 -Mode Offline -IrMilestone`. Local auxiliary
mode: `Run-Prototype.ps1 -Mode Vulkan12 -IrMilestone`; this is separate from
native strict evidence. FAIL takes precedence over SKIPPED; both runtime stages
are attempted after preflight succeeds. Old launchers retain their modes.

This package batches the generated draw and textured UV research milestone.
Production guest translation, emulator integration and Astro Bot are separate.
