# Source, licenses and upstream credit

This isolated test belongs to the experimental KytyPS5 RDNA1 mesh-fallback research branch. Preserve all supplied license texts and notices.

- **KytyPS5**, based on [Jetsku/KytyPS5](https://github.com/Jetsku/KytyPS5): project license GPL-2.0-only; full text in `LICENSE`. The exact research source commit and recursive dependency revisions are in `manifest.json`. The synthetic test does not implement PS5 guest shader translation or game compatibility.
- **Original Kyty**, [InoriRus/Kyty](https://github.com/InoriRus/Kyty), copyright (c) 2021 InoriRus: original MIT notice retained in `LICENSES/Kyty-MIT.txt` for upstream credit.
- **SPIRV-Tools**, [KhronosGroup/SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools): the validator/disassembler library is statically linked into `mesh_compute_proto_tests.exe`. Its Apache-2.0 license is copied verbatim to `LICENSES/SPIRV-Tools.txt`. See that dependency's recorded commit for its full source and per-file copyright notices.
- **SPIRV-Headers**, [KhronosGroup/SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers): definitions/generated tables used by the validator; its full, combined license/notice file is retained verbatim as `LICENSES/SPIRV-Headers.txt`. Do not replace it with a summary license label.
- The authored public synthetic shader is included in `source/mesh_output_proto.comp`; it contains no game content. glslang is a build-time compiler and is not included as a runtime executable/library.
- Microsoft Visual C++ runtime, Windows libraries, GPU Vulkan ICD/loader, and Khronos validation layers are **external prerequisites**, not redistributed by this package. Their providers' licenses apply. No Qt, SDL, emulator executable, driver, validation-layer DLL, game asset or proprietary shader binary is bundled.

Source excerpts are provided for inspection; they are not a representation that the entire corresponding upstream/dependency source tree is present in this small test package. Retain the originating source repository and pinned dependency revisions when transferring this private experiment between the developer's machines. Public redistribution/release is outside this task.
