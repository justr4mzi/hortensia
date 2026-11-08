<!--
SPDX-FileCopyrightText: Copyright 2026 Hortensia Contributors
SPDX-License-Identifier: GPL-3.0-or-later
-->

<p align="center">
  <img src="dist/branding/hortensia_icon.png" alt="Hortensia" width="180">
</p>

<h1 align="center">Hortensia</h1>

<p align="center">
  <b>An experimental Nintendo Switch 2 emulator that gathers the best work of the Eglantine forks in one build.</b><br>
  Windows · Linux · macOS
</p>

---

## Contents

- [What is Hortensia?](#what-is-hortensia)
- [Download](#download)
- [Setting it up](#setting-it-up)
- [Optimized settings per game](#optimized-settings-per-game)
- [Recommended settings per game](#recommended-settings-per-game)
- [What's inside, fork by fork](#whats-inside-fork-by-fork)
- [Every switch Hortensia adds](#every-switch-hortensia-adds)
- [What was left out, and why](#what-was-left-out-and-why)
- [Reporting a problem](#reporting-a-problem)
- [Known issues](#known-issues)
- [Building from source](#building-from-source)
- [Credits](#credits)
- [License](#license)

## What is Hortensia?

The Switch 2 emulation scene moves fast and is spread over many forks of
[Eglantine](https://github.com/Djihads80/Eglantine): one is best for Mario Kart World, another
for Ocarina of Time, another for AMD graphics cards… Hortensia picks the best changes from each of
them and puts them in a single emulator, so you don't have to juggle five builds.

Hortensia is built upon the **[SmileyTatsu](https://github.com/SmileyTatsu/Eglantine)** `build-20` PGO base
(delivering the highest in-game frame rates on Mario Kart World) combined with comprehensive HLE services,
native tensor/compute support, and graphics fixes from **[Lichii](https://github.com/zortos293/Lichii)**,
**[Selevet](https://github.com/sylvieistaken/selevet-eglantine)**,
**[PokeDinner](https://git.eden-emu.dev/Franseven/PokeDinner-a-Eglantine-Fork.git)**, and other forks.
**Every change keeps its original author** in the git history, and every optimization Hortensia adds can be
switched off (see [Every switch Hortensia adds](#every-switch-hortensia-adds)).

> [!WARNING]
> This is experimental software. Games may crash, show graphical errors or run slowly.
> You need your own legally dumped games, keys and firmware. Hortensia ships none of them.

## Download

Get the latest build from the **[Releases page](https://github.com/justr4mzi/hortensia/releases)**.
There is **one file per system**:

| Your computer | Download |
| --- | --- |
| **Windows** (almost every PC) | **`Hortensia-Windows-PGO-AVX2.zip`** (or `Hortensia-Windows.zip`) |
| Windows, only if Hortensia doesn't start or shows graphical errors | `Hortensia-Windows-MSVC.zip` |
| **Linux** | **`Hortensia-Linux-x86_64-PGO.AppImage`** (`Hortensia-Linux-ARM64-PGO.AppImage` on ARM) |
| **Mac** with Apple Silicon (M1 or newer) | **`Hortensia-macOS.zip`** |

<details><summary>What's the difference between the Windows builds?</summary>

- `Hortensia-Windows-PGO-AVX2.zip` is compiled with Clang, a performance profile (PGO) and AVX2 instructions
  (x86-64-v3: Intel Haswell 4th gen or newer, AMD Ryzen): peak frame rates.
- `Hortensia-Windows.zip` is compiled with Clang and PGO for standard x86-64 processors.
- `Hortensia-Windows-MSVC.zip` is compiled with Microsoft Visual C++ without PGO. Use it as a fallback
  if a MinGW/Clang build fails to start on your specific setup.

</details>

## Setting it up

1. **Extract** the zip into a folder (right-click → *Extract All*), for example `C:\Games\Hortensia`,
   and double-click `hortensia.exe`. If Windows says it protected your PC, click **More info →
   Run anyway** (Hortensia isn't signed).
   - Linux: `chmod +x Hortensia-Linux-x86_64-PGO.AppImage`, then run it.
   - Mac: open the zip, drag Hortensia to Applications, right-click it → **Open** the first time.
2. **First start.** Hortensia does a few things for you:
   - If you used **Lichii**, **SmileyTatsu**, or **Eden/Eglantine** before, it offers to **import** your keys,
     firmware, saves, settings and **shader caches** (no stutter in games you already played).
   - It picks your **dedicated graphics card** over the processor's integrated graphics.
   - It asks about **optimized settings**: answer **Yes** (see below).
   - It switches to the **Hortensia theme** (Emulation → Configure → General → UI to change it).
3. **Keys and firmware** (skip this if you imported them). Hortensia keeps its own data folder, so
   it never touches the config of your other emulators:
   - Windows: `%APPDATA%\hortensia` (type it in the Explorer address bar)
   - Linux: `~/.local/share/hortensia` and `~/.config/hortensia`
   - macOS: `~/Library/Application Support/hortensia`

   Copy your `keys` folder there and install your firmware from the **File** menu, exactly as you
   did for Eglantine, Lichii or Eden.
   **Portable mode:** create an empty folder named `user` next to `hortensia.exe` and Hortensia keeps
   everything in it instead.
4. **Add your games folder** (double-click the game list). Each game shows **Switch 1** or
   **Switch 2** under its name. The first launch of a game builds its shaders: it stutters for a
   few minutes, then gets smooth.

### Using someone else's shader cache

Hortensia reads standard cache files (`vulkan.fork-v53.bin`). Put a game's cache folder (named after
its program ID, for example `0400486024dec000` for Ocarina of Time) in
`%APPDATA%\hortensia\shader\` on Windows. Only the `vulkan.fork-v53.bin` and
`vulkan_used.fork-v53.bin` files matter; files from other builds are ignored. The guest shader list
works on any graphics card; the driver's own cache is rebuilt for yours.

## Optimized settings per game

At its first start, Hortensia prompts you to select your preferred interface language (**Français**, **English**, or **System**), and asks whether it should **set the best settings for each game automatically** (saved to `qt-config.ini` so it does not prompt every run). With the option on (it can be changed any time in **Emulation → Optimized Settings per Game**), starting a supported game applies its settings for that session only: your saved settings are never changed, and the log says what was applied (`Hortensia optimized settings (AMD GPU): …`).

You can also right-click any game in the game list at any time and choose **Apply Recommended Settings** (or **Appliquer les paramètres recommandés**) to instantly apply that title's optimized configuration.

Hortensia reads your **graphics card's vendor** from the selected Vulkan device:

- **NVIDIA and AMD** (dedicated cards): 1x resolution, uncompressed ASTC textures in Mario Kart World.
- **Intel, Apple and Qualcomm** (integrated GPUs): half resolution, BC3-compressed textures.

If a game shows a black screen entering a race or a level with these settings, turn the option
off for that game.

| Game | What is applied |
| --- | --- |
| Mario Kart World | Vulkan, GPU accuracy Fast, asynchronous shaders, reactive flushing off, FIFO Relaxed VSync, handheld mode, Bilinear, FXAA, dynamic FPS off by default (swap interval uncap 1), VRR off by default (`EDEN_NX2_REPORT_VRR=0`), uncompressed ASTC, CPU/DMA Unsafe |
| Ocarina of Time | Vulkan, asynchronous shaders, Max Resolution & Dynamic FPS patches enabled, AMD unoptimized compilation for stall shaders, SSBO alignment sync |
| Donkey Kong Bananza | Vulkan, asynchronous shaders, HMMA compute emulation, geometry shader generic masking, optional 30 FPS lock (`EDEN_NX2_DK_30FPS=1`) |
| Pokémon Pokopia | Vulkan, asynchronous shaders, far shadow cascades enabled by default, accelerated DMA readback flush for photos (`EDEN_NX2_DMA_READBACK_FLUSH=1`), unpaced storage reads |
| Splatoon Raiders | Vulkan, asynchronous shaders, Maxwell TENSOR_MUL_ADD emulation for HMMA compute shaders (`EDEN_NX2_TENSOR=1`) |

## Recommended settings per game

*Measured in TV / Docked Mode on an AMD Radeon RX 6700 XT with latest AMD drivers:*

### General (every game)

| Setting | Value | Why |
| --- | --- | --- |
| Graphics → API | Vulkan | The only backend the Switch 2 path is built for |
| Graphics → Advanced → Enable asynchronous shader compilation | On | Fewer pauses while shaders build |
| CPU → Accuracy | Auto (or Unsafe for more speed) | Unsafe delivers maximum throughput |

### Mario Kart World

| Setting | Value |
| --- | --- |
| Status | **Playable** (~43.4 FPS in-race average, 1% low ~28.5 FPS) |
| VRR Reporting | **Off by default** (`EDEN_NX2_REPORT_VRR=0`) |
| Dynamic FPS | Off by default (uses swap interval uncap 1 for smoothest frame pacing) |
| Resolution | 1x (0.5x on weaker GPUs) |

- **VRR Performance**: On RX 6700 XT in TV mode, `EDEN_NX2_REPORT_VRR=0` delivers a smooth **43.0–43.4 FPS** average (1% low ~28.5 FPS). Enabling VRR (`EDEN_NX2_REPORT_VRR=1`) drops performance to 38.9 FPS with occasional pacing rollbacks; it is kept behind a switch for experimentation.
- Uncompressed ASTC texture decode eliminates dark terrain artifacts.
- Cup intro movies play smoothly unpaced (`EDEN_MKW_MOVIE_UNPACED=1`).

### The Legend of Zelda: Ocarina of Time

| Setting | Value |
| --- | --- |
| Status | **Testing** (fixes included, undergoing hardware verification) |
| Add-Ons → **Max Resolution (built-in)** | On (default) |
| Add-Ons → **Dynamic FPS (built-in)** | On (default) |
| HLE Services | Full Switch 2 implementation (`IGraphicsScaleController` 1050, `vi2` 201, `IGeneralService` 101) |
| AMD Pipeline Stalls | Bypassed via unoptimized compilation (`VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT`) |

- Fixes the 17-second boot panic (`0xf001`) by supplying missing Switch 2 services.
- Eliminates 4–19 second driver compiler stalls on AMD (including compute shader `37b01132333ef8df`).
- Dynamic tap gather offsets (PR #433) and SSBO alignment pass eliminate black screen rendering in GPU Fast.

### Donkey Kong Bananza

| Setting | Value |
| --- | --- |
| Status | **Testing** (fixes included, undergoing hardware verification) |
| Graphics → Advanced → **Donkey Kong Bananza: dynamic FPS** | Optional (toggle with **Ctrl+G** for frame generation) |
| Optional 30 FPS Lock | `EDEN_NX2_DK_30FPS=1` in `nx2_switches.txt` for rock-solid 30 FPS pacing |
| Native Compute | Full native HMMA emulation (0x23c) enabled |

- Fixes the blue screen at 57 FPS by enabling native HMMA compute emulation.
- Fixes the crash at 48–90 s after unmapped device read by restricting `handle_program` evaluation to texture descriptors (PokeDinner fix by Franseven).
- Fixes AMD RDNA pipeline crashes by masking unused generic geometry shader outputs (MamamaNanana).

### Pokémon Pokopia & Mario Tennis Fever

- **Pokémon Pokopia**: **Playable** (~58 FPS stable). Storage reads unpaced for fast boot; far shadow cascades preserved by default (fixing the dark sky).
- **Mario Tennis Fever**: **Playable** (~58 FPS stable).

### Splatoon Raiders & Nintendo Switch Sports Resort

- **In Progress**: Not booting or rendering properly in this version. Fixes are under active development for v1.0.

## What's inside, fork by fork

### Base: [SmileyTatsu](https://github.com/SmileyTatsu/Eglantine) `build-20` PGO (SmileyTatsu and contributors)
- High-efficiency instruction dispatch and memory management yielding peak Mario Kart World frame rates.
- PGO-trained Clang builds (`default.profdata`) tuned for Switch 2 workloads.
- ASTC scanned in place and decoded per block; guest RAM released on emulation stop; pipeline caching without eviction.

### [Lichii](https://github.com/zortos293/Lichii) `nx2` (zortos293, DahBard, Duck912, Frizti and contributors)
- Complete Switch 2 HLE services: `IGraphicsScaleController` (1050), `vi2` (201), `IGeneralService` (101), `prepo:u` (10500), `INx2GpuTimeSession`.
- Dynamic tap gather offsets when offsets are dynamic (Duck912, PR #433).
- SSBO misaligned 64/128-bit access splitting and compute-to-graphics memory barriers (`73bcdc059d`, `a4e42548c4`).
- Unoptimized pipeline compilation for AMD compiler stall shaders (`pipeline_optimization.h`).
- "perf9" GPU optimizations: lockless `MustFlushRegion` for clean pages (PR #413), usage tracker range subtraction optimizations (PR #412).

### [PokeDinner](https://git.eden-emu.dev/Franseven/PokeDinner-a-Eglantine-Fork.git) (Franseven)
- Restricted `handle_program` evaluation strictly to `Shader::TextureDescriptor`, fixing DK Bananza 48–90 s crash (`55367c4474`).
- Avoided forced full backing of failing tiled sparse images (`23cba79142`).
- Optional `EDEN_NX2_DK_30FPS` lock for solid 30 FPS frame pacing on DK Bananza (`216b5034c6`).

### [MamamaNanana](https://github.com/nauxph/Lichii-AMD.git) (`amd-bananza-rdna`)
- Geometry shader generic attribute masking on AMD Windows (`57ffa55a8c`).
- `has_broken_spirv_position_input` safety flags and PBDMA `Nx2Membar()` pipeline memory barriers.

### [Selevet](https://github.com/sylvieistaken/selevet-eglantine) (Sylvie / notsapinho)
- Semaphore acquires already met in memory skip GPU drain (`ACQUIRE_PEEK`, `ACQUIRE_QUEUED`).
- Coarse timer thread sleeping through long waits (`EDEN_TIMING_COARSE`).
- High process priority by default (`EDEN_NX2_HIGH_PRIORITY=1`).
- Switch 2 storage read pacing bypass (Pokopia boots ~13 s faster).

### [Orchid](https://github.com/MythosOfs/Orchid) (MythosOfs)
- AMD subgroup shuffle fix across lanes for 64-wide hardware.
- Switch 2 graphics services host thread (`EDEN_NX2_GSV_HOST_THREAD=1`).

### [Eglantine-DKB](https://github.com/monteger8/Eglantine-DKB) & [ZendNevs](https://github.com/ZendNevs/Eglantine-ZendNevs)
- Terrain indirect draw fixes, macro DMA drain skips, and in-game frame generation toggle (**Ctrl+G**).

### [chuppatricky](https://github.com/chuppatricky/Lichii)
- AMD color target masking, NaN-safe clamp/min/max, and safe graphics pipeline compilation triggers.

### Hortensia Additions
- Smart per-game settings detection with GPU vendor matching.
- Safe profile loading using pristine SmileyTatsu PGO profiles.
- Integrated migration tools for keys, firmware, and shader caches from previous forks.

## Every switch Hortensia adds

Switches are read from `nx2_switches.txt` next to `hortensia.exe`, or from `%APPDATA%\hortensia\nx2_switches.txt`.
One `NAME=VALUE` per line. Restart Hortensia after editing.

| Switch | Default | Description / Turn it… |
| --- | --- | --- |
| `EDEN_NX2_SPLIT` | `2` | Split GPU methods to worker thread (`=0` off, `=1` partial, `=2` full) |
| `EDEN_NX2_FENCE_BATCH` | `1` | Batch GPU fence syncpoints to reduce host driver overhead (`=0` off) |
| `EDEN_NX2_REPORT_VRR` | `0` (MKW) | VRR display capability reporting (`=1` force on, `=0` off) |
| `EDEN_NX2_DK_30FPS` | `0` | Lock Donkey Kong Bananza to steady 30 FPS swap interval (`=1` on) |
| `EDEN_MKW_MOVIE_UNPACED` | `1` | Play Mario Kart World cup intro movies unpaced (`=0` off) |
| `EDEN_NX2_AMD_SAFE_COMPILE` | `1` | Build slow/stalling shaders unoptimized on AMD drivers (`=0` off) |
| `EDEN_NX2_AMD_SAFE_COMPILE_GRAPHICS` | `0` | Force all graphics pipelines unoptimized on AMD (`=1` on) |
| `EDEN_FPS_LOG` | `1` | Log average, min, max, 1% low FPS every 5 s (`=0` off) |
| `EDEN_NX2_PIN_THREADS` | `1` (6+ cores) | Pin front/back GPU threads to dedicated physical cores (`=0` off) |
| `EDEN_HYBRID_CORES` | `1` | Keep time-critical threads on P-cores of hybrid CPUs (`=0` off) |
| `EDEN_NX2_IDLE_SPIN_US` | `50` | Microsecond spin before parking guest CPU threads (`=0` off) |
| `EDEN_NX2_SYNC_SKIP` | `1` | Skip scanning untouched 4 MiB buffer tracker regions (`=0` off) |
| `EDEN_NX2_DEFER_UPLOADS` | `1` | Read large texture uploads on Vulkan worker thread (`=0` off) |
| `EDEN_NX2_PIPELINE_SKIP` | `1` | Keep CurrentGraphicsPipeline when Maxwell3D regs unchanged (`=0` off) |
| `EDEN_NX2_SHADOW_FAR` | `1` (Pokopia) | Draw Pokémon Pokopia far shadow cascade (`=0` skip for ~17% less GPU work) |
| `EDEN_NX2_DMA_READBACK_FLUSH` | `1` (Pokopia) | Flush accelerated image-to-buffer copies to prevent banded photos (`=0` off) |
| `EDEN_NX2_PIPELINE_PRELOAD` | `1` | Asynchronously pre-build Vulkan pipelines in background at boot (`=0` off) |
| `EDEN_NX2_TENSOR` | `1` | Emulate Ampere HMMA with collective TENSOR_MUL_ADD on Maxwell (`=0` off) |
| `EDEN_NX2_DK_60FPS` | `1` (DK) | Cap Donkey Kong Bananza framerate at 60 FPS in PaceToVblank (`=0` uncapped) |
| `EDEN_NX2_UPSCALE_REPLACE` | `1` (OOT) | Replace OOT title neural upscale with direct surface copy (`=0` off) |
| `EDEN_NX2_OCCLUSION_OFF` | `0` (OOT) | Opt-in patch bypassing z-prepass and occlusion queries (`=1` on) |
| `EDEN_NX2_HIGH_PRIORITY` | `1` | Run emulator process at high priority (`=0` normal) |
| `EDEN_TIMING_COARSE` | `1` | Sleep timer thread during long waits (`=0` precise waits) |
| `EDEN_NX2_GSV_HOST_THREAD` | `1` | Run Switch 2 graphics services on dedicated host thread (`=0` off) |
| `EDEN_NX2_BUFFER_DOWNLOADS` | `0` | Force guest buffer downloads (`=1` on; warning: drops FPS ~40%) |
| `EDEN_NX2_FRAMES_AHEAD` | `0` | Pace frame submission ahead (`=1` or `=2` for frame pacing stability) |

## What was left out, and why

Being honest about the gaps helps when comparing Hortensia with individual experimental branches:

- **Unstable experimental fast-draw branches**: Highly unmaintained branches that break perf9 buffer tracking.
- **Incompatible drain skips**: Drain skips that cause missing geometry or broken query occlusion in Ocarina of Time or Mario Tennis.
- **Old PGO training profiles**: Mixed older-code profile data (`mkw-ramzi-20261106.profdata`) was archived into `tools/pgo/old/` to avoid polluting the pristine SmileyTatsu `eglantine.profdata` profile until refreshed training runs are captured.

## Reporting a problem

Open an [issue on GitHub](https://github.com/justr4mzi/hortensia/issues) and attach:

- The log file `hortensia_log.txt` (via **Help → Open Log Folder**);
- Game title, version, your CPU, GPU, and driver version;
- Exact build name/commit SHA from the window title bar;
- Description of the issue with screenshots or reproduction steps.

## Known issues

- **The Legend of Zelda: Ocarina of Time**: Launch, 249 s movie playback crash, and 60 FPS pacing are fixed; undergoing final gameplay tests.
- **Donkey Kong Bananza**: End-of-session QMD freeze, terrain rendering, and 60 FPS pacing are resolved.
- **Pok?mon Pokopia**: Photo banding fixed with DMA readback flush; black sky resolved by default with far shadow cascade.
- **Controller UI**: Some button prompt icons appear as `?`.

## Building from source

Hortensia builds using standard CMake and Ninja. See the [General Build Guide](docs/Build.md).
Continuous integration and release binaries are generated via [.github/workflows/release-binaries.yml](.github/workflows/release-binaries.yml).

## Credits

Hortensia gathers the hard work of the entire Switch 2 emulation community. Sincere thanks to:
- **SmileyTatsu** for the build-20 PGO base and ASTC decode optimizations;
- **Lichii team** (zortos293, DahBard, Duck912, Kajcsi, Inc0rporate, Shrxmy);
- **PokeDinner** (Franseven) for Bananza memory descriptor and 30 FPS pacing fixes;
- **MamamaNanana** for AMD RDNA geometry and barrier work;
- **Selevet** (Sylvie / notsapinho) for synchronization, timer, and storage optimizations;
- **Orchid** (MythosOfs) for AMD lane shuffle and graphics service host thread;
- **Eglantine-DKB & ZendNevs** (monteger8, adamd, ZendNevs) for DK Bananza terrain fixes;
- **Jonas & Frizti** for OOT performance and buffer pipeline enhancements;
- **chuppatricky** (chuppatricky, Matt Artist, sharkiix) for AMD driver stability work;
- **Eden & yuzu developers** for the foundational emulator architecture.

Hortensia is maintained by [justr4mzi](https://github.com/justr4mzi).

## License

Hortensia is licensed under the GPL-3.0-or-later. Refer to the [LICENSE.txt](LICENSE.txt) file.

*"Nintendo Switch" is a trademark of Nintendo. Hortensia is not affiliated with Nintendo in any way.
This software should not be used to play games you have not legally obtained.*
