# Target-game performance campaign

Started 2026-11-07. Scope is README.md “Project scope”; existing README FPS
claims are historical context, not new measurements. Target: at least 60 native
gameplay FPS with correct graphics and game speed. Frame generation does not
satisfy this target. A guest 30 FPS cap must be distinguished from host slowdown;
unlocking it requires timing and physics validation.

## Acceptance and evidence

Use one GPU workload at a time. Build this checkout with tools/build-local.sh.
Record source SHA, binary SHA256, game identity/update, host CPU/GPU/driver,
immutable save/config/cache seed, resolution, handheld/docked, accuracy, VSync,
speed limit and frame generation. Use the same labelled gameplay checkpoint,
input sequence and measurement window for every comparison.

For each game: inspect screenshots before marking gameplay; warm caches in each
arm; collect at least three baseline runs and three candidate runs, alternating
order. Report median/min/max/spread, native present intervals, p50/p95/p99,
counts above 16.67/33.33/50/100 ms, shader compilation and host CPU/GPU load.
System frame-work CSV inverse means are not display FPS. Keep diagnostic perf,
RenderDoc and verbose traces separate from timed runs. Record sample failures.
Profile the heaviest reproducible scene, rank measured costs, change one cause,
and repeat. Promote only gains beyond run spread that repeat and pass graphics,
regression tests and Tennis menu smoke checks. Do not turn missing measurements
into zeros or use menu frame rates as gameplay results.

If blocked, retain evidence, reason and concrete next action here and move to
the next game. No title is verified at 60 FPS by this initial campaign entry.

## Queue and backlog

| Game | Current campaign state | Next action / blocker |
| --- | --- | --- |
| Hyrule Warriors: Age of Imprisonment | Measured stationary gameplay at 2x; graphics gate blocked by native shader rejection | Backlog: reproduce rejected fragment shader at instruction 0xf0, validate full shader and scene, then independent baseline boots |
| Other Hyrule Warriors title | Unavailable: user confirms only Imprisonment is owned/installed so far | Keep second title in availability backlog; no repeated request for its path |
| Mario Tennis Fever | Main menu, tutorial and a regular singles match reached/measured | Repeat deterministic opponent/route; inspect text rendering and heavy fever/doubles scenes |
| Pokémon Pokopia | First cave gameplay measured; broader validation pending | Verify saved-state reload, then measure outside/heavy scenes and investigate startup service assertion |
| Star Fox | Flight tutorial reached; repeated control customization modal interrupts measurement | Reproduce modal loop and finish tutorial; no device loss reproduced in this run |
| Super Mario Bros. Wonder | Overworld and Parade entry measured with matching built-in 60 FPS patch | Repeat heavy traversal and Wonder effects; full graphics gate pending |
| Yoshi and the Mysterious Book | Full v1.0.2 Wildwoods entry measured; saved chapter-one profile confirmed | Trace compute/QMD errors and validate heavier/second-level graphics before promotion |
| Donkey Kong Bananza | Ingot Isle measured; both diagnostic candidates unpromoted | Frames-ahead series terminated at candidate pre-window image mismatch; revisit with rendering/scene evidence |
| Kirby Air Riders | Device loss reproduced during opening sequence; process exited SIGABRT | Investigate failed GPU work; no valid gameplay FPS result |
| Kirby and the Forgotten Land | Switch 2 + Star-Crossed Worlds edition identified; hub measured | Confirm edition scope versus Switch 1 support ID; test levels, saved progress and error records |

## Initial diagnostic evidence (not a performance result)

Source b2f19333a6, build-local/bin/orchid, build succeeded via prescribed helper
with shared CPM cache. Build log: /tmp/orchid-goal-build.log.
Binary SHA256: 79edc189ab3474cdc50fb415c483958bceb87461a52c0731f25865ecfd98631c.
Host CPU: Ryzen 7 5800X3D; RADV reported by GUI. Full GPU/driver manifest pending.

Hyrule Warriors run: 20261107-162249-e81608bd under
/home/leon/.local/share/codex-orchid-mcp/state/runs/.
Seed: hw-blur-mission-20261106; game version 1.0.3 visible on title.
Settings inherited from seed: 2x resolution, handheld, FSR1, SMAA, speed 100%,
frame generation disabled. These are diagnostic settings, not a 1x baseline.

CPU perf captured 5,180 samples, no lost samples reported, 46,353,124 data bytes.
Artifact directory: perf-1794065006779137249-e29193 inside run. Collection covered
startup/title, not a controlled gameplay window. JIT symbols are unresolved and
addr2line emitted warnings. Individual GPU split wait/back-loop samples are leads,
not established gameplay bottlenecks. Do not optimize from this trace alone.

Harness compatibility backlog: current build writes profile/data/eden/log/orchid_log.txt,
while MCP log/error tools read eden_log.txt. Read the actual log for errors;
empty MCP classification is not a correctness pass. A preliminary test of the
installed older binary (84e5a38a14) is excluded from current-source results.
An exploratory Fast Fusion route was stopped after scope discovery and is excluded.

Mission gameplay reached and movement verified by consecutive screenshots; GUI
momentary readings around 41–43 FPS at 2x are observations, not a measured batch.
Input route: hw-mission-workshop-entry-inputs-v1 (11 steps, current package).
Checkpoint hw-mission-workshop-entry-v1 was recorded, but its immediate
comparison failed; route with required final checkpoint was NOT saved. Repair
the ROI/reference and validate replay before using this route in benchmarks.
Gameplay CPU diagnostic: perf-1794065287719052151-125abc in the same run.

Gameplay profile captured 4,972 samples (147,606,308 bytes); timing is excluded.
Top self-sample entries (diagnostic leads, not causal attribution):

```text
5.09% GPU orchid [.] VideoCommon::GpuSplit::WaitForWriters() - -
4.83% GPU libc.so.6 [.] __sched_yield - -
0.93% GPUFencingThrea libc.so.6 [.] 0x00000000001b1b33 - -
0.86% GPUBack orchid [.] Tegra::MemoryManager::GetSubmappedRangeImpl<true>(unsigned long, unsigned long, boost::container::small_vector<std::pair<std::conditional<true, unsigned long, unsigned long>::type, unsigned long>, 32ul, void, void>&
0.70% CPUCore_0 [JIT] tid 1759683 [.] 0x00007f4383fd0fca - -
0.70% GPUBack libc.so.6 [.] 0x00000000001b1bb3 - -
0.60% GPUBack orchid [.] VideoCommon::GpuSplit::BackLoop(std::stop_token) - -
0.58% GPUBack libc.so.6 [.] 0x00000000001b1b33 - -
0.58% GPUFencingThrea libc.so.6 [.] 0x00000000001b1b29 - -
0.54% GPUFencingThrea libc.so.6 [.] 0x00000000001b1b4a - -
```

Stop returned code 0 without forced termination, but MCP finalized=false and
qt_teardown_seen=false. The log-name mismatch may affect teardown detection;
this run remains ineligible for benchmark acceptance regardless.

## 2026-11-07: Hyrule stationary diagnostic measurement

Run 20261107-163003-2588fe32; same binary/seed and 2x settings as above.
Moved Zelda toward workshop, then left controller idle. Screenshots 0016, 0019
and 0020 show the same playable scene with characters, environment and objective.
This is a light stationary scene, not a heavy combat benchmark.

Three consecutive 30 s windows within ONE boot (not three independent runs):

| Window | Samples | Work mean ms | Work p50 ms | Work p95 ms | Work p99 ms | >33.33 ms | >50 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1257 | 23.847 | 22.498 | 32.886 | 40.973 | 52 | 0 |
| 2 | 1249 | 23.989 | 22.518 | 34.256 | 41.231 | 73 | 1 |
| 3 | 1256 | 23.862 | 22.469 | 33.529 | 41.452 | 66 | 0 |

These are system frame WORK durations, not GPU execution or display intervals.
The separate native service log advances by 600 presents every 14.30–14.40 s
in the final stationary scene (about 41.65–41.96 presents/s). No physical display
FPS claim is made. This does not meet 60 native frames/s. Raw window boundaries,
metrics and stutter counts: stationary-report.json inside the run directory.

Corrected log diagnosis: startup orchid_log.txt is also insufficient! SetGame
in src/common/logging.cpp switches to orchid_HWAOI_04003F40232DE000.txt. That
file contains both the Qt cleanup marker and two shader error records at 285.135 s.
The process returned 0 without force; manual inspection proves clean teardown,
while MCP's finalized=false is a false negative caused by following the wrong log.
The local eden_log.txt -> orchid_log.txt alias alone did not solve this.

Graphics gate: FAILED. Program 0x234f98dd00, stage 4, instruction 0xf0:
“native instruction before exit setup state is dead”, op words
80b 1800780b 40000000 03f1e200 001fc800. Screenshot inspection does not explain
the rejected pass. Do not accept a performance optimization until this is traced.
Next: capture complete failing program and determine whether the stage rejection
affects this scene. Continue other games in the meantime.

Checkpoint hw-workshop-objective-v2 uses the objective text ROI and passes
both immediate comparison (RMSE 0.01605) and end comparison (0.00608), threshold
0.02. Old checkpoint remains invalid. Input replay still needs explicit startup
waits and cutscene-state checks; its elapsed gaps cannot be inferred from tool
wait_s alone. No candidate code or setting was promoted.

## Pokopia setup progress

Current-source run 20261107-163851-fda849c8 reached language selection and
Ditto intro. This is not gameplay and no FPS result is accepted. Existing
legacy data-2 save contains only SYDAT.BIN (~2 KB), so its mere presence
does not establish an available progressed gameplay save.

Setup input route: pokopia-first-setup-v1. Stopped cleanly with finalized=true
and qt_teardown_seen=true after pointing this run's empty eden_log.txt at
orchid_PP_0400A3A01EE4C000.txt. This verifies the per-game log correction
without changing the emulator or shared configuration.
Filesystem seed pokopia-intro-language-set-20261107 saved successfully, ID
8f0bacec6934872de4c69532724938d861ae6b99fce850fe63f2fb36df188b65.
The seed records language/setup progress; it is not a RAM save-state and
must be checked on reboot. Next: advance intro and save gameplay entry.

## 2026-11-07: Pokopia first-cave diagnostic

Run 20261107-164226-b69a3144, source/binary unchanged, 1x handheld. The prior
intro-language seed returns to language selection; it did NOT prove persistent
setup progress. Advanced setup and verified movement in the first cave.
Checkpoint pokopia-cave-walking-v1 passed (RMSE 0.01812). Single 60 s stationary
window before profiling: 3,599 system-frame work samples, mean 16.666 ms, p95
18.532 ms, p99 19.803 ms, maximum 25.667 ms, none over 33.33 ms. These work
statistics are not physical display intervals. Native 600-present log windows
advance in roughly 10 s, consistent with 60/s. This light wall-facing scene is
not representative of the outside world and does not establish full-game success.

Later diagnostic host sampling: about 3.04 CPU core equivalents, RSS 11.23 GB,
device-wide GPU busy 29–31%, VRAM 3.35 GB. Sampling was under CPU profiling;
it does not establish unprofiled utilization or exclusive per-process GPU cost.
Perf recorded data but default report/script generation timed out. A bounded
flat report recovered successfully with no lost samples reported; largest
individual self-sample entries include GPU split back loop (2.55%), sched_yield
(2.05%), split Drain (1.68%), WaitForWriters (1.31%). These are diagnostic CPU
cost leads, not a critical-path proof. Raw artifact: perf-flat-recovered.txt in run.

Errors retained: startup IUserShimScopedObject command 1 assertion, and unmapped
48-byte read near teardown. Neither is silently treated as a graphics pass.
No optimization promoted. Raw results: pokopia-cave-20261107.json.
Input route pokopia-first-cave-exploration-v1 is recorded, not replay-validated.
Seed pokopia-first-cave-tested-20261107 saved after clean stop; only SYDAT.BIN
was visible before stopping, so persistent gameplay progress still needs reboot
verification. A filesystem seed does not resume RAM at the measured scene.

## 2026-11-07: Bananza Ingot Isle baseline diagnostic

Historical dk-leon save copied into stopped isolated run only, original preserved.
Immutable seed bananza-leon-import-20261107:
56e57703f9e5b9669e5622032646f8151c0decdb9fde6eb8cbe1d17617a4c1cd.
Bananza version 3.2.0, DLC 1.0.0, 1x handheld; RX 9070 XT / RADV 26.99.99.
Run 20261107-165157-c3f26d25 uses EDEN_NX2_HITCH_TRACE=1.
Continued File 1, Ingot Isle, moved forward 300 ms; tunnel scene, fur, rock
surfaces, lighting and HUD visible. Checkpoint bananza-ingot-tunnel-v1 passes
(RMSE 0.00891). Route bananza-leon-tunnel-entry-v1 recorded with final checkpoint.

Single 60 s diagnostic window: 2,931 complete native service intervals; mean
20.465 ms, p50 16.705 ms, p95 33.402 ms, p99 33.537 ms, maximum 39.370 ms.
Native service cadence estimate 48.86/s; no physical display FPS claim.
Mean phase costs: external 1.197 ms, fence 3.167 ms, composite 0.003 ms, pace
16.095 ms. Phase statistics exclude zero entries and are not a GPU execution
profile. Raw result: bananza-tunnel-baseline-20261107.json.

Log shows frequent 1/2 swap-interval transitions. Existing EDEN_NX2_SWAP_STABLE
option is the next isolated hypothesis: require 120 unchanged presents before
honouring interval >1. This preserves sustained low-rate requests while testing
whether transient requests produce avoidable pacing. Must validate game speed,
visuals, independent repeated A/B and no-trace runs before any default change.
Startup IUserShimScopedObject assertion is present here too; no shader rejection
was seen in the inspected initial gameplay log. Full gate remains pending.

### Bananza first candidate (not promoted)

Run 20261107-165701-dc3bbdf2 replays the same recorded entry route against the
same immutable seed and binary; only EDEN_NX2_SWAP_STABLE=120 differs. Hitch
trace remains enabled in both. Single 60 s stationary gameplay window: 3,594
complete native intervals; mean 16.692 ms, p50 16.659 ms, p95 18.235 ms, p99
20.309 ms, maximum 30.254 ms; no interval over 33.33 ms. Native cadence estimate
59.91/s. Mean phase times (nonzero): external 2.489 ms, fence 3.168 ms, composite
0.003 ms, pacing 11.790 ms, service 14.201 ms. The large observed change is
consistent with transient two-vblank pacing being responsible for lost throughput.
It is not a proven production gain or full-game 60 FPS result.

Automatic final checkpoint failed at RMSE 0.0376142 > 0.02 and route aborted.
The reference ROI includes animated illuminated rock behind the objective text.
Full screenshot still shows the expected tunnel, DK, fur, lighting, HUD and punch
tutorial, but a single visual inspection does not replace the failed route guard
or independent correctness tests. Raw results: bananza-swapstable-candidate-20261107.json.

Next: select a tighter/stable scene guard based on inspected reference images,
validate replay on both arms, collect >=3 alternating independent runs per arm
and a repeat batch. Validate movement/animation/cutscene duration against wall
time, audio sync, sustained native 30 requests and both handheld/docked scenes.
Then repeat without Hitch trace. Check default-profile regression and Tennis menu
before any production setting/code change. Existing global default remains unchanged.

| Variant | Runs | Native cadence estimate | Gate | Verdict |
| --- | ---: | ---: | --- | --- |
| default swap handling, Hitch trace | 1 | 48.86/s | startup unimplemented service assertion; independent correctness pending | diagnostic baseline |
| swap-stable 120, Hitch trace | 1 | 59.91/s | final automated image mismatch | rejected for promotion; repeat candidate |

## 2026-11-07: Bananza correctness review and next experiment

The swap-stable candidate is DEFERRED FOR TIMING RISK, not just awaiting a looser
image threshold. Source evidence: src/core/game_settings.cpp DonkeyKongBananza
overrides describe game steps following requested swap interval; the built-in
1.1.0 patch in src/core/file_sys/patch_manager.cpp also ties simulation to the
chosen interval. Shortening the service wait while the game requests interval 2
may accelerate simulation. Exact 3.2.0 speed impact remains unmeasured. Higher
service cadence alone therefore cannot qualify it as an optimization. No default
was changed. Six-run repeat attempt was cooperatively interrupted during the
first baseline route; zero accepted batch runs. Cleanup returned 0, unforced.
Artifacts, runner and interrupted report are retained under local MCP state
benchmarks/goal-bananza-repeat-20261107.

Image guard calibration was evaluated separately: same labelled text ROI, new
threshold 0.05. Five known gameplay images have RMSE 0.00883–0.03761, while
title/file-select/confirmation and black negative controls have 0.364–0.407.
Full-frame visual inspection showed matching scene components and animation/
illumination changes. Old failed checkpoint is preserved; new guard is named
bananza-ingot-tunnel-v2-calibrated. Raw calibration: bananza-guard-calibration-20261107.json.
This guard cannot prove render correctness or correct simulation.

Next hypothesis tests only EDEN_NX2_FRAMES_AHEAD=1 against default. It preserves
requested swap cadence and uses the existing previous-frame fence logic with its
GPU-behind safety check. The measured default spent ~3.17 ms trying the current
fence before falling back. Test whether bypassing that failed wait reduces work
and changes the game's own interval choice. Possible stale resources, visual
corruption and unsafe command reuse remain acceptance risks to check.
Three independent runs per arm, alternating order, launched via the local runner
/tmp/orchid-bananza-ahead-repeat/run.py. Current handle and terminal state must
be rechecked before starting another GPU workload. No gain is claimed yet.

## 2026-11-07: Star Fox controls-loop backlog

Run 20261107-170619-1929dde6 reached intro and then the flight tutorial. Ship,
HUD, rings and scenery were visible, and steering moved the ship. After steering,
CUSTOMIZE CONTROLS repeatedly reappeared despite selecting No. Intended 30 s
flight windows therefore contain a modal and are explicitly classified as dialog,
not gameplay. Raw diagnostic: starfox-controls-loop-20261107.json. No device
loss was reproduced; the specific later second-transmission failure is untested.

Compute rejection at 18.620 s: native instruction 0x1c40, op 23c / 2004723c
00000008 000008ff 041fe600, cooperative matrix path unavailable. This is not
itself proof of a broken frame: kepler_compute.cpp catches matrix-multiply
rejection and calls UpscaleInsteadOfNativeProgram. Full program/output and
fallback visual quality still need investigation. nx2 camera/chat stub assertions
and airplane-mode socket errors are also retained.
Route starfox-intro-controls-loop-v1 and clean filesystem seed
starfox-tutorial-controls-loop-20261107 saved. Seed is not RAM state; reboot
progress is unverified. Follow-up: resolve/avoid controls modal, then measure
uninterrupted gameplay and reproduce the reported later failure.

## 2026-11-07: Kirby Forgotten Land edition identified and hub measured

NUP metadata identifies KirbySwitch2.nup as Kirby and the Forgotten Land +
Star-Crossed Worlds (Switch 2), program 04004D300C5AE001; add-on metadata
01004D300C5AF001 carries the edition title. Existing supported-title table lists
only Switch 1 program 01004D300C5AE000, so frontend warns Unsupported Game.
Acknowledged Continue for an isolated exploratory run; support policy unchanged.

Run 20261107-171456-6bbec008 used kirby-native-existing-save-20261106. File 1
loads with 1% progress into Waddle Dee Town. Verified movement, characters, hub
geometry, grass, lighting and HUD. Single idle 30 s hub window: 1,799 native
service intervals, mean 16.66658 ms, consistent with 60/s. Separate system-frame
work mean 16.66588 ms. This is not heavy-level or full-game validation.
Raw metrics and all error records: kirby-forgotten-hub-20261107.json.
Route kirby-forgotten-switch2-hub-v1 includes frontend acknowledgement. Clean
stop confirmed. Next: verify edition scope and test a level/boss with repeated
runs and graphics/error checks.

Kirby hub gate remains pending: 15 retained error records include invalid texture
formats, sampler filters/modes and three matrix-program rejections. Visible
scene components alone do not prove all textures or fallback results correct.

## Availability clarification

2026-11-07: user confirms only Hyrule Warriors: Age of Imprisonment is currently
available (“Hab nur imprisonment bisher”). The other README Hyrule title stays
in the availability backlog; do not keep asking for a nonexistent local package.

## Bananza frames-ahead attempt ended

The six-run attempt is TERMINAL, not still running. Baseline/1 completed scene
guards and clean stop: one 30 s native-cadence diagnostic, 49.206/s. Candidate/1
with only EDEN_NX2_FRAMES_AHEAD=1 failed its route checkpoint before a measurement
window: RMSE 0.070255 > 0.05. Full screenshot shows the expected tunnel and
characters with changed idle pose/illumination, but the automatic guard failed.
No accepted candidate run and no valid A/B batch; neither a gain nor regression
can be concluded from the GUI moment reading. Do not relax the guard again to
force a result. Follow up with a less lighting-dependent scene-identification
method and complete correctness/speed validation, or investigate the fence path
in a different reproducible scene. No emulator defaults changed.

Raw interrupted/failed attempt: bananza-frames-ahead-attempt-20261107.json.
Runner, progress log and terminal report retained in local MCP state
benchmarks/goal-bananza-ahead-repeat-20261107. Both emulator stops were clean.

## Yoshi Wildwoods entry measurement

Package-table identity confirms local Yoshi package 0400245021D2C000, update
131072; GUI version 1.0.2. Initial seed yoshi-full-level-reached-20261106-2032
has a zero-byte Slot00 and loads New Game. It is unsuitable for level replay.
Seed yoshi-chapter-one-saved-20261106-2114 has a 308,611-byte Slot00 and loads
saved Wildwoods progress. Run 20261107-172736-c04ad2a3 reaches the book, selects
Explore and enters the habitat. Yoshi, bird/plant creatures, terrain, foreground
and background are visible; short right input moves Yoshi. No black/purple
frame is visible in these first-area screenshots. This does not validate level 2.

30 s first-area gameplay diagnostic with idle input/camera settling: 1,795
complete native intervals; mean 16.705 ms, p95 18.103 ms, p99 19.292 ms, max
65.546 ms; two intervals exceed 50 ms. Mean phases (nonzero): external 6.188 ms,
fence 2.159 ms, composite 0.008 ms, pace 8.362 ms. Single run, not repeated
acceptance. Raw result: yoshi-wildwoods-20261107.json.

Graphics/error gate pending: 123 retained records across startup/menu/loading/
level/shutdown include invalid ZBC depth entries, native compute rejection at
instruction 0x1c0 (all-zero instruction), QMD dispatch failures, and filesystem/
memory errors. Their presence does not prove the visible level is entirely wrong,
but hidden passes cannot be treated as correct. Route yoshi-wildwoods-entry-v1
saved; clean stop verified. Next: separate menu-only errors from level failures,
inspect full captured programs and validate the reported second-level failure.

## Mario Tennis Fever regular-match diagnostic

GUI version 5.0.0. First run 20261107-173224-0aca9bc0 reaches practice and
executes a ball toss. After ending practice/news and inadvertently entering
Adventure, saved clean seed mtf-after-first-practice-20261107. Reboot run
20261107-173656-4da6f700 reaches main menu without first-run practice, then
Free Play, Singles, Quick Play (7 points), Stadium Court (Grass), standard ball,
personal rackets, Mario versus intermediate CPU Yoshi. CPU was selected with
Random; observed identity is recorded, but route is NOT deterministic A/B input.

30 s measured window contains repeated A input, serves, rallies and point
transitions (score reaches 0–2). Both players, court, crowd, effects and scoreboard
are visible. Native cadence: 1,791 complete intervals; mean 16.739 ms, p95
17.820 ms, p99 22.098 ms, max 50.132 ms; four intervals exceed 33.33 ms, one
exceeds 50 ms. Mean phases (nonzero): external 4.977 ms, fence 5.280 ms,
composite 0.006 ms, pace 6.611 ms. These are instrumented service timings,
not host GPU execution or physical-display FPS, and not a stable-all-scenes claim.

Raw result: mtf-stadium-match-20261107.json. A previous seed's Hitch CSV was
inherited; parser now explicitly selects the single nonempty CSV overlapping
this monotonic window rather than the first file returned by glob. No zero-sample
result is accepted. Two error records: missing avatar image and unmapped 48-byte
read near shutdown. Intro/tutorial text screenshots show possible missing glyphs;
recheck fully settled text and atlas coherence. Main-menu smoke observed, but
full graphics/regression gate remains pending. Input route
mtf-mario-yoshi-stadium-match-v1 and seed mtf-stadium-match-played-20261107 saved.

Package identity/update inventory for available targets is recorded separately
in target-package-inventory-20261107.json. It reads package table fields only;
optional metadata TLVs were not interpreted across container format versions.

## Wonder 60-target diagnostic

Isolated seed wonder-goal60-overworld-20261107 changes only the inherited
per-game FPS target from 120 to 60, retaining original immutable seed unchanged.
Run 20261107-174453-4a021840 logs application of the matching built-in Wonder
60 FPS patch. Rosalina loads saved World 1 progress. Two separate 30 s windows
cover idle Badge House overworld and entry to Piranha Plants on Parade after
short movement/jump. Both contain 1,799 native intervals, mean about 16.669 ms,
no interval above 33.33 ms. This does not cover heavy traversal/Wonder effects.
Raw result: wonder-goal60-20261107.json, including six error records. No new
optimization is promoted. Clean stop verified; route saved as
wonder-goal60-overworld-parade-v1.

## Kirby Air Riders first diagnostic: device loss

Run 20261107-175321-c4229649 starts the rendered opening sequence, with heavy
cold compute-pipeline compilation. At 94.501865 s the actual per-game log
reports Vulkan device loss. Immediately preceding trace records show an
8.0 s queue_present and 8.7 s renderer_composite. These timings describe the
failure vicinity, not its root cause. No controlled gameplay window was reached
and no FPS result is accepted. The control server restarted across interruption
and no longer owns this session; process liveness is checked independently
before any new GPU workload. Preserve complete run/log and shader-cache data.
Next: capture the failing GPU work/programs with validation and distinguish
shader correctness, resource lifetime and driver behavior; keep in backlog while
working on other available targets.

Kirby process audit: original PID 2080287 no longer exists. Session manifest
records return code -6, finalized=false, no forced kill. Cold log contains
400 runtime pipeline builds, maximum 331.8 ms. No orphaned emulator restart
is inferred from the absent control-server session.

## Kirby Air Riders shader capture follow-up

Run 20261107-205405-4f622a0e used the same initial seed and source binary,
with native mapped shader dumps and RADV_DEBUG=hang (driver reports syncshaders
and hang enabled). This is an instrumented diagnostic, excluded from timing.
Shader files and SHA256 inventory are retained in the run and
kirby-air-riders-diagnostic-20261107.json. No failing shader is identified.

The opening ended after the log reached 24.54 s. Stop observed return code 0,
no forced termination, but no frontend teardown marker; no Device loss record
was reproduced. Termination cause is unresolved. This is not a successful
reproduction or a correctness pass. Earlier commentary describing this as a
crash is superseded by the recorded exit status. An initial hang-only setup run
20261107-205342-20a60d93 was intentionally stopped and is excluded.

Further boot was blocked by another MCP instance holding the GPU lock (Bananza
test); no lock bypass or unrelated process termination was attempted. Next:
input-free opening replay with explicit process tracking and shader capture,
then associate any actual GPU fault with submitted work before proposing a fix.

## Bananza guest-sleep fence smoke

Run 20261107-205456-28f2045a completed a single 30 s candidate window
with EDEN_NX2_FENCE_GUEST_SLEEP=1 and Hitch tracing. The experimental
change releases the host wake mutex and sleeps the current guest thread
for approximately 1 ms during the bounded fence probe, instead of blocking
the host on the condition variable. The exact uncommitted source diff is
retained in bananza-fence-guest-smoke-20261107.json. It is not promoted.

Both tunnel reference checks matched; shutdown returned 0, finalized and
unforced, with Qt teardown observed. 1,307 native service intervals: mean
22.954 ms, p50 16.782 ms, p95 33.476 ms, p99 33.748 ms, max 50.196 ms;
43.566 native presents/s. Three intervals exceed 50 ms. This is service
cadence, not GPU execution or physical display FPS. One candidate run
without a paired baseline cannot establish gain or regression.

The run retains an unimplemented IUserShimScopedObject assertion and two
cooperative-matrix shader rejections. Full graphics, simulation speed,
tests and Mario Tennis regression gates remain pending. No default or
release configuration is changed. Keep this hypothesis in the backlog;
next evaluate a stable paired scene before spending on a full repeat batch.

## Bananza fence control smoke

Control run 20261107-205832-4c403b53 completed with
EDEN_NX2_FENCE_GUEST_SLEEP=0 using the same binary, game, seed and settings
identities as the enabled smoke above. The runner's `candidate` arm label
is misleading: the retained environment field explicitly disables guest sleep.
Both calibrated reference ROIs matched; exit status 0, finalized, no forced
termination, and Qt teardown observed. The 30 s window contains 1,123 native
present-service intervals: mean 26.729 ms, p50 33.137 ms, p95 33.639 ms,
p99 33.861 ms, maximum 35.738 ms and no intervals over 50 ms.
The native service cadence is 37.412/s. Raw metadata and errors are retained
in bananza-fence-control-smoke-20261107.json.

This supplies one disabled control for the earlier enabled smoke, not a
repeated comparison. Independent boots and matched ROIs do not establish
equal simulated progress or full rendering correctness. Both runs retain
the IUserShimScopedObject assertion and two cooperative-matrix shader
rejections. The fence hypothesis remains unpromoted and local; next run
at least three alternating measurements per arm, then a repeat batch and
the simulation-speed, graphics, tests and Mario Tennis regression gates.
