# Open issue investigation

Snapshot: 2026-11-07, `e420cf2be` (v0.9.0). Scope follows
[the target-game list](../README.md#project-scope). This is an investigation
ledger, not a list of verified fixes. Issue bodies and all current discussion
comments were retrieved for the initial pass; linked logs, captures and fork
diffs still require individual analysis.

## Game and host reports

Issue links refer to <https://github.com/MythosOfs/Orchid/issues>.

| Issue | Scope and evidence | Next investigation or acceptance check |
| --- | --- | --- |
| [110](https://github.com/MythosOfs/Orchid/issues/110) | Mario Tennis Fever shading on NVIDIA, v0.9.0. Only excerpts; the unmapped read shown occurs during shutdown. | Reproduce gameplay and pause rendering; identify the failing pass before attributing it to the shutdown warning. Full log already requested. |
| [104](https://github.com/MythosOfs/Orchid/issues/104) | Steam Deck input, confirmed Bananza 3.2. No log. | Compare SDL device enumeration, bindings and guest Npad state in desktop and gaming mode. Verify the configured device receives input before changing drivers. |
| [101](https://github.com/MythosOfs/Orchid/issues/101) | Bananza 3.2, frame-generation conflict reported only with PGO in Deck gaming mode; normal build works. | Compare matching PGO/non-PGO binaries with the same Vulkan layers and launch environment. Capture the actual Vulkan initialization failure; no evidence yet establishes an OpenGL fallback cause. |
| [96](https://github.com/MythosOfs/Orchid/issues/96) | Wonder is in scope; Ocarina of Time is outside scope per the discussion. Report predates v0.9.0 Wonder timing changes. | Recheck the specific Wonder level on current main; separate a stable 30 FPS timing mode from slow emulation using frame time and speed measurements. |
| [87](https://github.com/MythosOfs/Orchid/issues/87) | Wonder edition/add-on activation. Earlier log found the title and add-on, which does not prove successful content use. New Bananza slowdown also reported. | Trace add-on visibility and entitlement through the loader and AOC service using the attached log. Keep loose-main and packaged launches separate. Bananza scene/version details remain unspecified. |
| [85](https://github.com/MythosOfs/Orchid/issues/85) | Hyrule Warriors device loss at first sword combat on NVIDIA. A full log was subsequently attached. | Analyze that full log and reproduce the combat transition, then capture the failing GPU work. Pipeline-build timing near the crash does not establish causation. |
| [84](https://github.com/MythosOfs/Orchid/issues/84) | Orbitals is now a target game. Discussion identifies a six-channel Sysmix limitation as an unconfirmed lead for the reported audio issue. | Reproduce the audio issue and verify the six-channel Sysmix hypothesis before implementing a fix. |
| [78](https://github.com/MythosOfs/Orchid/issues/78) | Pokopia black sky on RADV; screenshot and encoded logs supplied. Reporter explicitly rules out manually set environment overrides. | Decode and analyze both logs; isolate sky draw, shader output and render-target contents on RADV. Do not repeat the environment-variable question. |
| [76](https://github.com/MythosOfs/Orchid/issues/76) | Game unidentified; scope remains unknown. Only a shader-warning excerpt. | Game title and full log are missing. Do not infer a target game or crash cause from generic storage-buffer warnings. |
| [58](https://github.com/MythosOfs/Orchid/issues/58) | Pokopia saved photos remain corrupt after restart, while the camera preview is clean. Instrumented AM logs contain no shared-capture requests. Development-build geometry flicker is absent in 0.7.4. | Investigate image/readback and guest encoding independently of the geometry regression. Reproduce multiple photos; compare GPU output with CPU-visible bytes before save encoding. Reuse supplied saves and logs. |
| [55](https://github.com/MythosOfs/Orchid/issues/55) | Star Fox crash persists independently of controller reconnect; separate input loss appears vibration-related with direct Pro Controller driver enabled. | Share crash analysis with #47. Investigate vibration/input separately; driver-disable comparison already requested. |
| [53](https://github.com/MythosOfs/Orchid/issues/53) | Bananza SL501 map corruption reproduced locally on RADV in 0.7.5 and development build 1e105aff. Persists at 1x, docked and handheld. Save loads at the required checkpoint. | Load supplied save and open map immediately. Isolate the erroneous map draw or depth data; 1x resolution is already ruled out as a sufficient workaround. |
| [47](https://github.com/MythosOfs/Orchid/issues/47) | Star Fox tutorial loses Vulkan device at second voice transmission. Low GPU accuracy does not resolve it on RTX 5070 Ti. Nine complete compute programs were requested. | Analyze complete compute programs when available. Individual rejected instructions succeeding in isolation does not prove complete-program correctness or explain device loss. |
| [40](https://github.com/MythosOfs/Orchid/issues/40) | Yoshi second-level black/purple rendering. Captured programs motivated lowering fixes, but 1e105aff still failed and introduced reported model instability. Main has later IMAD.HI/resolve changes. | Test the supplied save on current main, including pause transition. Compare complete captures against 0.7.5 and current lowerer. Do not equate successful lowering with correct rendering. |
| [26](https://github.com/MythosOfs/Orchid/issues/26) | Bananza SL200 crash: reporter explicitly confirms fixed in 0.7.4. | Treat as reporter-confirmed historical fix, not a new code task. Current-main smoke test can check regression; no issue closure performed by this audit. |
| [21](https://github.com/MythosOfs/Orchid/issues/21) | Tennis match-entry device loss on NVIDIA in 0.7.2. README now documents a match-start fix. No confirming retest in discussion. | Compare failing log with the implemented fix and test match entry on current main; do not call this reporter-verified. |
| [5](https://github.com/MythosOfs/Orchid/issues/5) | Hyrule Warriors blur. Earlier investigation found 1600x900 internal target and no sustained rejected shaders. Internal resolution/TAA is a hypothesis. | Inspect the render chain with the reported settings and compare pre/post temporal resolve. Safe resolution scaling needs separate validation; sharpening alone does not establish correctness. |

## Fork scan backlog

The nine open scans contain 1,749 commit references before deduplication.
A scan is candidate discovery, not evidence that every listed commit belongs
in Orchid. Generic fixes may help target games even when first developed for
an unsupported game; title alone cannot decide inclusion.

| Scan | References | Initial disposition |
| --- | ---: | --- |
| [17](https://github.com/MythosOfs/Orchid/issues/17) | 282 | Discussion records several integrated fixes and an address-arbiter change. Occlusion submission candidate still needs query regression and GPU validation. |
| [23](https://github.com/MythosOfs/Orchid/issues/23) | 68 | Uniform carries and MME query flushing recorded as integrated. Descriptor-size candidate requires mapped-boundary tests; remaining entries unreviewed. |
| [34](https://github.com/MythosOfs/Orchid/issues/34) | 126 | Event deadline work recorded in discussion. Windows compiler candidate requires an actual Windows-SDK build; remaining entries unreviewed. |
| [59](https://github.com/MythosOfs/Orchid/issues/59) | 283 | Requires deduplication and source comparison. |
| [82](https://github.com/MythosOfs/Orchid/issues/82) | 278 | Requires deduplication and source comparison. |
| [83](https://github.com/MythosOfs/Orchid/issues/83) | 49 | Requires deduplication and source comparison. |
| [89](https://github.com/MythosOfs/Orchid/issues/89) | 210 | Requires deduplication and source comparison. |
| [94](https://github.com/MythosOfs/Orchid/issues/94) | 189 | Requires deduplication and source comparison. |
| [107](https://github.com/MythosOfs/Orchid/issues/107) | 264 | Contains query-fence candidates repeated across forks. Orchid already tracks pending releases and GPU completion; compare semantics and tests before porting. Remaining entries unreviewed. |

## Work order and verification

Start with reproducible supplied-save failures (#53, #58, #40), keeping the
Pokopia geometry regression separate from saved-photo corruption. Next analyze
full-log device losses (#85, #47/#55, #21) and the newer Tennis shading report.
Keep host input and PGO/layer failures isolated from game graphics changes.
Deduplicate fork candidates by commit and equivalent implementation before
selecting candidates relevant to these failures.

Each production fix needs a failing regression case where feasible, relevant
`orchid-agent-test` checks, the local build from `tools/build-local.sh`, and an
isolated affected-game run with inspected screenshots. Windows/NVIDIA and Deck
reports require matching-environment validation before claiming resolution.
No emulator fix or game validation has been completed by this initial ledger.
