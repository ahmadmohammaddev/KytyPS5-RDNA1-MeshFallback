# S-029 — Native RX IR arithmetic

Seven user-supplied files reviewed and copied/hash-verified under ignored `_Build/p3-s029/`. Input checkout `98792bdc6a82829f0af8300449dfdbb30cc41a0a`, clean branch and pinned ancestry/14 submodules verified. No local compilation or GPU execution.

Run UTC 2026-10-28 21:57:30, package source `3fd0fdd507a91759a0ca41be4150dff94d5e6485`, build `20261028T215237152Z`; desktop ZIP hash not independently supplied. Required-RX strict `--ir-add` with process-local implicit-layer isolation. Selected RX 5700 XT, AMD 25.6.1, API 1.4.308, loader 1.4.309; Khronos core and synchronization validation enabled. Six stages PASS, runtime exit 0 in 0.2373225 seconds. Both add and wrap strict 1.3 generated modules dispatch and report exact 1840-byte/460-word buffer matches; each cleanup reports zero errors and warnings. Raw buffer data was not supplied for independent rechecking.

| Category | Result |
| --- | --- |
| Compilation | NOT RUN on desktop |
| Unit tests | PASS, packaged host contract, fixtures, IR positive and rejection cases |
| Shader compilation / SPIR-V | Compilation NOT RUN; offline validation PASS |
| Vulkan integration | PASS for strict generated arithmetic and wrap; desktop Vulkan 1.2 runtime NOT RUN |
| RDNA1 | PASS for the scoped arithmetic tests |
| Astro Bot | NOT RUN |

Mapped buffers were coherent; noncoherent runtime remains untested. GPU time/utilization/VRAM, ICD file identity and OS build were not recorded. No image was expected for buffer-only tests. Host runtime duration is not GPU performance evidence. Broader guest translation, conventional drawing of generated IR, emulator integration and game compatibility remain open. Intel Vulkan 1.2 evidence remains separate.

Next: combine restricted generated IR geometry with standalone conventional indexed draw and validate full buffer and image with correct dependencies; native RX strict validation is a separate gate.
