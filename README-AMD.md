<!--
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later
-->

# Hortensia-AMD

Hortensia-AMD is [Hortensia](https://github.com/zortos293/Lichii) with fixes for **AMD Radeon** GPUs, tested with
**The Legend of Zelda: Ocarina of Time** for Switch 2 (`0400486024DEC000`). It is built on Hortensia's `nx2`
branch at `65e979adb3`, which in turn builds on Eglantine, Eden, yuzu and Sudachi. This fork is not
affiliated with Nintendo, AMD or the upstream projects.

*Português abaixo.*

## Download and setup

Download the zip from the [releases page](https://github.com/nauxph/Hortensia-AMD/releases), extract all
of it and run `hortensia.exe`.

You need:

- Windows 10 or 11 (64-bit) and an AMD Radeon GPU with a current driver. This build was tested on an
  RX 9070 XT with driver 2.0.395.
- The latest **Microsoft Visual C++ Redistributable (x64)**:
  <https://aka.ms/vc14/vc_redist.x64.exe>. The build uses MSVC 14.51, so an older runtime can stop
  it from starting.
- Your own keys, firmware and games, dumped from your own console. None of them are included.

For Ocarina of Time, set **GPU Mode** to **Fast** in the graphics settings, either globally or for the
game. All measurements below used Fast.

The first launch of this version compiles Ocarina's shaders again (the driver's cache does not carry
over), so it stutters for a while in each new area.

Shader stutter needs no extra setting: built pipelines are kept for the session, and while a very
large shader compiles for the first time, its draws are skipped instead of freezing the game.
Asynchronous shader building can stay off. To turn the skipping off, put `EDEN_NX2_HEAVY_SKIP=0` in
`nx2_switches.txt` (see Switches).

## Changes from Hortensia

The commits are on the `amd` branch.

- **Shader stutter in combat, on area changes and in new places.**
  - Switch 2 pipelines are kept for the session, up to an eighth of the RAM (port of Hortensia #313).
    Before, a pipeline unused for 2.5 minutes was destroyed, and going back to an area or fighting
    again rebuilt hundreds at once. One 90-minute session rebuilt about 10,000 and held draws for
    214 s, because AMD's driver cache missed many of the rebuilds.
  - A draw waiting for its pipeline builds that pipeline right away instead of waiting behind the
    whole queue (port of Hortensia #321).
  - While a shader of 4,000 or more instructions compiles for the first time, its draws are
    skipped unless that could lose data. This is the heavy skip from Hortensia #579, on by default
    here and also without asynchronous shaders. Draws held per new pipeline went from 111-124 ms to
    2-20 ms, and the worst freeze from 7-11 s to under 1 s, with no missing objects seen. A large
    object can appear a moment late in a new place.
- **Ocarina's image filter on the GPU's matrix units** (Hortensia #583). The game's tensor-core image
  filter runs on 16x16x16 cooperative matrices on GPUs that have them. On the RX 9070 XT the GPU
  went from about 64% to about 13% busy.
- **Occlusion query copies (frame rate).** On AMD drivers, the occlusion query results the game reads
  are copied after one barrier instead of waiting on each query (`VK_QUERY_RESULT_WAIT_BIT`). AMD's
  driver spent about 22 µs on each wait. With Ocarina's ~2,000 queries a frame, that came to 45 ms of
  a 61 ms GPU frame; the copies now take 0.4 ms.

  Measured at the Kokiri village sign (RX 9070 XT, docked, 1440p), in the same scene and session,
  over three 600-frame windows each:

  | | Before | After |
  |---|---|---|
  | Frame rate | 19.7 fps | 34.7 fps |
  | p95 frame time | 55.9 ms | 34.2 ms |
  | p99 frame time | 66.0 ms | 42.0 ms |
  | 1% low | 15.2 fps | 23.9 fps |

- **Green sky, fog and water.** Read-only table pointers are bound as far as a storage buffer
  reaches, up to 1 GiB (Hortensia #351, which replaced this fork's port of #292). The fog pass reads its
  light probes past the old 64 MiB cap, and on AMD it read garbage there.
- **Black water and foliage, then a yellow-white image.** After an area change, such as entering a
  house and coming back, Kokiri Forest's water and foliage turned black, and later the whole image
  turned yellow-white until a restart. The foliage and water shaders read the same light probes past
  the 64 MiB cap that graphics draws had, and get the same reach as the fog now. Float min, max and
  clamp also use NaN-defined SPIR-V instructions
  (Hortensia #432), like Maxwell's FMNMX, which returns the operand that isn't NaN, so a NaN no longer
  spreads across the screen through the bloom.
- **Flickering HUD, minimap and dialogs.** A full memory barrier runs around every render pass
  (`EDEN_NX2_RP_BARRIER=1`, on by default in this build). It costs about 0.05 ms a frame.
- **Movies in sync with their sound.** Ports of Hortensia #251 (movie clock) and #278
  (`EDEN_NX2_FS_READ_MBPS=0`).
- **Other ports.** Hortensia #204 and #215 (macro DMA reads wait for pending query results), #243
  (early macro DMA reads), #268 (texture cache budget) and #259 (opt-in `EDEN_NX2_PREFLUSH`, which
  should stay **off** on AMD because it turns water and grass black).
- **Hortensia `nx2` up to `65e979adb3`.** Among its changes:
  - #550 keeps the pipelines prebuilt from the disk cache until a draw first uses them.
  - #353 fixes the coloured blocks along the Hyrule Field horizon on AMD.
  - #397 stops buffer downloads from writing the GPU's older copy over newer CPU writes.
  - #446 polls suspended GPU channels more often (upstream measured Ocarina about 11% faster in
    Kokiri village).
  - #410 binds the buffers Ocarina's near tree leaves are read from.
  - #580 reuses a buffer range's page table walk, #584 keeps Ocarina's Dynamic FPS at real speed
    down to 10 fps, and #585 stops padding the audio with silence while sound is queued.
- **Diagnostics.** GPU time per recording line (`EDEN_DKB_FRAME_TRACE=1`), frame time percentiles in
  the log, and live `rp_barrier` switching on the debug server (`docs/nx2/debug-mcp.md`).

## Switches

The `nx2_switches.txt` next to `hortensia.exe` sets this build's defaults: the GPU split, fence
batching, 128-draw submissions, the render pass barrier and unthrottled game file reads. To change
one, copy its line into `%APPDATA%\eden\nx2_switches.txt`, which wins over the file next to the
executable, then restart. `EDEN_NX2_HEAVY_SKIP=0` there turns off the skipping of heavy shaders.

## Known issues

- Only Ocarina of Time on one PC was tested with this build.
- The frame rate moves between about 40 and 60 fps while walking. It is limited by the round trips
  between the emulated CPU and the GPU emulation threads, not by the graphics card (the RX 9070 XT
  is about 13% busy) or by one saturated thread.
- Loading a new area can pause the game for a few seconds; once it took 19 s.
- `EDEN_NX2_PREFLUSH=1` corrupts water and grass on AMD.

## License and source

Hortensia-AMD is GPL-3.0-or-later; see `LICENSE.txt` and `LICENSES/`. The source for each release is
its tagged commit on the `amd` branch of this repository.

---

## Português

O Hortensia-AMD é o [Hortensia](https://github.com/zortos293/Lichii) com correções para placas **AMD Radeon**,
testado com **The Legend of Zelda: Ocarina of Time** de Switch 2 (`0400486024DEC000`). Ele parte do
branch `nx2` do Hortensia, que por sua vez vem do Eglantine, do Eden, do yuzu e do Sudachi. Não tem
ligação com a Nintendo, com a AMD nem com os projetos originais.

### Como usar

Baixe o zip na [página de releases](https://github.com/nauxph/Hortensia-AMD/releases), extraia tudo e
abra o `hortensia.exe`.

Você precisa de:

- Windows 10 ou 11 (64 bits) e uma placa AMD Radeon com driver atualizado. Testado numa RX 9070 XT
  com o driver 2.0.395.
- O **Microsoft Visual C++ Redistributable (x64)** mais recente:
  <https://aka.ms/vc14/vc_redist.x64.exe>. Sem ele atualizado, o emulador pode nem abrir.
- Suas próprias chaves, firmware e jogos, extraídos do seu console. Nada disso vem incluído.

No Ocarina of Time, deixe o **GPU Mode** em **Fast** nas opções de gráficos, globais ou do jogo.

Na primeira vez que abrir esta versão, os shaders do Ocarina são compilados de novo (o cache do
driver não aproveita os antigos), então o jogo engasga um pouco em cada área nova.

As travadas de shader não precisam de nenhum ajuste. Os pipelines compilados ficam guardados durante
a sessão, e, enquanto um shader muito pesado compila pela primeira vez, os desenhos dele são pulados
em vez de travar o jogo. A compilação assíncrona pode ficar desligada. Para desligar o pulo, coloque
`EDEN_NX2_HEAVY_SKIP=0` no `nx2_switches.txt`.

### O que muda em relação ao Hortensia

- **Cópia das queries de oclusão:** nas placas AMD, os resultados são copiados depois de uma única
  barreira, sem esperar query por query. Na mesma cena, o FPS passou de 19,7 para 34,7, o p95 de
  55,9 para 34,2 ms e o p99 de 66,0 para 42,0 ms.
- **Céu, névoa e água verdes:** corrigidos (PR #351 do Hortensia, que substituiu o port da #292).
- **Água e plantas pretas, depois a imagem toda amarela:** corrigidos. Depois de entrar numa casa e
  voltar, a água e a folhagem da floresta Kokiri ficavam pretas e, com o tempo, a tela inteira ficava
  amarelo-esbranquiçada até reiniciar. Os shaders gráficos agora leem as probes de luz tão longe
  quanto a névoa (PR #351), e o mínimo, o máximo e o clamp de ponto flutuante não deixam mais um NaN
  se espalhar pela tela (PR #432).
- **Travadas de shader em combate, em troca de área e em lugares novos:**
  - Os pipelines ficam guardados durante a sessão, até 1/8 da RAM (port da PR #313). Antes, eram
    descartados depois de 2,5 minutos sem uso e recompilados ao voltar.
  - O desenho que espera um pipeline compila esse pipeline na hora (PR #321).
  - Enquanto um shader de 4.000 instruções ou mais compila pela primeira vez, os desenhos dele são
    pulados, se isso não perder dados (PR #579, ligado por padrão aqui).
  - A espera por pipeline novo caiu de 111–124 ms para 2–20 ms, e a pior travada de 7–11 s para menos
    de 1 s, sem objetos faltando. Um objeto grande pode aparecer um instante depois num lugar novo.
- **Filtro de imagem do Ocarina nas matrizes da placa** (PR #583): ele roda nas matrizes 16x16x16 das
  placas que têm esse recurso. Na RX 9070 XT, o uso da GPU caiu de ~64% para ~13%.
- **Blocos coloridos no horizonte de Hyrule Field:** corrigidos (PR #353).
- **HUD, minimapa e diálogos piscando:** corrigidos com uma barreira em volta de cada passe de
  renderização (`EDEN_NX2_RP_BARRIER=1`), que custa ~0,05 ms por quadro.
- **Vídeos fora de sincronia com o som:** corrigidos (ports das PRs #251 e #278).
- Deixe `EDEN_NX2_PREFLUSH` desligado: na AMD ele deixa a água e a grama pretas.

### Problemas conhecidos

- Só o Ocarina of Time foi testado, num único PC.
- Andando, o FPS varia entre ~40 e 60. O limite são as idas e voltas entre a CPU emulada e as threads
  que emulam a GPU, não a placa de vídeo (a RX 9070 XT fica em ~13% de uso).
- Carregar uma área nova pode pausar o jogo por alguns segundos; uma vez levou 19 s.

### Licença

GPL-3.0-or-later; veja `LICENSE.txt` e `LICENSES/`. O código de cada release é o commit marcado com a
tag dele no branch `amd`.
