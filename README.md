<!--
# SPDX-FileCopyrightText: Copyright 2026 Hortensia Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later
-->

<p align="center">
  <img src="dist/branding/hortensia_icon_with_logo.png" alt="Hortensia" width="600">
</p>

# Hortensia

**Hortensia** is an experimental, open-source Nintendo Switch and Switch 2 emulator written in C++.
Built on the newest **Lichii** (`zortos293/Lichii` `nx2` Build 316+ with AIO-10 merged) and **Eglantine** foundation, Hortensia converges 51+ verified multi-fork commits and open Pull Requests across 20+ active community forks into a single, cohesive build with the signature **Hortensia Hydrangea** artwork and deep violet theme.

> [!WARNING]
> This is experimental software. Compatibility and performance vary by game, hardware, and GPU driver.
> Only use games, keys, and firmware dumped from your own console. Hortensia includes none of them.

---

## Downloads and Setup

1. Extract the build archive into a folder and run **`hortensia.exe`** (or **`hortensia-cli.exe`** from the command line).
2. **Data and Logs**:
   - Windows: `%APPDATA%\hortensia` (logs in `%APPDATA%\hortensia\log\hortensia_log.txt`)
   - Linux / macOS: `$XDG_DATA_HOME/hortensia`, `$XDG_CONFIG_HOME/hortensia`, `$XDG_CACHE_HOME/hortensia`
   - Portable mode: create a `user` directory next to `hortensia.exe`.
3. **Profile Migration**:
   - On first launch, Hortensia can import existing profiles, saves, keys, and shader caches from **Lichii**, **Bouquet**, **Eden / Eglantine**, **Citron**, **Sudachi**, **Suyu**, or **Yuzu**.

---

## Converged Forks & Pull Requests (Nov 17–18, 2026 Mega Build)

On top of `zortos293/Lichii` `nx2` (`d1dc720bee9`, Build 316+ / AIO-10), Hortensia integrates 51 commits from across the ecosystem:

- **`zortos293/Lichii` Open PRs & Active Branches**:
  - `capy/rebind` (`ecde0c7130`): Rebind a draw's geometry buffers when a submit lands inside its `Configure`.
  - `capy/oot-camera` (`1f9932d2f0`, `44a59d2813`): Scene camera refusal diagnostics (`#1866`).
  - **PR #2122** (`f4adea6aea`): Reset first-draw history on guest CPU upload (`InvalidateGpuWriteHistoryOnCpuUpload`).
  - **PR #2121** (`23e5a91efd`): Swizzle with Switch 2 native tiling in `sw_blitter`.
  - **PR #2118** (`ae5843014a`): Keep fast draws across occlusion counter resets and render enable modes.
  - **PR #2116** (`0f979a4935`): Keep fast draws across vertex stream stride, enable, and divisor changes.
  - **PR #2109** (`1ca9540956`, Schnurzel700): Layer Switch 2 Edition upgrade packs over the update RomFS.
  - **PR #2107** (`d0887b1b3f`): Trace engine writes and texture cache images at `EDEN_NX2_TRACE_DST`.
  - **PR #2082** (`5fcc5329d0`, pete842): Order shared stores in SPIR-V before other threads of the warp read them.
  - **PR #2081** (`ed7dd5da5c`, `ef3dcc364c`, DuskDarf1): Stop copying padded Switch 2 render targets per draw; updated `lichii-windows.profdata` PGO profile trained on Donkey Kong Bananza, Ocarina of Time, and Switch 1 titles.
  - **PR #2051** (`38b03a6046`, `2bdb457d50`): Restrict header byte-offset handles to Switch Sports and recapture texture classifications.
- **`tasarren/Lichii` (jpkto)**:
  - Leave square depth-only passes unscaled (`80b5f0181a`).
  - Keep GPU-written buffers over unbacked device memory (`fb0755a787`).
  - BC1 ASTC recompression preserves textures with alpha in BC3 (`9c76a3d9c5`).
  - Select the global memory buffer on NVIDIA only in compute programs (`a3c96b8ea1`).
  - Back notes a sync once between reads of the sync serial (`efa41e4240`).
  - Share pipeline and descriptor set layouts between pipelines with identical bindings (`df190a104a`).
- **`wallaceman3/lichii-custom-optimization-cpu-and-gpu`**:
  - Dynamic 85% VRAM target, 3-tier anti-thrashing texture garbage collector, and `ShaderCacheWriter` / `JoinImages` exception shield (`15eead4e6f`, `22347a05d3`, `831707e2e40`).
  - Discrete vs. integrated GPU memory threshold protection (`831707e2e40`).
  - Batch draw submissions to 256 and low-latency micro-spin on GPU readbacks (`8f0436d3be`).
- **`lethevimlet/lichii-bc250`**:
  - Pre-reclaim up to 1.5 GiB before guest read bursts (`EDEN_NX2_PRE_RECLAIM_MIB`, `75e9f1b8ae`, `4fbde27eb7`).
  - Skip initial guest-memory upload for newly created un-rendered depth images >= 64 MiB (`EDEN_NX2_TC_SKIP_DEPTH_UPLOAD_MIB`, `1f7248a9880`).
- **`Frizti/Fritchii` Open PRs**:
  - **PR #73** (Diriande): Manual and adaptive frameskip (`EDEN_NX2_FRAMESKIP`, `EDEN_NX2_FRAMESKIP_AUTO`, `fd52bd5dc9`..`8cfcb5d601`).
  - **PR #61** (FFcustoms): Allow supported AMD drivers to overallocate device memory (`862676d366`).
  - **PR #60** (joakyng): Add `"no microphone"` input device and honor `"none"` in Switch 2 `sysmix` capture path (`41f860bf5a`, `35b99bfecb`).
- **`Djihads80/Eglantine` & Open PRs**:
  - Honor full declared size of Mario Kart World NVN graphics lighting storage buffers (`894f2c119cf`, snowstormxd).
  - **PR #834** (`4919ef8c7b7`): Report the console as offline in NIFM while Nintendo's servers are blocked unless a LAN/multiplayer room is joined (prevents Mario Kart World ghost upload / NPLN transfer screen hang).
- **`Wael3rd/Eglantine-MKW` & `nauxph/Lichii-AMD` & `DahBard/Lichii`**:
  - Back Switch 2 sparse regions whole on AMD's Windows proprietary driver (`VK_DRIVER_ID_AMD_PROPRIETARY`) to prevent `amdvlk64.dll` integer divide-by-zero crashes during Mario Kart World map loads (`c2393e50abd`).
  - Claim a held draw's worker-only graphics pipeline build (`0b6b3acca7`).
  - Leave semaphore waiters cleanly when a puller is torn down mid-wait (`9f4294b1b4`).
  - Keep no stages for a failed pipeline on eviction in `vk_pipeline_cache` (`3880800d09`).
- **`MythosOfs/Orchid`**:
  - Shadow the packed operands of `TEX 0xf60` with a uniform handle (`9bf1c6d058b`).
  - Shadow high registers of `P2R`, uniform `I2FP`, and indexed `LDC` (`da256c3d7b`).
  - Model immediate `SHF` in geometry exit liveness (`28e69ddee2`).
  - Give a transform feedback varying starting inside a vector its own byte offset (`e95d04d94f`).
  - Bounds-check every ASTC decode and unswizzle image store in host shaders (`05ca33744a`).
  - Compile Dynarmic x64 blocks under the host `MXCSR` instead of the guest's (`165dc39f81`).
  - Flush adjacent device pages of a GPU range in one `FlushRegion` (`1876a7d72b`).
  - Show emulation speed in the status bar with multicore enabled (`8ebad36f58`).
- **`notsapinho/Eglantine` & `justr4mzi/hortensia`**:
  - Flush accelerated image-to-buffer DMA copies on Switch 2 (`EDEN_NX2_DMA_READBACK_FLUSH`, fixes banded photos in Pokémon Pokopia, `7dde4f158d4`).
  - NUP container companion image (`.jpg`/`.jpeg`/`.png`) and built-in Ocarina of Time icon fallback (`ef88193d31d`, `9741e3a174d`).

---

## License

GPL-3.0-or-later. See [LICENSE.txt](LICENSE.txt).
