## What's new

### Mario Tennis Fever
- Fixed a device loss on NVIDIA at match start: framebuffers are now sized to the smallest attachment layer count instead of the largest (9cbb6dd)
- Built-in font allocation failures are handled cleanly (10d3274)

### Updater
- Orchid now checks this repository's GitHub releases for updates (Help > Check for Updates), CI builds are named `<branch>-build-<run>`, and the updater shows the commits since your build (#6)

### GPU / Shader recompiler
- Experimental NV cooperative-matrix (tensor) lowering, guarded behind a gate; also fixes a purple rendering bug (#5)
- Legacy program override layouts are preserved; IADD3.X carry outputs are rejected and the override surface buffer is validated (e4f15a0, 49d1964)
- Shader cache versions were bumped (Vulkan 25 / OpenGL 21), so your shader cache will be rebuilt once (ee46389, 10d3274)

### Tests & CI
- Fixed the failing unit tests, including an AVX-512 CLZ bug, and the tests now run in CI (#8)
- New tests for the supported bindless texture modes (7c2633d)

### Misc
- Removed leftover AI-agent files and CODEOWNERS from the repository (62d8525)

**Full changes:** https://github.com/MythosOfs/Orchid/compare/build-2...mario-tennis-fever-build-3
