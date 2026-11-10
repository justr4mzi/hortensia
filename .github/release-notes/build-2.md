## What's new

### Mario Tennis Fever
- Fixed the boot hang: with firmware fonts installed, the built-in font copies didn't fit into the 17 MiB font shared memory, so the game waited for a font forever. Switch 2 fonts now get their own shared memory (decd588)

### GPU / Shader recompiler
- Fixed the IMAD.WIDE lowering (57840c8)
- Storage images use the surface descriptor buffer (b2815c9)
- Fixes for native arithmetic, command synchronization and depth aliases (8b7fae3)
- Exact 24-bit depth normalization without reciprocal rounding errors (66c7e6b)

### Kernel
- Started threads are read under the scheduler lock and released on finalize (949a921)

**Full changes:** https://github.com/MythosOfs/Orchid/compare/build-1...build-2
