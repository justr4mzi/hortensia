# Local Linux build measurements

Measured on 2026-11-06 on Leon's Ryzen 7 5800X3D, 32 GiB RAM, Linux,
GCC and GNU Binutils 2.47. Source revision: `e6bbb9a345c66d74db002245d02ae7cf984dbb24`
plus the local build-script changes. Other desktop applications remained running.
These are short local samples, not a full clean-build comparison.

## Compilation

Compiled `k_page_table_base.cpp`, `vk_pipeline_cache.cpp` and `src/yuzu/main.cpp`
sequentially, twice each, using their generated compile commands and temporary
object outputs. All other flags were held constant, including `-O2` and the debug
path mapping. Previous-style compilation used `-g` without ccache; local cold
compilation used `-g1` with `CCACHE_RECACHE=true`; warm compilation repeated the
local commands with ccache enabled normally. Thus this isolates debug-info and
cache effects, rather than all differences between complete build configurations.

| Configuration | Total for three files, mean of two runs |
|---|---:|
| Previous-style `-O2 -g`, no compiler cache | 21.87 s |
| Local `-O2 -g1`, forced cache miss | 19.33 s |
| Local `-O2 -g1`, cache hits | 0.050 s |

The uncached compilation sample took **11.6% less time**. Cache hits do not apply
to newly edited content: they help when rebuilding content already compiled with
compatible settings, such as returning to a previous branch.

## Linking

Linked the same complete emulator object/library set to a temporary executable,
twice with each linker, reversing order on the second round. Compilation was not
running concurrently. This isolates linker selection using the local build's objects.

| Linker | Run 1 | Run 2 | Mean |
|---|---:|---:|---:|
| Default GNU ld / BFD | 3.99 s | 2.95 s | 3.47 s |
| Selected faster linker / gold | 3.04 s | 2.33 s | 2.68 s |

Gold took **22.6% less time** in this sample. It emitted GNU property-note warnings
for some objects; both linkers successfully produced executables.

## Complete local script invocation

Measured `./tools/build-local.sh` with eight parallel jobs and the existing build
directory. For rebuild cases, removed only the kernel page-table object, then let
the script rebuild the object, libraries and executable. No source files were edited.

| Scenario | Elapsed wall time |
|---|---:|
| Original script, unchanged inputs, two-run mean | 4.63 s |
| Revised script, unchanged inputs, two-run mean | 0.21 s |
| Revised script, kernel object removed, cache disabled | 12.35 s |
| Revised script, same kernel object removed, cache enabled | 3.44 s |

The script originally configured unconditionally, regenerating timestamps and
triggering unnecessary compilation/linking even with unchanged inputs. It now
reuses its configuration; Ninja automatically regenerates when CMake inputs change.
Explicit CMake options or `--configure-only` still request configuration.

The cached rebuild was **3.6 times faster** than recompiling the same object without
cache, including the remaining archive/link work. This is not a prediction of the
speedup for a fresh edit or a complete clean build. End-to-end rebuild cases were
measured once each; repeat measurements before drawing finer conclusions.

Raw results and measurement scripts for this run are in the ignored
`build-local/benchmark/` directory. They may be removed along with build artifacts.
