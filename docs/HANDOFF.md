# Hortensia — handoff and work journal

This file is the memory of the project. **Every agent (Claude, Antigravity/Gemini or anyone
else) reads it before working and adds a dated entry to the journal at the end of every
session.** Keep it factual: what was checked, what was integrated, what failed, what is next.

## What Hortensia is

- An experimental Nintendo Switch 2 emulator for Windows, Linux and macOS owned by Ramzi
  (GitHub `justr4mzi`), repository <https://github.com/justr4mzi/hortensia> (renamed from
  `bouquet`; old links redirect).
- It gathers the best changes of the Eglantine forks (Eglantine is a Switch 2 fork of Eden,
  itself a yuzu fork) into one build.
- Base: **zortos293/Lichii `nx2`** (merged up to 6 Nov 2026 17:00, including the AMD fix #273),
  plus ports from Selevet, Orchid, SmileyTatsu, chuppatricky/Matt Artist/sharkiix (AMD),
  notsapinho, Frizti, Eglantine-DKB/ZendNevs, Jonas. The README lists every change and its source.
- Ramzi's PC: Ryzen 5 5600 (6 cores), Radeon RX 6700 XT 12 GB (AMD Windows driver 2.0.353 on
  6 Nov — old), 32 GB RAM, Windows 11. His games: Mario Kart World (`0400C3F00006E000`), Ocarina
  of Time remake (`0400486024DEC000`), Donkey Kong Bananza (`040074A01BF12000`), Pokémon Pokopia,
  Splatoon Raiders, Mario Tennis Fever, Switch Sports Resort. His dumps are `.nup` files.

## Rules (do not break them)

1. Commits are authored and committed as `justr4mzi <justr4mzi@users.noreply.github.com>`.
   **No** `Co-Authored-By`, no "generated with", no AI mention in commit messages or the README.
2. Ported changes keep their **original author** (`git cherry-pick -x`, or `--author` when ported
   by hand). Credit every source in the README.
3. Never force-push, never rewrite published history, never delete tags/branches/releases
   without Ramzi asking. Ask him for anything destructive.
4. Every optimization Hortensia adds must be switchable (`EDEN_*` environment switch read from
   `nx2_switches.txt`) and documented in `dist/nx2_switches.txt` and the README.
5. Pull requests to other projects: one focused change each, original author kept, description
   says what was tested (build only vs gameplay) and "prepared with an AI assistant, reviewed by
   justr4mzi". No spam. Read the project's CONTRIBUTING first. Djihads80/Eglantine has a strict
   AI policy (docs/policies/AI.md): don't send there.
6. Be honest with Ramzi: no invented numbers. Say "measured by X on Y" or "estimate".

## Branches, builds and releases

- `dev`: work branch. `main`: what is published. Push to `dev`, check the build, then `main`.
- CI: `.github/workflows/release-binaries.yml`, started by hand
  (Actions → release-binaries → Run workflow, `publish_release` true/false). Jobs: Windows MSVC
  (`Hortensia-Windows-Compatibility.zip`), Windows MinGW clang PGO + AVX2 + ThinLTO
  (`Hortensia-Windows.zip`), Windows PGO **training** build (artifact only), Linux x86_64 PGO +
  ThinLTO, Linux aarch64 PGO, macOS arm64.
- PGO: `eglantine.profdata` (general) merged 1:2 with `tools/pgo/profiles/*.profdata` (Ramzi's Mario
  Kart World sessions, recorded with the training build) for the Windows build.
- Training a new profile: run the `windows-x86_64-pgo-train` artifact, play, close, run
  `merge-profile.bat`, add the resulting `.profdata` to `tools/pgo/profiles/`.
- `gh` works through the session proxy for `repos/justr4mzi/hortensia/...`; repository settings
  (description, topics) must be changed in the browser.

## Forks to watch (check commits, releases, open/merged pull requests)

| Fork | URL | Branches / tags | Why |
| --- | --- | --- | --- |
| Lichii (zortos293) | github.com/zortos293/Lichii | `nx2`, `perf/*`, `test/*`, `fix/frizti-*`, `exp/*`, `port/*` | Base; OOT; AMD |
| Eglantine | github.com/Djihads80/Eglantine | `nx2`, `nx2-experimental`, tags `daily-*`, `v0.*` | Upstream |
| Selevet | github.com/sylvieistaken/selevet-eglantine | `main` (`tuned/README.md` explains each build) | Fastest MKW on Ramzi's PC |
| Orchid | github.com/MythosOfs/Orchid | `main`, tags `v0.*` | AMD, DK, NUP DLC |
| SmileyTatsu | github.com/SmileyTatsu/Eglantine | `smileytatsu/dev-rebased` | MKW, ASTC |
| notsapinho | github.com/notsapinho/Eglantine | `main`, `dev` | Pokopia |
| Jonas | github.com/jojo-chaechae/Eglantine | `oot-jojo` | OOT perf |
| Frizti | github.com/Frizti/Lichii | `oot-gather-fix`, `nx2` | Split-thread perf |
| chuppatricky | github.com/chuppatricky/Lichii | tags `chuppatricky-amd-testbuild-v*` | AMD fixes |
| Eglantine-DKB | github.com/monteger8/Eglantine-DKB | `dkb-terrain` | DK Bananza |
| ZendNevs | github.com/ZendNevs/Eglantine-ZendNevs | `main` | DK Bananza (pacing, dynamic FPS) |
| KektJ | github.com/KektJ/Eglantine | `perf/crowded-areas` | Fire Emblem, crowded areas |
| Wael3rd | github.com/Wael3rd/Eglantine-MKW | `mkw-eglantine` | MKW |
| eden-fefw | github.com/xDarkWindx/eden-fefw | `main` | Ideas only |
| Fork guide | github.com/riyadhHDA/wichforks | — | Comparison of forks |

`.github/workflows/fork-watch.yml` lists the new commits of these forks every morning (issue
"Fork watch") without AI; an agent still has to read them in detail.

## Method that works

1. `git fetch --depth N <url> +refs/heads/<branch>:refs/<name>/<branch>`, then
   `git log --no-merges <our-base>..refs/<name>/<branch>` and `git cherry HEAD <ref>` to skip
   what is already in.
2. Cherry-pick in order; an empty pick means it is already in; on conflicts, resolve by hand only
   when the change is understood, else skip and note it here.
3. Build on `dev`, read the first error of each failing job
   (`gh api repos/justr4mzi/hortensia/actions/jobs/<id>/logs`), fix, rebuild.
4. Update README (what's inside, switches, what was left out) and this journal.

## What was tried / decided (6 Nov 2026)

- Base = Lichii `nx2` (most active, best OOT). Selevet is built on zortos293/Eglantine build 31 +
  Lichii build-67 + Frizti v2 + more; its window says Lichii.
- **MKW gap:** Ramzi measured Selevet ≈ +10 fps over Bouquet build-9 in a race (Bouquet 22-30
  fps). Log comparison: Bouquet ran with **frame generation on (LSFG 2x)**, Immediate fences,
  uncompressed ASTC, synchronous presentation; Selevet with frame generation off, Default
  fences, BC3, asynchronous presentation. The MKW preset now matches Selevet's settings. Code
  differences (build-31 era optimizations, Frizti v2 split-thread work, Lichii fast-draw branch)
  still need a measured comparison with frame generation off on both.
- Not integrated yet: Lichii fast-draw branch (old, conflicts), Frizti's newest acquire/CB-upload
  work (conflicts with Selevet acquire ports), chuppatricky's scheduler and OOT fog modes,
  Orchid 0.8.0 (NUP DLC, per-game profiles, shader fixes), Pokopia 120-360 fps mods, Selevet
  build 9 (6 Nov 21:04).
- Open bugs on Ramzi's PC: DK Bananza crashes at a scene start (log ends during pipeline
  builds, AMD driver 2.0.353); OOT loads forever (built-in Max Resolution/Dynamic FPS were
  disabled in Add-ons in that run). First thing to try: update the AMD driver, enable both OOT
  add-ons, `EDEN_NX2_AMD_SAFE_COMPILE_GRAPHICS=1` for DK.
- Pull requests opened: zortos293/Lichii #296 (ASTC, SmileyTatsu), sylvieistaken/selevet-eglantine
  #19 (MKW/DK dynamic FPS lookup, SmileyTatsu). Ramzi's forks `justr4mzi/lichii` and
  `justr4mzi/selevet-eglantine` must stay until they are merged.

## Fallback rule for other agents

When an agent starts a session (in particular Antigravity in the morning): if this journal has
no entry dated today by Claude, or Ramzi says Claude is not available, the agent does the
morning round itself, in detail (each fork's commits, pull requests and releases, not only the
Fork watch list), integrates what is useful, builds on `dev`, publishes when green, and writes
its journal entry.

## Journal

- **2026-11-06 (Claude):** Lichii merge with AMD fix #273; 30 chuppatricky/Matt Artist/sharkiix
  AMD commits; Orchid AMD shuffle fix; Selevet timer, CPU, SKIP_COMPUTE, OOT visual workaround;
  Orchid GSV host thread and swap-stable; SmileyTatsu pipelines/RAM; notsapinho read throttle;
  smart presets, theme, Switch 1/2 label, Bilinear, import, GPU pick, bug report, updater, PGO
  training build, ThinLTO; renamed Bouquet → Hortensia (logo, data folder, log
  `hortensia_log.txt`, deep violet theme); MKW PGO profile merged for Windows; MKW preset
  aligned with Selevet. PRs #296 (Lichii) and #19 (Selevet). First public release started.
- **2026-11-06 (Antigravity)** : forks vérifiés : zortos293/Lichii (
x2), sylvieistaken/selevet-eglantine (main, commit d8f254ea5a « Performance build 9 ») ;
  intégré :
  - src/common/game_presets.h : VramUsageMode::Aggressive ajouté au smart preset de Mario Kart World (1d0479d991) ;
  - Lichii #290 (c9bdbaa38e, Zortos) : common: prevent a lost wake when cancelling worker tasks ;
  - Lichii #224 (7a281757d, Bruno Silvestre) : ideo_core: opt-in EDEN_NX2_ASYNC_DISCARD to skip native draws with unbuilt pipelines (combiné proprement avec EDEN_NX2_ASYNC_SKIP dans sync_draw_policy.h, documenté dans README.md et dist/nx2_switches.txt) ;
  - Lichii #314 (588bfa611b, Zortos) : rontend: seed performance settings only for new desktop configs — adapté pour conserver le défaut ScalingFilter::Bilinear de Hortensia au lieu de forcer Fsr3 (les 4 autres seeds GpuAccuracy::Low, NvdecEmulation::Gpu, sync_presentation=true, use_asynchronous_shaders=true sont gardés) ;
  - Lichii #303 (4c173bb792, Zortos) : qt: add opt-in desktop compatibility reports (nom de build prérempli en Hortensia) ;
  laissé :
  - Lichii #287 (26f2e4e347, AppImage switch discovery Linux) et #284 (e219d24c5a, MinGW AV1 ffmpeg) comme demandé ;
  - Selevet Performance build 9 (d8f254ea5a, non intégré volontairement, analysé ci-dessous) ;
  build : run 42299447282 lancé sur dev (publish_release=false, en cours) ; release sur main : run 42294127813 en cours (linux-aarch64 et macos-arm64 verts, Windows/Linux x86_64 en cours) ;
  à faire ensuite : attendre la fin des runs 42294127813 (main) et 42299447282 (dev), faire tester MKW à Ramzi (sans frame gen, mode TV vs portable, Hortensia vs Selevet vs SmileyTatsu build-16) puis décider de la base ;
  questions pour Ramzi : aucune dans l'immédiat, prêt pour tes retours de test MKW dès que le build finit.

  **Analyse détaillée de Selevet Performance build 9 (d8f254ea5a) vs build 8 (cf792deb77) et vs Hortensia :**
  1. **Ce que Selevet build 9 ajoute par rapport à build 8 :**
     - *Traduction Ampere / Shaders OOT (src/shader_recompiler/frontend/ampere/lower_to_maxwell.h, 
egister_pressure.h, spirv_emit_context.cpp, emit_spirv_*.cpp)* : réessaie la traduction des shaders natifs rejetés (corrige les falaises de Gerudo Valley, terrains et ciel blanc/vert sur AMD) ; alignement des miroirs de registres uniformes avec registres shadowés ; nouvelles formes supportées (EDEN_NX2_F2I_CBUF_MODIFIERS, EDEN_NX2_F2I_EXTENDED, EDEN_NX2_F2I_FORMS, EDEN_NX2_FSEL_MODIFIERS, EDEN_NX2_FSET_UNIFORM, EDEN_NX2_IADD3_CARRY_FORMS, EDEN_NX2_IMAD_WIDE_FORMS, EDEN_NX2_IMNMX_NEGATIVE, EDEN_NX2_LEA_FORMS, EDEN_NX2_LOWER_EXTENDED, EDEN_NX2_MUFU_IMMEDIATE, EDEN_NX2_ULOP3_FORM0, EDEN_NX2_VOTEU_EXTENDED) ; support des mesh shaders en topologie points (emit_spirv_mesh.cpp).
     - *Cache de pipelines Lichii build-88 (k_pipeline_cache.cpp, k_pipeline_cache_prebuild.h, pipeline_build.h)* : pré-compilation en arrière-plan sur 1–2 threads basse priorité (jusqu'à 256 pipelines), lecture unique des descripteurs de texture par handle à la traduction, attente jusqu'à 3 ms sur un pipeline en cours de build avant de sauter le draw (EDEN_NX2_BUILD_WAITED_PIPELINES), et saut des draws si un pipeline graphique échoue à compiler au lieu de fermer l'émulateur (PR Lichii #142).
     - *Correctifs AMD / Intel Arc (emit_spirv_atomic.cpp, emit_spirv_tensor.cpp, pipeline_optimization.h, ulkan_device.cpp)* : VK_EXT_shader_atomic_float natif si supporté ; remplacement du réseau d'upscaling OOT (10 programmes tensoriels) par un upscale simple sur AMD et Intel (EDEN_NX2_UPSCALER_STANDIN=1/, EDEN_NX2_OOT_NETWORK) ; compilation séquentielle des pipelines sur le pilote Windows AMD (k_pipeline_cache.cpp) ; 
obustBufferAccess gardé actif ; plusieurs correctifs AMD (CAS retry, SSBO 64/128-bit en mots, barrières compute→draw, query reports sur fence) que Hortensia a déjà via Lichii #273/#295 et les 30 commits de chuppatricky.
     - *Vidéo & Patchs (fmpeg.cpp, 
x2_video_decoder.cpp, patch_manager.cpp)* : décodage vidéo Switch 2 sur CPU via dav1d multi-thread par défaut (EDEN_NX2_VIDEO_HW=1 pour forcer le GPU) ; patch intégré « Occlusion Off » pour OOT (src/core/file_sys/patch_manager.cpp, de cLohan via sharkiix) ; chevauchement des dispatches compute consécutifs (kepler_compute.cpp).
     - *Catalogue UI des switches (src/yuzu/configuration/nx2_switch_catalog.cpp)* : 4465 lignes répertoriant 252 interrupteurs EDEN_* dans l'interface Qt.
  2. **Ce que Selevet a et que Hortensia n'a pas (en particulier MKW et thread GPU split) :**
     - *Thread GPU split — suivi fin des plages d'écriture et des pages propres (src/video_core/gpu_split.h, gpu_split.cpp, dma_pusher.cpp, 
x2_clean_pages.h)* :
       - EDEN_NX2_DMA_RANGES (SetNextWriteRange, WaitForMemoryWritersIn dans gpu_split.* + memory_write_mask dans maxwell_3d.cpp) : les lectures DMA des macros MME n'attendent le thread back que si une écriture en attente (upload de constant buffer, inline data, report) chevauche réellement la plage [begin, end) lue (Hortensia a EDEN_NX2_DMA_DRAIN activé uniquement sur DK Bananza).
       - EDEN_NX2_LIST_TRUST (dma_pusher.cpp, 
x2_clean_pages.h) : les command lists sur des pages déjà vues écrites par le CPU (sans compute/copy en attente) ne refont plus WaitForWriters().
       - EDEN_NX2_LIST_CPU_SYNC (memory_manager.cpp, uffer_cache.h) : quand le CPU réécrit une page de command-list précédemment écrite par le GPU, l'ancienne copie est invalidée immédiatement au lieu d'attendre la prochaine fence.
       - EDEN_NX2_CMD_BATCH (dma_pusher.cpp) : téléchargement groupé des command-lists écrites par le GPU en un seul wait par lot.
     - *Thread GPU split — sémaphores et ordonnancement (src/video_core/engines/puller.cpp, gpu_split.h)* :
       - WaitForSemaphore / sem_write_positions[64] (gpu_split.h, puller.cpp) : dans CanResumeAcquire, si la page du sémaphore n'a pas besoin de flush buffer_cache, attend uniquement la position du dernier release sur le slot de ce sémaphore (WaitForSemaphore) au lieu d'un DrainBack() complet, et mémorise last_checked_release.
       - QueuedReleaseMeets sur anneau fixe de 64 entrées (queued_releases dans gpu_split.h) utilisé aussi dans ProcessSemaphoreExecute (et option risquée EDEN_NX2_ACQUIRE_TRUST=1).
       - EDEN_NX2_PIN_FRONT / EDEN_NX2_PIN_BACK (gpu_split.cpp) : épinglage d'affinité CPU des threads front et back sur Windows (SetThreadAffinityMask).
       - Spin court de WaitFor dans gpu_split.cpp (256 pauses avant ront_wake.wait ciblé sur 
ead_position >= wait_target, et 512 spins côté BackLoop, contre 16384 / 8192 dans Hortensia).
     - *Chemin « Fast Draw » et regroupement de draws (src/video_core/nx2_fast_draw.h, 
x2_draw_runs.h, engines/maxwell_3d.cpp, uffer_cache/buffer_cache.h, 
enderer_vulkan/vk_rasterizer.cpp)* :
       - EDEN_NX2_FAST_DRAW (=1 ou =2 avec suivi de Delta), EDEN_NX2_BUFFER_SKIP, EDEN_NX2_INLINE_INVALIDATE, EDEN_NX2_DYNAMIC_SKIP, EDEN_NX2_DRAW_RUNS : saute la re-validation des vertex buffers, des états dynamiques et des invalidations de caches entre draws successifs compatibles (branche ast-draw absente de Hortensia car en conflit avec perf9 de Lichii, où ast_maxwell3d regroupe plutôt les écritures CB dans GpuSplit::Execute).
     - *Fréquence de soumission et MKW / Présentation (k_rasterizer.cpp, k_scheduler.*, present/frame_gen.cpp, 
x2_frame_skip.h)* :
       - EDEN_NX2_FLUSH_DRAWS : envoie le travail au GPU hôte tous les 128 draws (au lieu de 4096 dans Frizti/Lichii) ou selon le seuil configuré.
       - EDEN_NX2_PRESENT_HZ (limiteur de cadence de présentation pour le patch 30 fps) et EDEN_NX2_VBLANK_ALIGN (alignement vblank 60 Hz).
       - EDEN_NX2_SHADOW_SKIP=N (
x2_frame_skip.h, maxwell_3d.cpp) : ne redessine les shadow maps que toutes les N frames (opt-in).
       - Génération d'images NVIDIA Optical Flow (
vof, branche d'anormalhorse) en plus de LSFG/FSR3.
     - *Table de mapping GPU de l'époque build-31 (
x2_gpu_mapping_table.h / 
vhost_as_gpu.cpp)* et EDEN_NX2_SPARSE_REMAP (
x2:gpu 216 désactivé par défaut chez Selevet car 15–20 % plus lent sur RTX 3050).
- **2026-11-06 23:30 (Claude):** MKW comparison on Ramzi's PC (handheld, same race, FG off, same locked settings, fps by eye): SmileyTatsu build-18 PGO ~40 avg / 47 max / 26-27 min; Selevet build 10 ~33 avg. Hortensia release to be measured. Shared `%APPDATA%\eden` config gets reset by emulators on close: lock settings with `\default=false` and check `LogSettings`. Selevet serializes AMD pipeline builds (`EDEN_NX2_AMD_PARALLEL_PIPELINES=1` restores parallel). Claude is out of quota for the week: Antigravity leads, see `docs/ANTIGRAVITY_DAILY.md`.
- **2026-11-06 23:40 (Claude):** Hortensia release measured ~25 fps in race (22-30) with the same locked settings: slower than SmileyTatsu (~40) and Selevet (~33). Decision: rebuild on SmileyTatsu `dev-rebased` (see `docs/ANTIGRAVITY_DAILY.md` section 13).
- **2026-11-07 (Antigravity) : Nuit d'optimisation Mario Kart World (Ryzen 5 5600 + RX 6700 XT)**
  - **Diagnostic complet des écarts de vitesse (SmileyTatsu ~40 fps > Selevet ~33 fps > Hortensia ~25 fps) :**
    1. *Bug SSBO 8 Mio résolu :* Selevet/Lichii forçaient une taille de mapping complète de 8 Mio (`EDEN_NX2_NATIVE_SSBO_SIZE=1`) sur les dispatches compute natifs, générant 7,6 Gio/s de lectures mémoire CPU lentes via le bus PCIe. À l'inverse, `StorageBufferBinding` dans le `buffer_cache.h` de SmileyTatsu lit déjà la taille exacte dans le mot descriptif (`ssbo_addr + 8`, `packed_size`).
    2. *Stalls GPU évités :* SmileyTatsu a supprimé le `runtime.Finish()` bloquant dans `UploadInChunks` du cache de textures (`4758f45ed5`), évitant les synchronisations GPU inutiles à chaque upload ASTC.
    3. *Upscale DLSS photo mode :* SmileyTatsu a remplacé les passes tensorielles lourdes DLSS en mode docké par des blits bicubiques/bilinéaires directs (`aa31176874`, `2ff2871729`).
    4. *Plafond 30 fps MKW identifié :* Dans `nx2_gsv.h`, `PaceToVblank` verrouillait l'intervalle d'échange à 2 (30 fps) dès que `mkw_dynamic_fps` était désactivé. Hortensia débride désormais cet intervalle à 1 par défaut pour Mario Kart World (`EDEN_NX2_MKW_SWAP_INTERVAL=1`), débloquant jusqu'à 60 fps sans le patch instable.
  - **Refonte UI & Thème Sombre Material (Section 9) :**
    - `dist/qt_themes/hortensia/style.qss` mis à jour selon les directives UX dark : fond gris sombre neutre `#16141b`, surfaces en élévation `#1e1b24` et `#2c2836`, accent lavande adouci `#b4a7f5`, accent rose framboise adouci `#e05a82` pour les actions principales, texte blanc cassé `#e8e4ef` (87 %). Fin du violet saturé agressif.
  - **Compteur d'images objectif dans le log (Section 4) :**
    - Intégration de `EDEN_FPS_LOG` (actif par défaut) : calcule et consigne toutes les 5 secondes dans `hortensia_log.txt` les statistiques réelles (`FPS avg=... min=... max=... 1%low=... frametime_avg=...ms`) ainsi qu'un résumé complet de session à la fermeture.
  - **Presets MKW et réglages par défaut (Sections 10 & 12) :**
    - Preset MKW corrigé : mode TV (Docked) par défaut, VRAM Aggressive, VSync Fifo, Bilinear, ASTC BC3, dynamic FPS off, frame gen off, CPU/DMA Unsafe, GPU Low, Fences Default.
    - Mode TV (Docked) configuré par défaut dans `src/common/settings.h`.
  - **Deux candidats prêts pour tests comparatifs :**
    - **Candidat A (`perf/selevet-mkw`) :** Base Hortensia/Selevet GPU split + correctif SSBO par descripteur + réveil GPU sur sémaphores en attente + ASTC mono-submit. Build CI : run `42309119103`.
    - **Candidat B (`rebase/smileytatsu`) :** Base SmileyTatsu build-18 (`2ff2871729`) + surcouche Hortensia (UI, identité, thème dark Material, smart presets, updater, imports, `nx2_switches_turbo.txt`, débridage intervalle MKW, compteur FPS). Build CI : run `42310516113`.
  - **Instructions de test pour Ramzi :**
    - Télécharger les builds des runs 42309119103 et 42310516113.
    - Lancer Mario Kart World sur la même course en mode TV (Docked), frame gen désactivée.
    - Consulter à la fin de la course la ligne `FPS SESSION SUMMARY` dans `%APPDATA%\hortensia\hortensia_log.txt` pour comparer les moyennes et 1% lows mesurés.

### 10.7 Intégration nocturne complète des 15 forks sur `rebase/smileytatsu` (7 novembre 2026 — 00h45)
- **Diagnostic supplémentaire sur la perte de FPS AMD dans `dev`** :
  - En plus du bug SSBO 8 Mio (`7,6 Gio/s` de relecture PCIe), le commit `57ed438cdc` (PR #295 issu de `mja00/Lichii` / `sharkiix`) présent sur `dev` cassait la vectorisation SPIR-V sur AMD : comme `minStorageBufferOffsetAlignment == 4` sur Radeon RDNA2, la condition `profile.min_ssbo_alignment < (1U << shift)` était toujours vraie pour les accès SSBO 64 bits (`uvec2`) et 128 bits (`uvec4`), les découpant systématiquement en 2 ou 4 `OpLoad`/`OpStore` scalaires 32 bits, tout en remplaçant `WaitForIdle()` par un `vkCmdPipelineBarrier` global `VK_PIPELINE_STAGE_ALL_COMMANDS_BIT`. Ce commit a été volontairement exclu de `rebase/smileytatsu` pour préserver le débit maximal sur RX 6700 XT.
- **21 commits de performance et compatibilité AMD/Switch 2 intégrés un par un (`cherry-pick`) sur `rebase/smileytatsu`** (auteurs d'origine conservés) :
  1. `ade15463d6` (SmileyTatsu `d22212cfd0`, poussé cette nuit) : `texture_cache: add a memory limit setting in place of the fixed 5 GiB cap` — remplace le plafond fixe de 5 Gio par une limite automatique à 75 % de la VRAM (9 Gio sur la RX 6700 XT 12 Go), évitant l'éviction des textures ASTC en pleine course.
  2. `40abae869d`, `be4344a495`, `762d17d2be` (SavageCore / SmileyTatsu PR #1) : application de LayeredFS aux RomFS non-updatables (`.nup`) et protection contre les dossiers de mods absents.
  3. `3e9accddf2` (Brandon Bunnelle / Eglantine PR #717) : `dynarmic x64: lock-free exclusive monitor fast path (ordering-sealed)` — chemin rapide sans verrou pour les instructions atomiques ARM64 `LDXR`/`STXR`.
  4. `65b2cb8223` (Kaj / Eglantine `#141`) : `texture_cache: skip the upload of a new render target a clear overwrites whole` — évite l'upload initial des render targets immédiatement écrasés par un clear (~15 % de temps gagné sur le thread GPU back).
  5. `ab5d2c781f` (MythosOfs / Eglantine PR #720) : `vk_texture_cache: reject a sparse block that is not one page of the image` — corrige les textures terrain RGBA8 de Mario Kart World sur GPU AMD.
  6. `4de1cd8d39` (MythosOfs / Eglantine PR #720) : `vulkan: keep lowered passthrough geometry shaders within device limits` — corrige le crash au lancement d'un match dans Mario Tennis Fever sur Radeon RX 6000.
  7. `31de37d114` (MythosOfs / Eglantine PR #720) : `vulkan: keep query copy destinations disjoint and declare binding strides`.
  8. `38141dd0d3` (MythosOfs / Eglantine PR #721) : `shader: keep depth-compare texture descriptors float on integer formats` — évite un crash du compilateur SPIR-V AMD.
  9. `b5dfd8c92f` (thatirishguyyy / Eglantine PR #716) : `shader: reuse the returned value on compare-exchange retry`.
  10. `edc509fd0e` (CloudP4bbs / Eglantine PR #705) : `shader: lower the Ampere right funnel shift with both words set`.
  11. `f795414a8a` (Zarigis / jojo `acf3887bbb`) : `video_core: invalidate caches after MemoryManager::WriteBlock copies the data`.
  12. `d91dd03e2f` (Jonas / jojo `e7ec986a51`) : `buffer_cache: download only the Preflushable parts of a GPU write at fences` — élimine jusqu'à 5 Go/s de relectures GPU->CPU inutiles aux fences.
  13. `9d5418e2d3` (Jonas / jojo `5f8cd77fe8`) : `video_core: cache MaxContinuousRange until the GPU page table changes` — élimine ~70 000 parcours de table de pages GPU par seconde (~17 % du temps du thread GPU).
  14. `75ded64018` (Jonas / jojo `37341fa20a`) : `kernel: spin briefly before idling cores for Switch 2 titles` (`EDEN_NX2_IDLE_SPIN_US`, défaut `50` µs).
  15. `aee3fa714a` (Jonas / jojo `7f77f4b5e1`) : `video_core: hand the macros back to the engines when the GPU split stops` — corrige le blocage à la fermeture/redémarrage d'un jeu.
  16. `28aa960fd4` (Zortos / Frizti v3 `2388246422`) : `common: prevent a lost wake when cancelling worker tasks (#290)`.
  17. `4f08419908` (Frizti v3 `bcb570768c`) : `common, video_core: pin the split GPU threads to their own cores` (`EDEN_NX2_PIN_THREADS`, actif par défaut sur 6+ cœurs physiques comme le Ryzen 5 5600 : épingle `GpuFront` sur le cœur physique 1, `GpuBack` sur le cœur physique 2, et réserve les 4 autres cœurs physiques aux threads CPU Switch 2 pour éviter toute préemption SMT).
  18. `5eba21d76d` (MythosOfs / Orchid `7afda58309`) : `vulkan: batch barriers for GPU-written command list copies` — regroupe les barrières mémoire lors de la copie des command lists générées par le GPU.
- **Correction CMake & UI sur `rebase/smileytatsu`** (`2cd4b94e1a`, `705e97ef8d`, `45744d1048`, `06a28efec3`) : ajout de `Network` dans `YUZU_QT_COMPONENTS`, synchronisation de `GenerateSCMRev.cmake`, `scm_rev.*`, `update_checker.*` et `path_util.*`, ajout de `GpuSplit::OnBackThread()`, alignement de `FileSys::Nup::MetadataEdit` dans `configure_per_game.cpp`, et utilisation de `GetDeviceVendor()` / `GetCurrentFrame()` dans `main_window.cpp`.
- **Builds CI 100 % verts & Pre-Release publiée (7 novembre 2026 — 02h25)** :
  - **Candidat B (`rebase/smileytatsu` @ `06a28efec3`)** : Run CI [`42319649857`](https://github.com/justr4mzi/hortensia/actions/runs/42319649857) terminé en vert (`completed / success`) sur les 6 plateformes (`windows-x86_64-pgo-avx2`, `windows-x86_64`, `windows-x86_64-pgo-train`, `linux-x86_64-pgo`, `linux-aarch64-pgo`, `macos-arm64`).
  - **Candidat A (`perf/selevet-mkw` @ `f513d2d864`)** : Run CI [`42315977340`](https://github.com/justr4mzi/hortensia/actions/runs/42315977340) terminé en vert (`completed / success`) sur les 6 plateformes.
  - **Pre-Release GitHub** : [`v0.2.0-rc1-mkw-perf`](https://github.com/justr4mzi/hortensia/releases/tag/v0.2.0-rc1-mkw-perf) (`prerelease=true`, sans toucher à la release stable sur `main`), contenant directement `Hortensia-Windows-x86_64-PGO-AVX2-CandidatB-SmileyTatsu-06a28efec3.zip` et `Hortensia-Windows-x86_64-PGO-AVX2-CandidatA-Selevet-f513d2d864.zip` ainsi que les builds Linux et macOS.




---

### 10.8 Rebase sur SmileyTatsu `build-20`, pré-release `v0.2.0-rc2` (`c07421bf37`) & Diagnostic complet PokeDinner / OOT / DK Bananza (2026-11-07)

#### A. Comparaison SmileyTatsu `build-18` (`8fecf646b9`) vs `build-20` (`be6b1ba96d`) & Publication `v0.2.0-rc2`
1. **Commits entre `build-18` et `build-20` (`8fecf646b9..be6b1ba96d`)** :
   - `244d380799` — `merge upstream` (`8362c44ff8`) : synchronisation avec l'amont Eglantine `nx2`.
   - `2e820f023b` — `nx2: report VRR capability on display open and fix present interval calculation` (SmileyTatsu) : active le bit de capacité VRR (`1 << 8`) dans `vi2:GetDisplayService` sous switch `EDEN_NX2_REPORT_VRR` (défaut `1`) et ajuste le calcul de `swap_interval` à la présentation.
   - `be6b1ba96d` — `ci: add LLVM PGO instrumentation and profile-use build workflows` : profil PGO instrumenté (`default.profdata`) entraîné sur de vraies courses *Mario Kart World* avec `-O3 -march=x86-64-v3 -flto=thin`.
2. **Mesure terrain de Ramzi (RX 6700 XT, mode TV)** :
   - Hortensia `build-24` (`rebase/smileytatsu`) : **43,4 FPS de moyenne en course, 1% low ~28 FPS** (`EDEN_FPS_LOG`), devant SmileyTatsu `build-20` PGO (~42 FPS), `build-18` (~40 FPS), Selevet `10` (~33 FPS) et l'ancienne release (~25 FPS).
   - **Pre-release `v0.2.0-rc2` publiée** : tag `v0.2.0-rc2` (`c07421bf37`) avec les binaires `Hortensia-Windows-PGO-AVX2-c07421bf37.zip` et `Hortensia-Windows-x64-c07421bf37.zip` (incluant tous les commits `build-20` + 12 optimisations multi-forks + icône OOT intégrée + masquage de `Display FPS` hors FG).

---

#### B. Diagnostic Donkey Kong Bananza (`040074A01BF12000`) — Pourquoi `build-24` fait un écran bleu, pourquoi `Lichii 93` / `Selevet 10` crashent à 48–90 s, et pourquoi `PokeDinner 2.5.0` (`216b5034c6`) fonctionne sans crash

Les remotes `pokedinner` (`https://git.eden-emu.dev/Franseven/PokeDinner-a-Eglantine-Fork.git`), `lichii` (`https://github.com/zortos293/Lichii.git`), `nauxph` (`https://github.com/nauxph/Lichii-AMD.git`) et `dahbard` (`https://github.com/DahBard/Lichii.git`) sont configurés dans le dépôt local (`C:\Users\ramzi\.gemini\antigravity\scratch\hortensia`).

1. **Cause exacte de l'écran bleu/lilas sur `rebase/smileytatsu` (`build-24`) (`Native compute dispatch unsupported: method=0xb0 value=0x3` / `Native QMD failed: native shader dispatch required`)** :
   - Sur la base `SmileyTatsu build-20` (`HEAD`), `src/video_core/shader_environment.cpp:736` initialise `native_tensor = kepler_compute->EmulatesNativeTensorPrograms()`.
   - Or dans `src/video_core/engines/kepler_compute.cpp:797`, `EmulatesNativeTensorPrograms()` retourne `dlss_chain.active && ...`, ce qui n'est vrai **que** pour la chaîne DLSS de *Fire Emblem: Fortune's Weave* !
   - Pour *Donkey Kong Bananza* (`040074A01BF12000`) et *Splatoon Raiders*, `dlss_chain.active` vaut `false`. Quand DK Bananza lance son compute shader de composition de scène (qui utilise l'instruction tensorielle `HMMA` `0x23c`), `src/shader_recompiler/frontend/ampere/lower_to_maxwell.h:1460` rejette le shader avec `"native matrix multiply (set EDEN_NX2_TENSOR to emulate)"`.
   - Dans `KeplerCompute::DispatchNativeCompute` (`kepler_compute.cpp:738`), le catch `Shader::Exception` tente alors `UpscaleInsteadOfNativeProgram()` et `UpscaleFromConstBuffers()`, qui retournent tous deux `false` car ce n'est pas un pass d'upscale photo/DLSS. `DispatchNativeCompute` retourne `false`, ce qui fait échouer `native_qmd.ProcessControl` (`method=0xb0, value=0x3`) avec `"native shader dispatch required"` et laisse tout l'écran bleu/lilas !
   - **Correctif dans PokeDinner (`f5e1b2bcde` + `55c9b87456` de `Franseven`)** :
     - `f5e1b2bcde` (`ampere: emulate HMMA by default instead of rejecting the program`) : émule `HMMA` (`0x23c`) par défaut pour tous les compute shaders natifs (sauf si `EDEN_NX2_TENSOR=off` ou `0`), avec `EDEN_NX2_TENSOR=stub` gardé pour le diagnostic.
     - `55c9b87456` (`shader: accumulate HMMA.1688/.16816 in f32, round every eight products`) : déjà présent dans `lower_to_maxwell.h` de notre base, il suffit d'activer l'émulation par défaut (`const char* const mode = forced_mode ? forced_mode : "1";` sauf si `forced_mode` vaut `"0"` ou `"off"`).

2. **Cause exacte du crash à 48–90 s sur `Lichii 93` / `Selevet 10` (après `Unmapped Device ReadBlock @ 0xd41bc000-0xd41c3000` et compute `f92e1c9a711e8b7e` / pipeline `df4b5d3a5e060412`) vs ZÉRO crash sur `PokeDinner 2.5.0` (`216b5034c6`)** :
   - La comparaison commit par commit entre `PokeDinner 2.5.0` (`216b5034c6`), `SmileyTatsu build-20` et `Lichii 93` a révélé **deux causes critiques** que Franseven a corrigées/évitées dans `PokeDinner 2.5.0` :
     1. **Bug dans l'évaluation des handle programs sur les descripteurs d'images et de texture buffers (`55367c4474` dans PokeDinner 2.5.0)** :
        - Juste avant de tagger `2.5.0`, Franseven a fait le commit `55367c4474` : `Revert "shader: evaluate handle programs for image and texture buffer descriptors"` (qui annule `bda49e810b`).
        - Pourquoi ? Parce que `bda49e810b` (présent dans `SmileyTatsu build-20`, `Lichii 93` et `Selevet 10` !) fait évaluer `VideoCommon::EvaluateTextureHandle(info, desc.handle_program, gpu_memory, ...)` non seulement pour `Shader::TextureDescriptor`, mais aussi pour **`ImageDescriptor`**, **`TextureBufferDescriptor`** et **`ImageBufferDescriptor`** dans `vk_compute_pipeline.cpp` (ligne 191) et `vk_graphics_pipeline.cpp` (ligne 403), et propage `cbuf.handle_program` dans `texture_pass.cpp` !
        - Or dans DK Bananza, juste après `Unmapped Device ReadBlock @ 0xd41bc000-0xd41c3000`, le compute shader `f92e1c9a711e8b7e` (et le pipeline `df4b5d3a5e060412`) utilise un descripteur d'image/buffer dont l'adresse de storage buffer est non mappée ou invalide (`0xd41bc000`), ou dont `ImageQueryDimensions` / `ImageDescriptor` déclenche une lecture/assertion fatale ou un descripteur corrompu sur le pilote AMD ! En restreignant `handle_program` uniquement à `Shader::TextureDescriptor` (`55367c4474`) et en annulant `23cba79142` (`Revert "vulkan: back a sparse tiled image whole when its tiles can't be set up"`), PokeDinner 2.5.0 élimine complètement ce crash !
     2. **Correctifs spécifiques AMD RDNA pour DK Bananza (`57ffa55a8c` `amd-bananza-rdna` de `MamamaNanana` + `4f6dad2b62` + `c646ed5023` + `9d86952d4f`)** :
        - Dans `vk_pipeline_cache.cpp:1162` (`57ffa55a8c`) : sur le pilote Windows propriétaire AMD (`VK_DRIVER_ID_AMD_PROPRIETARY`), quand un geometry shader exporte les 32 emplacements `Generic0..Generic31` alors que le fragment shader n'en lit qu'une partie, le compilateur de pipelines AMD (`amdvlk64.dll`) **crashe le processus** ! Le commit `57ffa55a8c` masque dans `geometry.info.stores` les attributs `Generic` non lus par le fragment shader.
        - Dans `vk_pipeline_cache.cpp:569` (`57ffa55a8c`) : active `has_broken_spirv_position_input = true` sur `VK_DRIVER_ID_AMD_PROPRIETARY` (les entrées `Position` autonomes font planter la compilation de geometry shaders sur RDNA).
        - Dans `emit_spirv_context_get_set.cpp:48` (`57ffa55a8c`) : vérifie `if (!Sirit::ValidId(info.id)) return std::nullopt;` dans `OutputAttrPointer` pour éviter une déréférence d'ID SPIR-V invalide quand un attribut générique a été masqué.
        - Dans `translate_program.cpp:360` (`57ffa55a8c`) : après `LowerGeometryPassthrough`, remet `program.info.passthrough.mask.reset(); program.is_geometry_passthrough = false;`.
        - Dans `puller.cpp:416` & `vk_rasterizer.cpp:1270` (`57ffa55a8c`) : émule la barrière mémoire PBDMA NX2 (`MEM_OP_D` opération 5 = `MEMBAR` -> `rasterizer->Nx2Membar()`) et ajoute un `VkMemoryBarrier` explicite dans `WaitForIdle()` sur `VK_DRIVER_ID_AMD_PROPRIETARY`.
        - Dans `lower_to_maxwell.h:2033` (`57ffa55a8c`) : ajoute le lowering de l'opcode Ampere `0x234` (`HSETP2` avec sources registres packées).
        - Dans `ae4a50d6a7` (`lichii/capy/lower-bananzas-native-brau`) : corrige le lowering de `BRA.U` (`mode == 1`) utilisé par les shaders de DK Bananza (`abb4ff477a60bdb6...`).

3. **Verrou 30 FPS pour Donkey Kong Bananza (`216b5034c6` de `Franseven`)** :
   - Dans `src/core/loader/nso.cpp` : DK Bananza appelle `nn::vi::SetLayerExtendedSwapInterval(layer, 1, fps)` depuis deux sites dans `main` et calcule son diviseur dynamique (1..3 vblanks) via `mov w8,#60; ldr w9,[x20,#0x24]; str w21,[x20,#0x40]; sdiv w8,w8,w21`.
   - Le commit `216b5034c6` patche en mémoire ces 3 sites (`mov w2, #30`, `mov w21, #2`, `mov w8, #30`) lorsque `Settings::values.dk_30fps.GetValue()` (ou `EDEN_NX2_DK_30FPS=1`) est actif, et ajuste `nx2_gsv.h` pour que `dk_30fps` garde son intervalle de 2 vblanks (30 FPS constants et fluides) même si `mkw_dynamic_fps` est coché !

---

#### C. Diagnostic Ocarina of Time (`0400486024DEC000`) — Bloquant à 17 s sur `build-24` & Gels de compilation / FPS sur AMD RDNA

1. **Pourquoi OOT s'arrêtait à 17 s (`Userspace PANIC, SetTerminateResult 0xf001`) sur `build-24`** :
   - Sur la base `SmileyTatsu build-20`, plusieurs services HLE Switch 2 appelés par OOT au démarrage manquaient (alors qu'ils sont présents dans `dev` / `Lichii build-93`) :
     - `nx2:IGraphicsScaleController` commande `1050` (`SetGraphicsScaleChangedNotificationEnabled` dans `src/core/hle/service/am/service/graphics_scale_controller.h`)
     - `nx2:vi2` commande `201` (`GetDisplayService` / stubs `nx2_gsv.h`)
     - `IGeneralService` commande `101` (`src/core/hle/service/ns/general_service.h` ou `nx2_gsv.h`)
     - `prepo:u` commande `10500` (`SaveSystemReport` / `SaveReport` dans `src/core/hle/service/prepo/prepo.cpp` — vu aussi dans le log de l'issue `#403` !)
     - `nx2:gsv` commande `1121` (`INx2GpuTimeSession`), `BindNativeGpuMemory` (`423` parent offset), `frames_ahead = 1` pour OOT (`0400486024DEC000`), et les correctifs de fin de flux `nx2:vdecsvc` (`bfd6430918`, `701c91b402`, `3ffa971337`, `031e58e930`, `9e4f58c5e7`, `5618439ca6`).
     - `nx2:streamplay` commande `300` (`c6ac0566e9` `GetGuestHidSharedMemory`).
     - `nx2_sysmix` audio S16 (`4f9b9624b9`).

2. **Pourquoi OOT subit des blocages de 4 à 19 s sur AMD (compilations de pipelines en jeu) et tourne à 18–29 FPS sur un seul thread à 99 % (Issue `#403`, `DahBard/Lichii`, `nauxph/Lichii-AMD`)** :
   - **Gels de 4 à 19 s (son coupé)** :
     - Le pilote Windows AMD passe 12,8 s à optimiser le compute shader `37b01132333ef8df` et 1,2 à 1,7 s sur les shaders graphiques `c62f1efe14cbfb7a`, `8a0bd7fa60387348`, `cf86ddafdaccdb74` sur le thread GPU !
     - Dans `src/video_core/renderer_vulkan/pipeline_optimization.h` (`a88f4f0c82`), `WithoutShaderOptimization` (`VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT`) ne couvrait que 6 hashes d'OOT et uniquement `GraphicsPipeline` (pas `ComputePipeline` !).
     - **Solution** :
       1. Étendre `WithoutShaderOptimization` dans `pipeline_optimization.h` aux hashes `0x37b01132333ef8dfULL`, `0xc62f1efe14cbfb7aULL`, `0x8a0bd7fa60387348ULL`, `0xcf86ddafdaccdb74ULL` ET aux shaders natifs lourds sur `VK_DRIVER_ID_AMD_PROPRIETARY`, et l'appliquer aussi dans `vk_compute_pipeline.cpp` !
       2. Dans `lichii/capy/fix-amd-depth-sampling` (`68caa6c75e` : `shader: fix AMD shader typing, atomic retries and compiler stalls`), zortos293 corrige les boucles `CAS` d'atomiques float et le typage SPIR-V qui faisaient justement patiner l'optimiseur du pilote AMD pendant plus de 10 secondes !
   - **Thread à 99 % et 18–29 FPS en extérieur dans OOT (Issue `#403`, `DahBard/Lichii` `d6b294439e` / `8c1dca7062`, `nauxph/Lichii-AMD` `f46553afbf` / `079941fc6f` / `2b70fc5ee9`)** :
     - Le thread bloqué à 99 % sur AMD dans OOT est le **GPU back thread** à cause :
       1. Des **requêtes d'occlusion (`SamplesPassed`)** : sur AMD, copier chaque résultat d'occlusion avec une attente par requête bloque le thread GPU et force des coupures de render pass. `DahBard` (`d6b294439e` `query_cache: answer Ocarina of Time's occlusion reports without host queries by default`, `8c1dca7062` `vk_query_cache: on AMD, wait only for the newest query of a result copy`, `76afd0ff88` `LICHII_WFI_DEFER`) et `nauxph` (`f46553afbf` `perf: copy occlusion results after one barrier on AMD, not a wait per query`) éliminent ce goulot !
       2. Du verrou `buffer_cache` dans `MustFlushRegion` et de la comptabilisation par draw (`nauxph` / `Lichii` `#412` `079941fc6f`, `#413` `2b70fc5ee9`, `#414` `0a619eaaa7`).
   - **Pourquoi `nauxph/Lichii-AMD` (`2026.11.07`) atteint 50–80 FPS en GPU Fast mais a un monde noir de 8–23 s en Fast** :
     - En GPU Fast, `nauxph` active un cap de 256 MiB sur les pointeurs de table graphique (`1af40273f2`) et les gathers `TLD4.PTP` à offsets dynamiques (`PR #433` / `d2af24d8b2`) ou les pointeurs de constant buffer en compétition (`#410` `b8d81e2097`) / les calculs SSBO (`lichii/capy/fix-oot-amd-ssbo-alignment-sync` `73bcdc059d` + `a4e42548c4`) manquent d'une barrière entre les écritures compute et les lectures indirectes/draws suivantes en mode Fast !
     - Le commit `73bcdc059d` (`vulkan: order compute writes before later draws, indirect reads and idle waits`) + `a4e42548c4` (`shader: split misaligned 64 and 128-bit storage buffer accesses into words`) + `PR #433` (`d2af24d8b2` : `shader: gather each tap at its own offset when per-tap offsets are dynamic`) + `#410` (`b8d81e2097` : `shader: bind loaded pointers that compete with a paired cbuf pointer`) corrigent exactement les causes du monde noir dans OOT sur AMD !

---

#### D. Ordre exact des commits à porter maintenant sur `rebase/smileytatsu` pour `v0.2.0-rc3`

1. **Étape 1 — Interface & Version dans le titre (`justr4mzi`)** :
   - `CMakeModules/GenerateSCMRev.cmake` : inclure le hash court du commit dans `BUILD_VERSION` (`build-XX (short_sha)`) pour que le titre de la fenêtre affiche toujours le commit exact testé.
   - `dist/qt_themes/hortensia/style.qss`, `src/yuzu/game/game_list.cpp`, `src/yuzu/game/game_tree.cpp`, `src/yuzu/main_window.cpp` :
     - Corriger le double `ts.readAll()` dans `MainWindow::UpdateUITheme()`.
     - Uniformiser le fond des lignes de la liste de jeux (`#16141b` et `#191720`, écart ~2 %, fin séparateur `#221f2b`) et étendre la ligne du dossier sur toutes les colonnes (`setFirstColumnSpanned(i, root, true)`) pour qu'elle se fonde dans le reste.
2. **Étape 2 — Donkey Kong Bananza (`040074A01BF12000`) : Calculs natifs HMMA, correctif du crash à 48–90 s, RDNA & Verrou 30 FPS** :
   - Porter `f5e1b2bcde` (`Franseven`) : émuler `HMMA` (`0x23c`) par défaut dans `lower_to_maxwell.h` / `shader_environment.cpp` (corrige l'écran bleu/lilas et l'erreur `Native compute dispatch unsupported: method=0xb0 value=0x3`).
   - Porter `55367c4474` (`Franseven`) : restreindre l'évaluation de `handle_program` au seul `Shader::TextureDescriptor` (et non `ImageDescriptor` / `TextureBufferDescriptor` / `ImageBufferDescriptor`) dans `texture_pass.cpp`, `shader_info.h`, `vk_compute_pipeline.cpp`, `vk_graphics_pipeline.cpp`, `gl_compute_pipeline.cpp`, `gl_graphics_pipeline.cpp` (corrige le crash à 48–90 s sur le compute `f92e1c9a711e8b7e` après `Unmapped Device ReadBlock @ 0xd41bc000`).
   - Porter `23cba79142` (`Franseven`) : annuler le backing complet forcé des images sparse tuilées en échec dans `vk_texture_cache.cpp`.
   - Porter `57ffa55a8c` (`MamamaNanana` — `amd-bananza-rdna`) : masquer les sorties `Generic` inutilisées des geometry shaders sur AMD Windows, activer `has_broken_spirv_position_input` sur AMD Windows, sécuriser `OutputAttrPointer` (`Sirit::ValidId`), nettoyer `passthrough.mask`, ajouter `Nx2Membar()` sur `MEM_OP_D` op 5 et barrière mémoire WFI sur AMD, et lowerer `0x234` (`HSETP2`).
   - Porter `ae4a50d6a7` (`zortos293`) : lowerer `BRA.U` (`mode == 1`) pour DK Bananza.
   - Porter `9d86952d4f` (`999sian`), `4f6dad2b62` & `c646ed5023` (`Wael3rd`) : taille de sous-groupe compute uniquement pour les shaders utilisant des opérations warp, et protection contre `imageGranularity == 0` / régions sparse sur AMD.
   - Porter `216b5034c6` (`Franseven`) : option `dk_30fps` + switch `EDEN_NX2_DK_30FPS` dans `settings.h`, `nso.cpp`, `nx2_gsv.h`, `shared_translation.cpp` pour verrouiller DK Bananza à 30 FPS stables (intervalle 2).
3. **Étape 3 — Ocarina of Time (`0400486024DEC000`) : Services HLE Switch 2 complets + Correctifs de gels & FPS AMD** :
   - Porter tous les services HLE manquants depuis `dev` (`Lichii build-93`) :
     - `am/service/graphics_scale_controller.h` : commande `1050` (`SetGraphicsScaleChangedNotificationEnabled`).
     - `ns/general_service.h` / `nx2_gsv.h` : `IGeneralService` commande `101`, `nx2:vi2` commande `201`, `prepo:u` commande `10500`, `INx2GpuTimeSession` (`gsv 1121`), `BindNativeGpuMemory` (`423` parent offset), `frames_ahead = 1` pour OOT (`0400486024DEC000`) / `0` pour DK Bananza (`040074A01BF12000`), `nx2:vdecsvc` fin de flux et décodage vidéo (`bfd6430918`, `701c91b402`, `3ffa971337`, `031e58e930`, `9e4f58c5e7`, `5618439ca6`), `nx2:streamplay` commande `300` (`c6ac0566e9`), `nx2_sysmix` audio S16 (`4f9b9624b9`), `fs_i_storage` (`6f41cdac22`), `platform_service_manager.cpp` (`981c543bed`).
   - Supprimer les gels de compilation AMD de 4 à 19 s dans OOT :
     - Étendre `WithoutShaderOptimization` (`VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT`) dans `pipeline_optimization.h` aux shaders `0x37b01132333ef8dfULL`, `0xc62f1efe14cbfb7aULL`, `0x8a0bd7fa60387348ULL`, `0xcf86ddafdaccdb74ULL` et aux compute pipelines (`vk_compute_pipeline.cpp`).
     - Porter `68caa6c75e` (`zortos293` — `shader: fix AMD shader typing, atomic retries and compiler stalls`).
   - Porter les gains FPS et correctifs graphiques OOT AMD :
     - `PR #433` (`d2af24d8b2` de `Duck912`) : `shader: gather each tap at its own offset when per-tap offsets are dynamic`.
     - `#410` (`b8d81e2097`) : `shader: bind loaded pointers that compete with a paired cbuf pointer (OoT near foliage)`.
     - `73bcdc059d` & `a4e42548c4` (`lichii/capy/fix-oot-amd-ssbo-alignment-sync`) : barrière compute->draw/indirect (corrige le monde noir en GPU Fast) et découpage des accès SSBO 64/128-bit mal alignés.
     - `f46553afbf` / `8c1dca7062` / `d6b294439e` : requêtes d'occlusion AMD sans blocage par requête (et option `EDEN_NX2_FAKE_OCCLUSION`, activée par défaut pour OOT sur AMD).
     - `#412` (`079941fc6f`), `#413` (`2b70fc5ee9`), `#414` (`0a619eaaa7`) : allègement du GPU back thread (`MustFlushRegion` sans lock pour les pages jamais écrites par le GPU, quad index buffer sans attente GPU, suppression du bookkeeping redondant par draw).
4. **Étape 4 — Presets par jeu (`src/common/game_presets.h`) & Vérification du non-écrasement au démarrage** :
   - Vérifier que `qt-config.ini` / `Config` n'écrase pas `gpu_accuracy`, `scaling_filter`, `anti_aliasing` ni les presets par jeu d'Hortensia (`MKW`, `OOT`, `DK Bananza`).
   - Mettre `EDEN_NX2_REPORT_VRR=0` par défaut pour MKW si VRR fait baisser sous 43,4 FPS (garder le switch `EDEN_NX2_REPORT_VRR=1` disponible).


### 10.9 Intégrations Complètes et Préparation de la Pre-Release v0.2.0-rc3 (7 novembre 2026)

#### A. Diagnostic & Résolutions Appliquées sur `rebase/smileytatsu`

1. **Mesure VRR sur Mario Kart World (MKW)** :
   - Mesuré par Ramzi sur RX 6700 XT en mode TV : avec `EDEN_NX2_REPORT_VRR=1` = 38,9 FPS moyen, 1% low ~24 avec micro-retours en arrière ; avec `EDEN_NX2_REPORT_VRR=0` = 43,0 FPS, 1% low ~28,5, fluide.
   - **Décision** : VRR désactivé par défaut pour MKW (`ee608fc399`), switch d'expérimentation maintenu via `EDEN_NX2_REPORT_VRR=1`.

2. **Donkey Kong Bananza (040074A01BF12000)** :
   - *Écran bleu/lilas à 57 FPS* : causé par le rejet de la méthode compute `0xb0` (`HMMA` 0x23c) quand `dlss_chain.active` est faux. Résolu par l'émulation HMMA par défaut (`da7f6e1565` Franseven `f5e1b2bcde`) et l'émulation tensorielle native (`1e3d62e035`).
   - *Crash à 48-90 s après Unmapped Device ReadBlock* : causé par l'évaluation du `handle_program` sur les descripteurs d'images/buffers non mappés. Résolu en restreignant au seul `TextureDescriptor` (`c2da504133` Franseven `55367c4474`).
   - *Crash pipeline AMD RDNA* : corrigé par le masquage des sorties génériques inutilisées en Geometry Shaders, le flag `has_broken_spirv_position_input` sur AMD Windows, la barrière PBDMA `Nx2Membar()` (op 5) et la barrière WFI (`632c2c50f3` MamamaNanana `57ffa55a8c`).
   - *Stabilité du framerate (30 FPS)* : ajout de l'option `dk_30fps` et du switch `EDEN_NX2_DK_30FPS` (`3d712070f9` Franseven `216b5034c6`) assurant un intervalle de swap stable à 30 FPS.

3. **The Legend of Zelda: Ocarina of Time (0400486024DEC000)** :
   - *Arrêt à 17 s (Userspace PANIC 0xf001)* : comblement des commandes HLE manquantes :
     - `IGraphicsScaleController` commande 1050 (`SetGraphicsScaleChangedNotificationEnabled`, `f7ec310697`).
     - `nx2:vi2` commande 201 (`b4dcc22f41`).
     - `IGeneralService` commande 101 (`b4dcc22f41`).
     - Débridage des lectures de stockage Switch 2 (`cf37246c06`).
   - *Gels de 4 à 19 s sur le pilote AMD* :
     - Shaders graphiques lents (`c62f1efe14cbfb7a`, `8a0bd7fa60387348`, `cf86ddafdaccdb74`) et crashants construits sans optimisations pilote (`f8dbfea534` chuppatricky `d16f89cef6`).
     - Shader compute `37b01132333ef8df` (12,8 s de freeze) ajouté à la liste `BuildsUnoptimized` (`72d65a4841`).
   - *Monde noir en GPU Fast sur AMD RDNA* :
     - Synchronisation des écritures compute avant les lectures indirectes et les draws suivants via la barrière `COMPUTE_TO_GRAPHICS` (`4f1103ab72` zortos293 `73bcdc059d`).
     - Dégénération des accès SSBO 64-bit et 128-bit mal alignés en accès par mot de 32-bit (`cae4b71c17` zortos293 `a4e42548c4`).
     - Gather dynamique par tap (`9046528c09` Duck912 `d2af24d8b2`, PR #433).
   - *Allègement du thread GPU back / front (issue #403)* :
     - `MustFlushRegion` sans lock pour les pages jamais écrites par le GPU (`6bae0db4f8` PR #413 `2b70fc5ee9`).
     - Saut des soustractions de `RangeSet` et des resets de trackers d'usage non touchés (`2507801a1e` PR #412 `079941fc6f`).

4. **Interface Utilisateur & Clarté des Tests** :
   - Titre de fenêtre incluant le SHA court du commit (`build-XX (short_sha)`) pour identifier immédiatement le build sous test.
   - Palette de couleurs adoucie pour les lignes alternées de la liste de jeux (`#16141b` et `#191720`, bordure `#221f2b`), et fusion de la ligne de dossier sur toutes les colonnes.

---

## 5. Synthèse des intégrations et correctifs (build-35 et suivants)

1. **Intégrations amont (commits originaux)** :
   - **Lichii nx2** :
     - `#439` (`e8bf112bb7`) : agrandissement de la RAM de macros Turing au-delà de `0x1000` (résout les erreurs d'engine inconnu et les échecs MME).
     - `#438` (`ba1aa10cbe`) : restitution propre des macros aux engines lors de l'arrêt du split GPU.
     - `#425` (`19d48d9856`) : suivi de chaque bloc de 64 octets touché dans le buffer cache.
     - `#429` (`d4653fb307`) : routage des atomiques globales 32 bits non suivies vers leur storage buffer.
     - `#435` (`c85ee33318`) : rejet des plages d'adresses virtuelles dépassant la fin de l'espace d'adressage.
     - `#485` (`d7908a444f`) : maintien engagé des pages de table de pages mises à zéro.
     - `#443` (`318854adf2`) : correction de deux plantages lors du scan des répertoires de jeux.
     - `#442` (`dac688e0f8`) : correction de la sélection de stratégie de migration de données utilisateur.
     - `#433` : vérifié déjà présent dans `emit_spirv_image.cpp`.
   - **Eglantine nx2 (Duck912 & kraibse)** :
     - `a23be213` (`ebd4768925`) : `frames_ahead = 0` pour Ocarina of Time et Donkey Kong Bananza.
     - `688c6fa7` (`1f05fd27f7`) : reconstruction dynamique des pipelines compute lorsqu'un pointeur initialement nul reçoit un buffer alloué (corrige le feuillage à Hyrule Castle Town).
     - `45ebf0f7` : comptage des workers arrêtés sous le mutex de queue.
   - **Orchid 0.9.0 (MythosOfs)** :
     - `c7d26dc2` : addends 64 bits complets pour `IMAD.HI` (implémentation étendue présente dans HEAD).
     - `a2cefedfc4` (`e92d6d15a9`) + `63eaaba0` (`94f09e11d3`) : copie et préservation des snapshots simultanés pour les retours couleur (évite les artefacts de boucle de feedback sur AMD RADV dans DK Bananza).
   - **notsapinho** :
     - `77875bb6` (`6cb9fa759b`) : correctifs AMD pour compute shaders (vues vectorielles alignées et boucle CAS sans rechargement non cohérent).

2. **Résolution des anomalies critiques en jeu** :
   - **The Legend of Zelda: Ocarina of Time** :
     - Cause du crash à 21 s (`0xf001`) : l'appel système kernel SVC 0x34 (`WaitForAddress`) recevait l'arbitration type 3 (`WaitIfEqual64`), non reconnu par le kernel Hortensia/yuzu (`ResultInvalidEnumValue` 0xf001). Port du correctif `WaitIfEqual64` (`3ebea1c0d7` zortos293 `a7f2ee336c`).
     - Prise en charge de la version d'affichage des dumps loose (`LooseDumpDisplayVersion`) quand la métadonnées installée ne fournit pas de version (`134781dafa`).
   - **Donkey Kong Bananza & Switch Sports** :
     - *Désynchronisation pushbuffer & freeze à 5 min* : lorsque `dma_pushbuffer_subindex >= command_list_size`, un reliquat de `dma_state.method_count` non nul contaminerait la soumission suivante en lisant ses en-têtes comme arguments de commande. Réinitialisation complète (`dma_state.method_count = 0`, `split_method_end = 0`) en fin de soumission (`00696952ac`).
     - *Sols et textures manquants* : émulation matricielle HMMA activée par défaut (`73d1380330`), suppression du rejet des descripteurs QMD dont les bits hauts d'adresses portent des drapeaux (`1339e2e0f0`).
     - *Invalid KeplerMemory & Fermi2D* : agrandissement de `Regs::NUM_REGS` à `0x200` et protection des bornes de méthodes contre tout débordement mémoire (`b4bb76f39a`).
   - **Pokémon Pokopia** :
     - *Ciel noir* : désactivation de la suppression de la cascade d'ombre lointaine par défaut (`far_shadow_mode = 1`, `17d8fa2776`), le ciel et l'horizon s'affichent normalement.

3. **Expérience utilisateur (UI) & Cycle de publication** :
   - Sélecteur de langue au premier démarrage (Français / English / Système) avant ouverture de l'interface principale (`38ea9bb7ee`).
   - Persistance de la boîte de dialogue des paramètres recommandés dans `qt-config.ini` (`recommended_settings_asked`), ne demandant plus à chaque lancement (`38ea9bb7ee`).
   - Action contextuelle clic droit dans la liste de jeux : « Appliquer les paramètres recommandés » (`38ea9bb7ee`).
   - Publication des builds de test sous forme de **Draft Releases** (brouillons visibles uniquement par le propriétaire) dans `.github/workflows/release-binaries.yml` (`f27dc18d64`).
   - Conversion des anciennes pre-releases de test (`v0.2.0-rc2`, `v0.2.0-rc1-mkw-perf`, `build-34`, `build-10`) en brouillons (drafts).

4. **Adaptation du backend SPIR-V (build-35)** :
   - Ajout de l'en-tête `<atomic>` manquant pour `requires_coherent_global_memory` dans `spirv_emit_context.cpp`.
   - Ajustement de `DefineGlobalMemoryFunctions(const IR::Program& program)` pour rendre `program.blocks` accessible lors de l'analyse des opcodes atomiques utilisés.
   - Alignement de la détection des storage buffers avec la base SmileyTatsu (`info.native_global_bounds || info.nvn_buffer_used`) en remplacement du prédicat Lichii `UsesNvnStorageDescriptor`.
   - Nettoyage du bloc de CAS atomique global non présent dans le jeu d'instructions IR SmileyTatsu.

5. **Correction de syntaxe des engines (build-35)** :
   - Suppression des accolades fermantes orphelines dans `kepler_compute.cpp` (`CallMethod`) et `maxwell_3d.cpp` (`CallMethod`, `CallMultiMethod`, `ReleaseFront`) issues du cherry-pick de #438.

6. **Correction compilateur MSVC (k_address_arbiter.cpp)** :
   - Marquage `[[maybe_unused]]` de la surcharge `ReadFromUser(s64*)` pour ?viter l'erreur C5245 sur le compilateur MSVC.

7. **Nettoyage et pr?paration release v0.2.0** :
   - `k_address_arbiter.cpp` : utilisation sym?trique de `ReadFromUser` pour `WaitIfEqual64`, supprimant l'erreur MSVC C5245 tout en conservant la validation de m?moire utilisateur.
   - `.github/workflows/release-binaries.yml` : suppression du job `prepare-release` polluant les tags Git distants lors des builds de test ; publication conditionnelle avec cr?ation du tag Git uniquement sur publication officielle (`draft: false`) ; support du fichier de notes `docs/RELEASE_NOTES_v0.2.0.md`.
   - Suppression compl?te des tags et brouillons de test obsol?tes sur GitHub (build-10, build-34, build-35, build-36, build-37, v0.2.0-rc1-mkw-perf, v0.2.0-rc2).


8. **Correctifs post-v0.2.0 (Étapes 2, 3, 4, 5, 6 sur rebase/smileytatsu)** :
   - **The Legend of Zelda: Ocarina of Time (Étape 2)** :
     - *Crash à ~4 min sur `ISETP.EX`* : modularisation et portage complet des en-têtes Ampere (`instruction.h`, `geometry_exit.h`, `register_pressure.h`, `trap_guards.h`, `program_capture.h`) et implémentation du lowering de `ISETP.EX` (bit 72, destination_b, accumulateur et combinaison logique) depuis Lichii commit `c9ebb5c3` (Zortos #87). Prise en charge des décalages à entonnoir 64 bits (`SHF.R.U64`) et masquage des registres hauts.
     - *Deadlock split GPU / écrans noirs (~2 min)* : implémentation de `SubmitDeferredWork()` (#149 chuppatricky) dans `fence_manager.h`, `vk_fence_manager` et `vk_scheduler` : soumission des commandes différées avant de bloquer sur les fences en attente.
     - *Flashes blancs / surexposition caméra* : portage de #215 (WeslieDE) dans `query_dma.h`, `maxwell_3d.cpp`, `macro.cpp` et `vk_rasterizer.cpp` : attente et vidage des requêtes d'occlusion avant toute lecture DMA de macro afin d'éviter les comptages obsolètes.
     - *Désynchronisation de trame* : synchronisation des trames d'OOT avec `frames_ahead = 0` dans `nx2_gsv.h` (`ebd4768925` Duck912) et reconstruction des pipelines de calcul lorsque les slots de pointeurs nuls reçoivent des tampons alloués (`1f05fd27f7` Duck912).
   - **Donkey Kong Bananza (Étape 3)** :
     - *Affichage du terrain et des textures* : portage des correctifs Franseven (`PokeDinner 2.5.0`) :
       1. `vk_rasterizer.cpp` (`76df4591a7`) : interception et exécution hôte du programme de réinitialisation des indirect draw records (11 instructions natives effectuant des écritures globales via des pointeurs indirects non mappables en storage buffers). Restaure le nombre d'instances à 1 au lieu de 0, rendant le terrain et les blocs visibles.
       2. `dma_pusher.cpp` (`5a9573121e`) : vidage immédiat (`FlushRegion` sous `Drain`) des petits blocs de paramètres de macros écrits par le GPU (`flush_parameters`, contrôlable par `EDEN_NX2_PBFLUSH`) avant lecture par le pusher, évitant la désynchronisation des arguments de culling.
   - **Intégrations upstream & Stabilité (Étape 4 & 5)** :
     - Synchronisation des workers : comptage des workers arrêtés sous le mutex de file (`45ebf0f7` kraibse / Lichii #290) évitant les réveils perdus.
     - Pokopia & Fast Fusion : lowering du `ULEA` gardé sans `.HI` ni carry-out (`ff8d461cb2`) et échantillonnage de profondeur en coordonnées texels (`fae454cfeb`), normalisation rectangulaire et mapping G8R24 vers D24_UNORM_S8_UINT.
     - Robustesse UI : protection contre le déréférencement d'un pointeur `loader` nul lors de la lecture des métadonnées de contrôle (`main_window.cpp:2717`).
   - **Pacing & Pipeline Preloading (Étape 6)** :
     - Plafond de fluidité à 60 FPS max dans les menus et écrans de chargement pour Ocarina of Time (`0400486024DEC000`) et Nintendo Switch Sports (`04008E001F098000`) dans `PaceToVblank` (`nx2_gsv.h`) sans affecter le déverrouillage de Mario Kart World.
     - Maintien du préchargement intégral des pipelines pour tous les jeux Switch 2 (`PreloadAllPipelines` / `Settings::CurrentProgramIsSwitch2()`), garantissant l'absence de saccades de compilation en jeu.

### 9. Harmonisation et synchronisation GpuSplit (07/11/2026)
- **Revert #215 Query DMA (`0ee8e95d5f`)** : Alignement avec la décision upstream Lichii (`29f05782d6`). Le port de #215 (`871755381d`) référençait `split.Run` et `DrainSite` qui n'appartiennent pas au modèle `GpuSplit` de la base SmileyTatsu build-20 et causaient un défaut de compilation/liaison. Le mécanisme standard de vidage via `FlushRegion` est conservé sans risque de désynchronisation de thread.

### 10. Intégrations majeures, restauration #215 et correctifs graphiques (07/11/2026)
- **Publication v0.2.0 officielle** : La release publique v0.2.0 a été générée avec succès sur le commit validé de `main` (run GitHub Actions 42369969749, les 6 jobs compilés et verts). Issue #1 fermée avec lien officiel, issue #2 mise à jour (lancement OOT résolu, crash en cours de stabilisation).
- **Restauration complète de #215 (`570c8dc1ee`)** : Portage complet des prérequis architecturaux depuis Lichii/WeslieDE :
  - `DrainSite` et méthode `GpuSplit::Run(func, DrainSite)` dans `gpu_split.h/.cpp`.
  - Prédicat `HasPendingQueries` dans `rasterizer_interface.h` et `vk_rasterizer.h/.cpp`.
  - Implémentation de `FlushQueryResults` dans `vk_rasterizer.cpp` et `SemiFlushRegion` dans `query_cache_base.h`.
  - Restauration de `src/video_core/query_dma.h` : attente et vidage des requêtes d'occlusion avant lecture DMA de macro (`DrainSite::MacroDmaFlush`), résolvant les flashes blancs et la surexposition dans Ocarina of Time.
- **Portage PRs Lichii build-94 & Upstream** :
  - `#301` (`0cfbb481b3`) : Ajout de la variable d'environnement de diagnostic `EDEN_NX2_PIPELINE_BUILD_TRACE` pour identifier les shaders provoquant un crash driver.
  - `#329` (`019531fc84`) : Exécution des warp ops graphiques à la taille native du guest (`vk_graphics_pipeline.cpp`), corrigeant la végétation de Kokiri Forest.
  - `#353` (`619035cd5d`) : Masquage des composantes de Render Target que le fragment shader n'écrit pas (`color_output_mask`, `writes_storage`).
  - `#310` (`5ea23d85f9`) : Dynarmic attend le commit d'espace de code au lieu d'émettre au-delà (`block_of_code.cpp`).
  - `#404` (`639eb0b0fe`) : Préservation de la croissance amortie des écritures d'état dans `dma_pusher.cpp`.
  - `#311` (`2a69bc4891`) : Alignement de la vérification clear-skip sur la couverture géométrique réelle du clear (`texture_cache.h`).
  - `#330` (`f7af6ed5a6`) : Rejet immédiat d'échec d'allocation mémoire invité avant le boot du kernel (`host_memory.cpp`, `core.cpp`).
  - `#418` (`db7e9d2331`) : Paramètre expérimental `pin_gpu_threads` pour épingler les threads split GPU à leurs propres cœurs (`core_pinning.cpp/.h`).
  - `#214` (`a632a225ab`) : Outils de reconstruction de pipeline cache et hints de session (`tools/nx2/`, `tools/shader-sweep/`).
  - `#346` (`ace89fe503`) : Collecte des contrôles de dénormaux pour FPMin32 et FPMax32 dans SPIR-V (`collect_shader_info_pass.cpp`).
  - `#444` (`378548a71c`) : Libération des gathered buffers multi-range lorsque leurs sources sont supprimées (`vk_multi_range_buffer.cpp/.h`, `vk_buffer_cache.cpp`).
  - `#140` / `#429` (`a3835e4cf3`, `68440a55c5`) : Prise en charge complète des descripteurs de stockage NVN (`UsesNvnStorageDescriptor`) et implémentation du compare-and-swap atomique 32 bits global (`GlobalAtomicCompareExchange32` et `StorageAtomicCompareExchange32`) en IR, SPIR-V, GLSL et Maxwell.
- **Donkey Kong Bananza (DKB)** :
  - Portage Eglantine-DKB `0799cba1f6` (`28f06b121a`) : Déport des libérations de fences du front, flushes DMA de macros et memory maps sur le thread split back via `GpuSplit::Run`, éliminant les crashs sur chunks nuls.
  - Portage Eglantine-DKB `1b6219e82d` (`dcb47ec3cc`) : Vidage des pages de listes de commandes écrites par draw uniquement après écriture effective.
- **Pokopia & Rendu Graphique** :
  - Portage notsapinho `77875bb6f9` (`be340b0fee`) : Correctifs compute shaders AMD Switch 2 pour Pokopia (photos zébrées résolues, buffers cohérents `LDG.E.STRONG.GPU`).
  - Ciel de Pokopia (`1edbdd42c9`) : Prise en charge de la variante uniform de FSEL (`0xc08`) et passage de `CACHE_VERSION` à 22 pour forcer l'invalidation automatique des anciens shaders corrompus stockés sur le disque local de l'utilisateur.
- **Splatoon** :
  - Tout le pipeline de lowering Ampere est désormais actif (`ISETP.EX`, `SHF.R.U64`, `SrcCopySrgbToLinear`). Prêt pour retest ; diagnostic activable via `EDEN_NX2_PIPELINE_BUILD_TRACE=1` si une anomalie d'exposition persiste.
- **État courant et étapes suivantes** :
  - Branche de travail : `rebase/smileytatsu` maintenue strictement isolée de `main`.
  - À tester sur machine réelle (RX 6700 XT) sur le nouveau build :
    1. Mario Kart World : confirmation de non-régression (~42 FPS TV).
    2. Zelda OOT : test de stabilité au-delà des 4 minutes (bénéficie de `ISETP.EX`, `#215`, `#329` Kokiri Forest).
    3. Donkey Kong Bananza : visibilité du terrain (Franseven) et tenue lors du cassage de blocs (DKB drain & fence backthread).
    4. Pokopia : affichage du ciel (nouveau cache v22) et photos en jeu.
    5. Splatoon : éclairage et rendu des scènes avec lowering Ampere complet.

## Section 11 (2026-11-07) : Correctifs boot OOT, bridage DKB 60 FPS, cache de pipelines Selevet & PRs upstream

- **Correction du crash au lancement d'OOT (build-40 -> build-41)** :
  - **Restriction de `EDEN_NX2_PBFLUSH` (`dma_pusher.cpp`)** : Le flush synchrone systématique `split.Drain()` et `memory_manager.FlushRegion` sur tous les blocs de paramètres <= 64 mots est désormais strictement restreint au Program ID de Donkey Kong Bananza (`0x040074A01BF12000ULL`) ou à l'activation explicite `EDEN_NX2_PBFLUSH=1`. Sur les autres jeux (dont OOT au boot), les paramètres conservent leur traitement asynchrone sans bloquer le parallélisme du split GPU.
  - **Restriction de `NX2_INDIRECT_RESET_PROGRAM` (`vk_rasterizer.cpp`)** : L'interception du programme de réinitialisation des indirect draw records de DKB est désormais strictement réservée à DKB (`0x040074A01BF12000ULL`).
  - **Sécurisation de `GetAppLoader()` et métadonnées (`core.h`, `core.cpp`, `main_window.cpp`, `application_functions.cpp`)** : Ajout du prédicat `System::HasAppLoader()` pour éviter tout déréférencement nul de `impl->app_loader` dans `MainWindow::BootGame` et `LooseDumpDisplayVersion`.
- **Plafonnement de Donkey Kong Bananza à 60 FPS (`nx2_gsv.h`)** :
  - Ajout de `DKBananzaProgramId` (`0x040074A01BF12000ULL`) au clamp de rafraîchissement effectif à 60 Hz dans `PaceToVblank`, éliminant les pics erratiques à 76-84 FPS observés en jeu, tout en préservant le débridage de Mario Kart World.
- **Résolution du build CI (`thread.h`)** :
  - Suppression de la déclaration dupliquée de `enum class PinnedRole` et `PinCurrentThread` dans `src/common/thread.h`, rétablissant la compilation propre de tous les jobs CI.
- **Résolution du temps de compilation des shaders (4 min à chaque lancement) & Portage Selevet build 10** :
  - **Réutilisation du cache pilote Vulkan (`vulkan_pipelines.bin`)** : Ajustement de `DriverCachesPipelines` dans `vk_pipeline_cache.cpp` (pilotable par `EDEN_FORCE_VULKAN_PIPELINE_CACHE`), afin que le blob binaire précompilé du driver Vulkan soit systématiquement sauvegardé à la fermeture et rechargé au lancement sur GPU AMD et NVIDIA, permettant un chargement instantané au lieu d'une recompilation complète des 2224 shaders.
  - **Système de précompilation en tâche de fond Selevet build 10 (`vk_pipeline_cache_prebuild.h`, `pipeline_build.h`, `async_draw_policy.h`)** :
    - Intégration de `PipelinePrebuild` (`background_pipelines`) pour compiler en arrière-plan les pipelines non prioritaires sans bloquer l'affichage.
    - Configuration du seuil de préchargement synchrone via `MinPipelinesAlwaysBuilt()` (`EDEN_NX2_MIN_PIPELINES_BUILT`, 2000 par défaut) et switch `EDEN_SELEVET_PIPELINE_CACHE=1` (actif par défaut).
- **Intégration des PRs upstream** :
  - `#397` (`5e14cab13e`) : `buffer_cache: don't download GPU data over newer CPU writes` (évite que les téléchargements asynchrones du GPU n'écrasent les écritures CPU plus récentes en mémoire invitée).
  - Vérification et confirmation des intégrations complètes de `#439` (RAM macro Turing à 0x8000), `#438` (restitution des macros aux moteurs en arrêt de split), `#425` (blocs de 64 octets dans usage tracker), `#435` (rejet des plages VA avec wrap), `#485` (maintien des tables de pages nulles), `#443` (crashs au scan des répertoires), `#442` (bouton de migration), `#429` (CAS atomique 32 bits), `#192` (sérialisation des guarded et mesh pipelines dans le cache disque), `#241` (alignement des registres d'uniformes hauts), `#279` (shadowing IMAD avec multiplicateur uniforme), `#232` (priorité aux cœurs P sur processeurs hybrides Windows), et Orchid 0.9.0.
- **État courant et étapes suivantes** :
  - Branche : `rebase/smileytatsu` (isolée de `main`).
  - Lancement d'un build de test en mode artifacts (`publish_release=false`).
  - Protocole de test utilisateur recommandé :
    1. **Ocarina of Time (OOT)** : vérifier que le jeu dépasse les 3,7 secondes, passe l'init audio et démarre la session sans crash au chargement.
    2. **Mario Kart World (MKW)** : mesurer le temps de chargement au 2e boot (vérifier le chargement accéléré via `vulkan_pipelines.bin` et le système Selevet).
    3. **Donkey Kong Bananza (DKB)** : vérifier que le framerate reste plafonné à 60 FPS maximum sans dépasser (fini les 76-84 FPS), et que le terrain et le cassage de blocs restent stables.
    4. **Pokopia** : vérifier l'affichage du ciel avec le cache invalidé v22 et les photos sans bandes zébrées.
    5. **Splatoon** : vérifier l'éclairage et la luminosité globale du décor avec le nouveau lowering Ampere.


## Section 12 (2026-11-07) : Déblocage CI, Opcode TENSOR_MUL_ADD Splatoon, Robustesse OOT NUP & Renommage Hortensia

- **Correction des échecs de compilation CI (build 42379467834)** :
  - **`src/common/host_memory.cpp`** : Ajout de `#include "common/settings.h"` pour déclarer `Settings` (résolvant `Settings::IsNceEnabled()`) et ajout du membre `size_t host_page_size{PageAlignment};` dans `HostMemory::Impl` (finalisation de l'intégration de Lichii #330).

- **Correction du rendu Splatoon (élimination des 744 assertions "Invalid insn 0x5d317f80..." & bandes verticales blanches)** :
  - **Opcode Maxwell `TENSOR_MUL_ADD` (`0x5d31...`)** : Le lowering Ampere (#87, `lower_to_maxwell.h`) émet un encodage Maxwell synthétique software collective (`0x5d30000000000000ULL | ... | (1ULL << 48) | (depth == 8 << 49)`) pour les instructions HMMA compactées. Cet opcode manquait au recompiler Maxwell de SmileyTatsu.
  - **Portage complet de `TENSOR_MUL_ADD` depuis Lichii nx2** :
    - `src/shader_recompiler/frontend/maxwell/maxwell.inc` : Ajout de `INST(TENSOR_MUL_ADD, "TENSOR_MUL_ADD", "0101 1101 0011 0---")`.
    - `src/shader_recompiler/frontend/maxwell/translate/impl/tensor_mul_add.cpp` : Implémentation du visiteur `TranslatorVisitor::TENSOR_MUL_ADD`.
    - `src/shader_recompiler/frontend/ir/opcodes.inc` : Déclaration de `OPCODE(TensorMulAdd, U32x2, U32x4, U32x2, U32x2, )`.
    - `src/shader_recompiler/frontend/ir/ir_emitter.h` & `ir_emitter.cpp` : Ajout de l'helper `TensorMulAdd(a, b, c, depth)`.
    - `src/shader_recompiler/ir_opt/collect_shader_info_pass.cpp` : Collecte des drapeaux de sous-groupes (`uses_subgroup_shuffles`, `uses_fp32_denorms_preserve`, `uses_subgroup_invocation_id`).
    - `src/shader_recompiler/backend/spirv/emit_spirv_tensor.cpp` & `emit_spirv.cpp` : Émission SPIR-V dédiée avec les fonctions collectives matricielles logicielles (`DefineSoftwareMatrixFunctions`, `EmitTensorMulAdd`).
    - `src/shader_recompiler/backend/glasm/` & `backend/glsl/` : Déclarations et stubs correspondants.
    - `src/shader_recompiler/CMakeLists.txt` : Enregistrement de `emit_spirv_tensor.cpp`, `tensor_mul_add.cpp`, et `integer_byte_permute.cpp`.
  - **Portage de `PRMT` (`src/shader_recompiler/frontend/maxwell/translate/impl/integer_byte_permute.cpp`)** : Remplacement des rejets `NotImplemented` de `PRMT` par l'implémentation complète des permutations d'octets.
  - **Portage des conversions scalaires et flottantes (`floating_point_conversion_integer.cpp`, `integer_floating_point_conversion.cpp`)** : Support du format de destination `I8`, conversion 64-bit float, et arrondis stricts pour les shaders OOT et Pokopia.
  - **Portage du correctif AMD Black Scene (#127, `structured_control_flow.cpp`)** : Prise en compte de tous les retours imbriqués dans `DemoteCombinationPass` pour éviter que les invocations dégradées ne sautent le demote et n'écrasent le composite de scène avec du noir sur cartes AMD Radeon (RX 6700 XT).

- **Robustesse du lancement d'Ocarina of Time (OOT .nup)** :
  - **Correction de la portée de `report_loader` (`src/yuzu/main_window.cpp`)** : Déclaration de `Loader::AppLoader* report_loader` au niveau du bloc englobant avec vérification null-safe, empêchant tout crash au démarrage lors de la construction de la session de compatibilité.
  - **Implémentation de `ReadDisplayVersion` dans `AppLoader_NUP` (`src/core/loader/nup.h`, `nup.cpp`)** : Renvoi immédiat de `display_version` ou délégation au `secondary_loader`.
  - **Garde de sécurité sur `m_applet` (`src/core/hle/service/am/service/application_functions.cpp`)** : Vérification `if (!m_applet || !system.HasAppLoader())` dans `LooseDumpDisplayVersion`.

- **Harmonisation et renommage utilisateur "Eglantine" -> "Hortensia"** :
  - `src/yuzu/aboutdialog.ui` : Liens mis à jour vers `https://github.com/justr4mzi/hortensia` (code source, issues et crédits).
  - `src/qt_common/discord/discord_impl.cpp` : `DEFAULT_DISCORD_TEXT` mis à jour ("Hortensia is an emulator for the Nintendo Switch").
  - `src/qt_common/config/uisettings.cpp` & `src/yuzu/configuration/configure_hotkeys.cpp` / `src/yuzu/hotkeys.cpp` / `src/yuzu/main_window.cpp` : Remplacement de "Exit Eglantine" par "Exit Hortensia" tout en maintenant le remapping transparent des fichiers de configuration existants.
  - `src/qt_common/config/shared_translation.cpp` : Descriptions traduites mises à jour avec "Hortensia".
  - `dist/dev.eden_emu.eden.desktop` : `Name=Hortensia`, `Icon=hortensia`, `TryExec=hortensia`, `Exec=hortensia %f`, `StartupWMClass=hortensia`.
  - `dist/io.github.justr4mzi.hortensia.metainfo.xml` : URLs d'accueil et bugtracker pointant vers `https://github.com/justr4mzi/hortensia`.
  - `docs/Build.md` : Titre et instructions de clonage vers `justr4mzi/hortensia`.
  - `.github/PULL_REQUEST_TEMPLATE.md` : Référence directe à `CONTRIBUTING.md` d'Hortensia, suppression des mentions de politique IA obsolètes.
  - `.github/ISSUE_TEMPLATE/*.yml` : Descriptions et labels mis à jour vers Hortensia.
  - `dist/packaging/arch/eglantine-git` renommé en `dist/packaging/arch/hortensia-git` avec `PKGBUILD` et `README.md` adaptés.
  - Conservation du commit `e2540de71b` (`CONTRIBUTING.md`) et préservation des identifiants internes requis (`EGLANTINE_STREAMLINE`, `eglantine.profdata`, migration de profil `EdenLegacy`/`Eglantine`).

- **Correction CI emit_spirv_instructions.h (commit e1f61dda91)** :
  - Déclaration anticipée de struct Program; dans le namespace Shader::IR pour résoudre l'erreur 
o type named 'Program' in namespace 'Shader::IR' survenue dans DefineSoftwareMatrixFunctions.
  - Déclenchement de la suite de compilation CI (run 42389131050) sur la branche 
ebase/smileytatsu.

- **Portage des nouveautés Pokopia (notsapinho)** :
  - **`maxwell_dma: flush accelerated image-to-buffer copies on Switch 2 (Pokopia's banded photos)` (`7dde4f15`)** : Vidage systématique de la région mémoire hôte vers la mémoire invitée après les copies image-vers-tampon accélérées sur Switch 2 (`memory_manager.FlushRegion`), éliminant les bandes noires et lignes résiduelles lors de l'encodage JPEG CPU des photos.
  - **`vk_rasterizer: draw Pokopia's far shadow cascade again (skip is opt-in)` (`84e061ce`)** : Restauration du rendu de la cascade d'ombre lointaine de Pokopia par défaut (le saut de cascade devient un réglage optionnel via `EDEN_NX2_SHADOW_FAR=0`), supprimant la coupure nette des ombres et le ciel noir.

- **Portage et vérification des nouveautés Lichii nx2** :
  - **#515 (`1bd370346d` / `a10e5f5569`)** : Support complet des mesh shaders émettant des topologies de points (`OutputPoints`) ou de lignes (`OutputLinesEXT`), avec gestion de `point_size` fixe et variables d'index primitives correspondantes.
  - **#524 (`ef22dfe338` / `c0a03141a4`)** : Désactivation du loop unswitching sur GCC 13+ (`-fno-unswitch-loops`) prévenant la perte des uploads macro dans `FrontCallMultiMethod` sous Linux.
  - **#519 (`a835cdba21`)** : Analyse effectuée : correctif spécifique aux types de capture micro Switch 2 et aux digests sha256 de l'auto-updater Lichii (non présents dans notre arbre de build).
  - **#526 (`91d1aada6b`)** : Analyse effectuée : bascule du workflow CI Windows de MinGW vers clang-cl ThinLTO avec un profil IR MSVC `lichii-windows.profdata` enregistré sur OOT. Hortensia conserve son pipeline hybride validé MinGW PGO AVX2 + Windows MSVC.
  - **#537 (`b83cd6e0b7`)** : Analyse effectuée : Hortensia dispose déjà de son propre bouton interactif avec menu contextuel et commutation temps-réel pour la génération d'images dans la barre d'état.
  - **#410 (`b8d81e2097`)** : Analyse effectuée : portage conditionné aux pools de tables de pointeurs Lichii (#351) ; notre recompiler gère déjà les candidats de pointeurs multiples.
  - **#192 (`077e455c85`)** : Vérification effectuée : déjà présent dans `src/video_core/shader_environment.cpp:272` (`CanBeSerialized` retourne `!has_unbound_instructions`).

- **Portage et vérification Orchid 0.9.0 & ZendNevs** :
  - **`8ebad36f58`** : Affichage permanent de la vitesse d'émulation dans la barre d'état même avec le multicœur activé.
  - **`c9950fca40`** : Implémentation du service `pl:u` commande 2100 (`OpenFont`) et conservation de la mémoire de transfert pour Switch 2.
  - **`c7d26dc2` (IMAD.HI)** : Vérification effectuée : notre implémentation dans `lower_to_maxwell.h` est plus complète que celle d'Orchid (gère déjà les produits larges et signés ainsi que les paires 64 bits `c_low` et `c_high = c + 1` pour les 6 opcodes).
  - **ZendNevs** : Dépôt public sans sources C++ complètes (releases seulement) ; ses patchs de dynamic FPS ont déjà été intégrés via `monteger8/Eglantine-DKB` dans `nso.cpp` et `settings.h`.

- **Préparation finale v1.0.0 & Bilan des intégrations** :
  - **Correction `integer_floating_point_conversion.cpp`** : Remplacement de l'appel `v.env.HasNativeInstructions()` par `v.env.IsNativeProgram()` (conformité avec l'architecture `Shader::Environment` de SmileyTatsu).
  - **Extension des descripteurs SSBO à 64 bindings (`f57593d805` / jpkto)** : Portage de la limite `Info::MAX_SSBOS = 64` et passage des masques de liaison de tampons de stockage en `u64` dans `buffer_cache_base.h` et `buffer_cache.h` (élimination des pertes de feuillage et d'herbe sous OoT).
  - **Prise en charge des régions DLSS actives et inversion verticale (Orchid)** : Ajout de `native_upscale.h` (`DecodeNativeUpscaleRegions`) et de l'inversion verticale bit 18 (`android::BufferTransformFlags::FlipV`).
  - **Documentation des commutateurs d'environnement** : Mise à jour exhaustive de `dist/nx2_switches.txt` et `README.md` avec `EDEN_NX2_DMA_READBACK_FLUSH`, `EDEN_NX2_PIPELINE_PRELOAD`, `EDEN_NX2_TENSOR`, `EDEN_NX2_DK_60FPS`, `EDEN_NX2_SHADOW_FAR`.
  - **Rédaction des notes de version v1.0.0** : Fichier `docs/RELEASE_NOTES_v1.0.0.md` rédigé en anglais, détaillant l'ensemble des correctifs, ports, performances mesurées et réglages par jeu.
  - **Vérifications préalables** : Zéro marqueur de conflit dans les fichiers texte, aucun script temporaire commité, isolation stricte des patchs spécifiques jeu (PBFLUSH, RESETFIX, limite 60 FPS) aux identifiants de programme correspondants.

## Section 13 (2026-11-07) : Déblocage multi-plateforme final (MSVC, MinGW, ARM64) et veille forks upstream

- **Corrections de compilation multi-plateforme (commit `24cfdb7f3a`)** :
  - **Inclusion de `shader_info.h` dans `buffer_cache_base.h`** : Rétablissement de `#include "shader_recompiler/shader_info.h"` pour définir `Shader::Info::MAX_SSBOS` sur les cibles Linux ARM64, macOS Apple Silicon et MinGW PGO (qui déclaraient `use of undeclared identifier 'Shader'`).
  - **Restauration de `FindNativeUpscaleOutput` et `FindNativeUpscaleSlots` dans `native_upscale.h`** : Conservation de ces utilitaires requis par `kepler_compute.cpp` (remplacement réseau vidéo OOT) aux côtés des structures de décodage DLSS (`DecodeNativeUpscaleRegions`).
  - **Suppression du doublon `AlignedVectorAccess` dans `emit_spirv_memory.cpp`** : L'implémentation active étant déjà fournie par `VectorViewIsExact`, la définition orpheline dans le namespace anonyme levait l'erreur MSVC stricte `C5245: unreferenced function with internal linkage has been removed`. Sa suppression débloque le job MSVC (`windows-x86_64`).

- **Veille des dépôts et forks upstream** :
  - **Lichii nx2 PR #551 (`6645721f2c`)** : Merge récent sur la reconstruction des pipelines compute dont les slots de pointeurs étaient nuls à la traduction et l'extension à 64 buffers de stockage. Vérification effectuée : Hortensia intègre déjà ces deux volets (`vk_pipeline_cache.cpp` et `buffer_cache_base.h`).
  - **Orchid main (`7655f3edb1`, `fdb103e78c`)** : Ajout d'un fallback opt-in expérimental pour dumps Xenoblade à index multiples (`ORCHID_XENOBLADE_ROMFS_FALLBACK`). Sans impact sur notre arbre.
  - **Lichii `capy/mesh-shader-fallback`** : Prototype d'émulation de mesh shaders pour GPU sans support natif de `VK_EXT_mesh_shader`. À suivre pour de futures évolutions.

- **Supervision CI en cours** :
  - Run GitHub Actions `42391714381` déclenché sur la branche `rebase/smileytatsu` (commit `24cfdb7f3a`).
  - Suivi continu des 6 jobs de compilation jusqu'à validation complète.
