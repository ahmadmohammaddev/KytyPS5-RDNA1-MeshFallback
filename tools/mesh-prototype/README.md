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

Retain `result.json` and all logs, including failures/skips. A successful strict test requires all nine full-buffer scenario comparisons and **zero validation errors after cleanup**. Nine scenarios contain eight actual one-workgroup dispatches and one no-dispatch Pending initialization check. No image/window/gameplay/FPS is expected. Results prove only this synthetic workload on the recorded device.

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
