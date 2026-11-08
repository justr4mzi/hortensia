# Hortensia v1.0.0

Hortensia v1.0.0 marks a major milestone as the definitive, production-ready release of the next-generation Nintendo Switch and Switch 2 emulator. Combining the highest-performing foundations of **SmileyTatsu build-20** with the most critical rendering, kernel, and recompiler advances from **Lichii, PokeDinner, notsapinho, DahBard, Orchid, and ZendNevs**, Hortensia v1.0.0 delivers unprecedented compatibility, ultra-fast shader caching, and full brand harmonisation.

---

## What's New in v1.0.0

### 1. Game Repairs & Fixes

- **The Legend of Zelda: Ocarina of Time (NUP / Loose Dump)**:
  - **249s Native Crash Fixed**: Completely resolved native crash at 249.66 s (`Unmapped Device WriteBlock` of 4.1 MB) during movie playback by implementing strict address space boundary guards across `DeviceMemoryManager` (`WalkBlock`, `GetSpan`, `GetPointer`, `ReadBlock`, `WriteBlock`) and fixing out-of-bounds page checks in `SparseLargeVector`.
  - **Launch Crash Resolved**: Fixed native emulator crash on startup (at 3.7 s) by properly scoping `Loader::AppLoader* report_loader` in `MainWindow::BootGame`, preventing dangling pointers and null dereferences when building compatibility sessions.
  - **NUP Display Version & Applet Protection**: Added `ReadDisplayVersion` to `AppLoader_NUP` and guarded `m_applet` against null dereferences in `LooseDumpDisplayVersion`.
  - **Hyrule Grass & Foliage Culling**: Expanded maximum storage buffer bindings per stage (`Info::MAX_SSBOS`) from 32 to 64 with 64-bit mask representations across `BufferCache`, preventing culling drops in dense Hyrule Field and Castle Town areas.
  - **White Flashes Elimination**: Draining GPU split query results prior to macro DMA reads.
  - **Framerate Capped at 60 FPS**: Strictly enforced 60 FPS wall-clock pacing in `PaceToVblank` on high refresh rate displays (144 Hz+), eliminating high-framerate physics speeding.

- **Splatoon Raiders**:
  - **Vertical White Stripes & Maxwell Assertion Fix**: Fully resolved the 744 "Invalid insn 0x5d31..." asserts and vertical white stripes. Ported native Maxwell opcode `TENSOR_MUL_ADD` (`0x5d31...`), Maxwell translator `tensor_mul_add.cpp`, IR emitter, and dedicated SPIR-V collective matrix emitter (`emit_spirv_tensor.cpp`) from Lichii to support Ampere HMMA lowering.
  - **RDNA Black Scene Elimination**: Fixed demote combination traversal depth in `DemoteCombinationPass` (#127) to ensure helper invocations are properly discarded on AMD Radeon RX 6000/7000 GPUs.

- **Pokémon Pokopia**:
  - **Black Sky Fixed**: Restored far shadow cascade draws by default in `vk_rasterizer.cpp` (`EDEN_NX2_SHADOW_FAR=1`, port of notsapinho `84e061ce`). Cascade skipping is now opt-in via `=0` for lower-end hardware.
  - **Banded / Striped Photos Fixed**: Forced synchronization of accelerated DMA image-to-buffer copies to guest memory (`memory_manager.FlushRegion`, port of notsapinho `7dde4f15`), eliminating corruption during CPU JPEG encoding.

- **Donkey Kong Bananza**:
  - **End-of-Session Freeze Resolved**: Fixed channel 1 and 2 deadlock on `CanResumeAcquire` caused by secondary queue QMD submissions (`qmd[4] & 0x100`). Loosened strict QMD version checks, added automatic fallback semaphore releases on any QMD dispatch issue, and handled puller engine method 3 as NOP.
  - **Terrain & Rendering Fixed**: Integrated Franseven PokeDinner 2.5.0 and Eglantine-DKB fixes (handling of `GpuSplit::Run` fence cleanups, deferred draw topology overrides, and macro DMA parameter flushes).
  - **Strict Game Isolation**: Confined `EDEN_NX2_PBFLUSH` and `EDEN_NX2_RESETFIX` strictly to DK Bananza (`0x040074A01BF12000`), protecting parallelism in all other games.
  - **60 FPS Framerate Cap**: Capped DK Bananza at 60 FPS in `PaceToVblank` to prevent pacing instability and high refresh rate acceleration.

- **Mario Kart World**:
  - **Instant Shader Startup**: Re-enabled Vulkan driver pipeline caching (`DriverCachesPipelines`) and ported Selevet build 10 background pipeline preloading (`EDEN_NX2_PIPELINE_PRELOAD=1`). 2,200+ pipelines now reload in seconds on subsequent launches rather than 4 minutes.
  - **Buffer Cache Write Preservation (#397)**: Guarded against GPU memory downloads overwriting newer CPU memory writes.
  - **Handheld Pacing Uncap**: Uncapped to interval 1 for high-framerate gameplay (41–43+ FPS in peloton, up to 60 FPS in front).

---

### 2. Upstream Integrations & Architecture Ports

- **Lichii (zortos293 & contributors)**:
  - `#515`: Full support for mesh shaders outputting points and lines (`OutputPoints`, `OutputLinesEXT`, and fixed `point_size`).
  - `#524`: Build flag `-fno-unswitch-loops` for GCC 13+ in `src/CMakeLists.txt`, preventing miscompilation of Maxwell macro upload loops on Linux.
  - `#397`: Buffer cache CPU-write freshness preservation.
  - `I2F` and scalar conversion enhancements supporting 64-bit integer-to-float conversions with strict RZ rounding.
  - Expanded storage buffer binding limit to 64 (`f57593d805`).

- **notsapinho**:
  - Far shadow cascade restoration (`84e061ce`).
  - Accelerated image-to-buffer DMA flush (`7dde4f15`).

- **Orchid 0.9.0 (MythosOfs)**:
  - Real-time emulation speed indicator in status bar even with multicore enabled (`8ebad36f58`).
  - Implementation of Switch 2 `pl:u` command 2100 (`OpenFont`) and transfer memory lifecycle pinning (`c9950fca40`).
  - Native DLSS active output region cropping inside oversized texture allocations (`28f44295c8`).
  - Vertical inversion presentation flag parsing (`4e48ad3ec1`).

- **PokeDinner (Franseven) & Eglantine-DKB (monteger8 / adamd)**:
  - Safe QMD method execution and native tensor dispatch fallbacks.
  - Command page drain and fence retirement on split back thread.

---

### 3. Complete Brand Harmonisation ("Hortensia")

- All user-facing strings, dialogs, desktop files, AppStream metadata, issue templates, and pull request guidelines are fully rebranded to **Hortensia**.
- Configuration hotkeys seamlessly map legacy `"Exit Eglantine"` to `"Exit Hortensia"` with zero configuration loss.
- Profile migration assistant allows importing keys, firmware, and saves from Eden, Eglantine, Selevet, and Lichii.
- Preserved historical commit credits and internal compiler flags (`EGLANTINE_STREAMLINE`, `eglantine.profdata`).

---

### 4. New Environment Switches (`nx2_switches.txt`)

| Switch | Default | Description |
| --- | --- | --- |
| `EDEN_NX2_PIPELINE_PRELOAD` | `1` | Asynchronously pre-build Vulkan pipelines in background at boot (Selevet port) |
| `EDEN_NX2_DMA_READBACK_FLUSH` | `1` | Flush accelerated image-to-buffer copies to prevent banded photos in Pokopia |
| `EDEN_NX2_SHADOW_FAR` | `1` | Draw Pokopia far shadow cascade (`=0` skip for ~17% less GPU work) |
| `EDEN_NX2_TENSOR` | `1` | Emulate Ampere HMMA with collective TENSOR_MUL_ADD on Maxwell (`=0` off) |
| `EDEN_NX2_DK_60FPS` | `1` | Cap Donkey Kong Bananza framerate at 60 FPS in PaceToVblank (`=0` uncapped) |
| `EDEN_NX2_PBFLUSH` | `1` (DK only) | Drain parameter blocks <= 64 words for DK Bananza terrain indirect draws |
| `EDEN_NX2_RESETFIX` | `1` (DK only) | Guard indirect reset program state for DK Bananza |

---

### 5. Measured Performance (RX 6700 XT, TV Mode)

- **Mario Kart World**: ~28 FPS (dense peloton pack), ~43 FPS (front of race), 1% low ~25 FPS.
- **Donkey Kong Bananza**: Solid 30 FPS (with `EDEN_NX2_DK_30FPS=1`) or capped 60 FPS without pacing jumps.
- **Pokémon Pokopia**: Full 60 FPS, complete far shadows, clean photography.
- **Splatoon Raiders**: Full speed, artifact-free fragment shading without invalid insn asserts.
- **The Legend of Zelda: Ocarina of Time**: Stable 60 FPS, complete foliage and grass rendering.
