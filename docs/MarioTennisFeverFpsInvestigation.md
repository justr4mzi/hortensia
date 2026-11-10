# Mario Tennis Fever dynamic FPS test

Status: dynamic timing implemented; normal GPU gameplay validation is pending.
The requested behavior is a 120-FPS limit with normal game speed when actual
rendering falls to, for example, 90 FPS. A 30-FPS-only build does not meet that
request.

## Implementation

`TennisDynamicFps` samples monotonic host time once per actual VI present,
using `Core::g_nx2_present_count`, as Wonder does. The root task traversal is
not a sufficient clock boundary: loading loops can animate and present without
traversing the root. Sampling that root previously reused stale deltas.

The shared game field at NRO+`17995d4` remains in its canonical target-scaled
representation. The patch normalizes 697 build-checked scalar float loads and
one SIMD broadcast at their point of consumption. Every hook checks the actual
load address; branch aliases load their original data unchanged.

Nine pause snapshots and the relative speed modifier remain canonical. This
avoids restoring a stale dynamic multiplier after a pause, replay or slow-motion
change. The independent UI default also receives the dynamic step. A shared
fractional carry supplies exact multiples of 1/64, because character and ball
consumers otherwise truncate rates such as 90 or 144 FPS and lose time.

Twenty-one timeout decrements, including the raw W24 script loop, consume real
elapsed time. Inclusive counters finish at -1 with borrow; ordinary counters
saturate at zero. The common FrameAdvance counter is changed only for the direct
script-wait caller. Other FrameAdvance(1) calls still yield one frame. Polling
callbacks use their own monotonic counter timestamps, so repeated polls cannot
charge the same frame time repeatedly or hang waiting for a new present.
Counter restarts, process changes, target changes and discontinuities reset their
state. The previous direct-wait SVC's lost W0 writeback is also corrected.

## Key consumers and preserved control reads

Offsets refer to the unpacked, module-ID-checked NRO.

- UI root `11ce260`: independent default at `11ce2c4`; audit at `11ce2c8`.
- Character producer shared-step read `10630b8`; integration reads model+390
  at `106ed28`. Character animation also uses model+390.
- Ball simulator shared-step read `1081b8c`; requested 1/64-frame advance at
  `1081bd8`. Live state is NRO+`3e4b9e0`. Prediction copies intentionally use
  a different mode and must not receive unconditional timestep overrides.
- SIMD position integration: `LD1R {V3.4S},[X8]` at `f179c0`.
- Canonical snapshot reads: `975198`, `9755bc`, `980c00`, `982118`, `9825fc`,
  `982838`, `987380`, `9876f4`, `f949b8`.
- Canonical relative-speed read: `9469b8`, before the *=1.05 write at `9469d0`.

All patch words, including the dynamic read table and counter operations, are
validated before any write. The local unpacked game file accepted the patch.

## Validation and limits

- Local GUI build passed. All23 Tennis/Wonder test cases passed (26,249 assertions).
  Selected-line mutation score85.7%:6 killed,1 survived,15 invalid; report
  `/home/leon/Orchid/orchid-issues/mutation/20261107-145103-1436313`.
- Final executable software-renderer smoke test `tennis-software-probe-xj6xwcsu`
  rendered the main menu and shut down cleanly. Target120, actual UI calls around
  29–32/s, observed input clock0.99996–1.000032 in steady5-second windows.
  This confirms the measured menu input clock, not full GPU gameplay.
- Synthetic tests cover 120-target/90-actual timing, changing actual FPS,
  30/120/144/1000 targets, slow motion, pause, restart, 1/64 carry, timeout
  completion, inclusive borrow, and polling without new presents.
- Software-renderer run `tennis-software-probe-skdgi5r2` (under `/tmp`) reported
  the actual UI input clock near 1.000 while the target was 120 and actual calls
  varied well below that, including about 12–14/s. This is not a GPU performance
  benchmark. Long loading gaps were marked uncovered.
- CPU software rendering reached the practice court, but its multi-second frame
  times exceed the catch-up limit. Its character/ball samples cannot validate
  normal gameplay speed and must not be presented as a passed gameplay check.
- Normal GPU runs were rejected because another Orchid MCP instance owned a GPU
  test. Do not bypass that ownership. A real 120-to-90 gameplay test, movement
  distance comparison, prediction behavior and performance overhead remain open.
- Discontinuities longer than max(250 ms, four target frames) reseed instead of
  advancing the game by an unbounded loading/pause catch-up step. Emulation speed
  overrides retain their meaning; unlimited emulation uses the nominal step.
- Specialized timeline clocks (17999b4 and scene+ea4), unknown indirect consumers
  and per-call damping are not proven fully independent of frame rate.

Logging remains enabled in the launcher. `clock_speed` is a separate observed
input clock, not a claim that every part of the game has been validated.
`ball_flight_requested` measures requested advance; collisions or inactive states
can stop actual substep processing. CSV position columns contain pre-integration
positions for character/live ball and NaN for UI.

This remains an experimental patch with normal GPU gameplay checks incomplete.
The user explicitly requested committing and pushing this current state after
being informed of the pending gameplay validation and the AI regression report.
Publication does not establish that the AI issue has been fixed.

## Status display and AI report

The Tennis status label now reads a separate `TennisSpeedMeter` from actual
character integration observations, falling back to a UI animation stream in
menus. It does not use the FPS target, emulator-clock percentage or the ball's
requested prediction advance. It works without CSV logging, ignores other
objects instead of adding their clocks, averages over half a second and shows
no value for interrupted/stale measurements. Other games retain emulator speed.
A private software-rendered title-screen test displayed Game speed100% at24FPS,
matching its input audit near1.0. Tests cover a real75% character clock overriding
an otherwise100% menu clock, pause, missing data and process generation changes.

The user reports that only the AI visibly swings but misses balls. This may be
a dynamic-patch regression, but is not yet causally confirmed. Prediction mode1
uses fixed nominal future steps; live integration uses dynamic steps. Reachability
path106b660→1071580 and106b7b8–106b7d4 (predicted frames added to ball time) need
further checking. The integer actor+78 update truncates fractional steps at
106a150–106a158, but the known hit gate only checks age>=3; this does not establish
why shots would miss throughout a match. No direct AI use of native3ece3b4 was
found among its35GOT and9direct references. No speculative AI correction is
included in the speed-display build.
