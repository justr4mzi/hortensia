# Experimental Wonder dynamic simulation

Branch: `experiment/wonder-dynamic-timestep`.

Enable for a single launch with `EDEN_WONDER_DYNAMIC_FPS=1`. Without it the
existing fixed-FPS patch remains in use. The normal Wonder FPS setting remains
the presentation limit. Only the existing supported Switch 2 program/build is
accepted. `EDEN_WONDER_SPEED_AUDIT=1` additionally enables readback reports.

## Time feedback

The loader replaces the primary and fallback simulation-rate instructions with
private SVCs (`0xf001`, `0xf002`). Each returns W1 without modifying other
registers. The fallback additionally stores its new rate in controller+0x50,
matching the primary path's following STR; returning W1 alone would leave the
source of game-time scaling unchanged. These are data writes, not runtime
executable patches. Disassembly confirms the caller computes `60.0 / Hz` from
controller+0x50 and stores it at main+0x3aac460; a time accumulator consumes it.

Once per present, the controller compares host elapsed time (normalized for the
emulator speed setting) with the actual preceding game step read back from that
shared time-scale field. It maintains at most 50 ms of phase error and corrects
20% of it per sample. Integer Hz is chosen by nearest reciprocal time step;
remaining quantization error is corrected through subsequent actual readback.
Simulation Hz can reach 125% of the presentation target (up to 1000 Hz) so the
controller can also correct overspeed. This does not raise presentation FPS.

Compensation stops at a 30-Hz physics step, or the explicitly lower target.
Below the floor slowdowns remain. Pauses, loading gaps, missing observations,
invalid readbacks and changed targets/speed reset feedback. Unlimited emulation
speed retains the configured rate. No unbounded loading-stall catch-up occurs.

## Audit limits

The audit accumulates the preceding frame's shared scale divided by 60 and
compares it with monotonic host elapsed time. This measures the shared engine
time scale, not every independent level timer, collision result or audio clock.
Only consecutive single-present samples with a positive finite scale and gaps
of at most 250 ms are covered; other elapsed time is reported as uncovered.
Other early returns can still bypass both timing hooks. Start/scene transitions
must not be confused with a controlled gameplay-only speed benchmark.

## Validation

The `[wonder_fps]` tests cover fixed mode, build/opcode validation, both dynamic
hooks, bounded correction, repeated queries, pauses, speed/target changes,
fractional FPS, long jitter traces, deliberately perturbed actual readbacks,
safety-floor recovery and audit coverage. Accumulated simulated time is checked
against elapsed time rather than merely checking requested Hz.

For gameplay validation use the same isolated save/config seed and route for
both binaries. Compare covered game and wall-time deltas after warmup, retain
uncovered time, and check walking, jumping, moving platforms and rhythm events.
The below-40-Hz special path and audio synchronization still require testing.

## Prior prototype evidence

The first implementation used the preceding frame duration without phase
feedback and left the fallback unchanged. Its first readback run measured
43.686819 engine seconds in 45.055711 covered wall seconds (96.962%), plus
1.050037 uncovered seconds. It included startup and scene transitions, so it
was not a controlled gameplay-only benchmark. Five-second windows included
both under- and overshoot; constant full speed was not established.

The initial audit attempt crashed because the physical core's transient debug
interface is cleared before SVC dispatch. The corrected audit uses the process
interface by core index. The legacy local MCP collector still expects
eden_log.txt; actual evidence is in the per-game Orchid log instead.

## Overworld diagnosis and separated 30-FPS audit

Strict max-gap filtering of the completed live run found 13 covered audit
intervals (65.055 seconds) near real time: weighted engine scaling100.0237%.
Mean FPS alone is insufficient because it hides individual long stalls. The
new audit retains separate above/below30 totals and a continuously above30
subset after2 seconds settling; skipped/long frames remain explicitly uncovered.
`EDEN_WONDER_SPEED_CSV=1` writes every unique-present timing readback. Existing
`EDEN_NX2_HITCH_TRACE=1` preserves every native present, including sub30 drops.

A25-second live profile at Pipe-Rock Plateau identified CPU texture unswizzling
and upload during new cache-image insertion as a hotspot; per-process physical
GPU activity was low. This justifies a cache-churn diagnostic, not a speculative
cache-matching change. `EDEN_NX2_TC_LOG=1` already provides the necessary first
creation/deletion/collector/upload counters. Independent reproduction was
blocked by another isolated GPU session. No other session was interrupted.

## Overworld cache-budget fix

The bounded cache probe reproduced eviction/re-upload churn at the saved W1
Pipe-Rock Plateau Badge House node: some600-present windows created over2000
images and uploaded7–9 GiB while the collector was limited to a5-GiB critical
ceiling. Raising the diagnostic limits reduced churn and reached stable windows
with no uploads; a longer run still showed churn when that diagnostic cap filled.

The actual fix lets this Wonder title follow the existing driver-budget policy
when a nonzero native budget is available. The existing pressure margin, allocator
slack reserve, headroom and periodic budget shrink handling remain in force.
Other titles, unknown budgets and smaller devices retain their conservative
limits. No physics-floor or texture-matching semantics change is involved.

The test starter's old NAND path was also corrected: its current save had been
written to the older wonder-loading-fix test profile. That NAND was copied into
the current profile after exit, and the previous copy was retained. The source
test profile remains available as rollback; new test launches use their own NAND.
The authoritative saved Overworld seed is wonder-overworld-current-20261106.

### Paired Overworld result

One diagnostic A/B pair used the same saved Badge House node, scene settings and
immutable save/config/cache seed, with80 seconds selected after the initial map
load in each run. Native-present timing gave70.87 to73.79 mean FPS; individual
frames slower than30 FPS fell from194/5669 (3.42%) to65/5901 (1.10%). The99th
percentile frame gap fell from59.52 ms to35.05 ms. Both runs used the same
lightweight cache/hitch/audit logging; this is one pair, not a universal guarantee.

The final budget-driven run had long zero-upload stretches, but later driver
budget shrink again forced collection and produced hitches. The fix reduces
the demonstrated cache-cap problem; it does not eliminate every Overworld dip.
Both owned runs exited without forced termination. Regression checks passed
13 test cases /12172 assertions.
