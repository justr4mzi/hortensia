<!--
# SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later
-->

# Lichii-AMD

Lichii-AMD is [Lichii](https://github.com/zortos293/Lichii) with fixes for **AMD Radeon** GPUs, tested with
**The Legend of Zelda: Ocarina of Time** for Switch 2 (`0400486024DEC000`). It is built on Lichii's `nx2`
branch at `9fa4a71fa0`, which in turn builds on Eglantine, Eden, yuzu and Sudachi. This fork is not
affiliated with Nintendo, AMD or the upstream projects.

*Português abaixo.*

## Download and setup

Download the zip from the [releases page](https://github.com/nauxph/Lichii-AMD/releases), extract all
of it and run `lichii.exe`.

You need:

- Windows 10 or 11 (64-bit) and an AMD Radeon GPU with a current driver. This build was tested on an
  RX 9070 XT with driver 2.0.395.
- The latest **Microsoft Visual C++ Redistributable (x64)**:
  <https://aka.ms/vc14/vc_redist.x64.exe>. The build uses MSVC 14.51, so an older runtime can stop
  it from starting.
- Your own keys, firmware and games, dumped from your own console. None of them are included.

For Ocarina of Time, set **GPU Mode** to **Fast** in the graphics settings, either globally or for the
game. All measurements below used Fast.

The first launch of this version compiles Ocarina's shaders again (shader cache version 54), so it
stutters for a while in each new area.

## Changes from Lichii

The commits are on the `amd` branch.

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

- **Green sky, fog and water.** Compute table pointers are bound up to 256 MiB (port of Lichii #292).
  The fog pass reads its light probes past the old 64 MiB cap, and on AMD it read garbage there.
- **Black water and foliage, then a yellow-white image.** After an area change, such as entering a
  house and coming back, Kokiri Forest's water and foliage turned black, and later the whole image
  turned yellow-white until a restart. The foliage and water shaders read the same light probes past
  the 64 MiB cap that graphics draws still had. Graphics table pointers now get 256 MiB too (the
  second commit of Lichii #292). Float min, max and clamp also use NaN-defined SPIR-V instructions
  (Lichii #432), like Maxwell's FMNMX, which returns the operand that isn't NaN, so a NaN no longer
  spreads across the screen through the bloom.
- **Flickering HUD, minimap and dialogs.** A full memory barrier runs around every render pass
  (`EDEN_NX2_RP_BARRIER=1`, on by default in this build). It costs about 0.05 ms a frame.
- **Movies in sync with their sound.** Ports of Lichii #251 (movie clock) and #278
  (`EDEN_NX2_FS_READ_MBPS=0`).
- **Other ports.** Lichii #204 and #215 (macro DMA reads wait for pending query results), #243
  (early macro DMA reads), #268 (texture cache budget) and #259 (opt-in `EDEN_NX2_PREFLUSH`, which
  should stay **off** on AMD because it turns water and grass black).
- **Lichii `nx2` up to `9fa4a71fa0`.** Among its changes, #397 stops buffer downloads from writing the
  GPU's older copy over newer CPU writes, and #410 binds the buffers Ocarina's near tree leaves are
  read from.
- **Diagnostics.** GPU time per recording line (`EDEN_DKB_FRAME_TRACE=1`), frame time percentiles in
  the log, and live `rp_barrier` switching on the debug server (`docs/nx2/debug-mcp.md`).

## Switches

The `nx2_switches.txt` next to `lichii.exe` sets this build's defaults: the GPU split, fence
batching, 128-draw submissions, the render pass barrier and unthrottled game file reads. To change
one, copy its line into `%APPDATA%\eden\nx2_switches.txt`, which wins over the file next to the
executable, then restart.

## Known issues

- Only Ocarina of Time on one PC was tested with this build.
- The GPU emulation thread now limits the frame rate. It is about 91% busy at the village, and
  heavier areas run slower.
- `EDEN_NX2_PREFLUSH=1` corrupts water and grass on AMD.
- In Hyrule Field, AMD cards can show coloured blocks along the horizon (Lichii #322). Its fix, #353,
  is not in this build yet.

## License and source

Lichii-AMD is GPL-3.0-or-later; see `LICENSE.txt` and `LICENSES/`. The source for each release is
its tagged commit on the `amd` branch of this repository.

---

## Português

O Lichii-AMD é o [Lichii](https://github.com/zortos293/Lichii) com correções para placas **AMD Radeon**,
testado com **The Legend of Zelda: Ocarina of Time** de Switch 2 (`0400486024DEC000`). Ele parte do
branch `nx2` do Lichii, que por sua vez vem do Eglantine, do Eden, do yuzu e do Sudachi. Não tem
ligação com a Nintendo, com a AMD nem com os projetos originais.

### Como usar

Baixe o zip na [página de releases](https://github.com/nauxph/Lichii-AMD/releases), extraia tudo e
abra o `lichii.exe`.

Você precisa de:

- Windows 10 ou 11 (64 bits) e uma placa AMD Radeon com driver atualizado. Testado numa RX 9070 XT
  com o driver 2.0.395.
- O **Microsoft Visual C++ Redistributable (x64)** mais recente:
  <https://aka.ms/vc14/vc_redist.x64.exe>. Sem ele atualizado, o emulador pode nem abrir.
- Suas próprias chaves, firmware e jogos, extraídos do seu console. Nada disso vem incluído.

No Ocarina of Time, deixe o **GPU Mode** em **Fast** nas opções de gráficos, globais ou do jogo.

Na primeira vez que abrir esta versão, os shaders do Ocarina são compilados de novo (cache versão 54),
então o jogo engasga um pouco em cada área nova.

### O que muda em relação ao Lichii

- **Cópia das queries de oclusão:** nas placas AMD, os resultados são copiados depois de uma única
  barreira, sem esperar query por query. Na mesma cena, o FPS passou de 19,7 para 34,7, o p95 de
  55,9 para 34,2 ms e o p99 de 66,0 para 42,0 ms.
- **Céu, névoa e água verdes:** corrigidos (port da PR #292 do Lichii).
- **Água e plantas pretas, depois a imagem toda amarela:** corrigidos. Depois de entrar numa casa e
  voltar, a água e a folhagem da floresta Kokiri ficavam pretas e, com o tempo, a tela inteira ficava
  amarelo-esbranquiçada até reiniciar. Os shaders gráficos agora também leem as probes de luz até
  256 MiB (segundo commit da PR #292), e o mínimo, o máximo e o clamp de ponto flutuante não deixam
  mais um NaN se espalhar pela tela (PR #432).
- **HUD, minimapa e diálogos piscando:** corrigidos com uma barreira em volta de cada passe de
  renderização (`EDEN_NX2_RP_BARRIER=1`), que custa ~0,05 ms por quadro.
- **Vídeos fora de sincronia com o som:** corrigidos (ports das PRs #251 e #278).
- Deixe `EDEN_NX2_PREFLUSH` desligado: na AMD ele deixa a água e a grama pretas.

### Licença

GPL-3.0-or-later; veja `LICENSE.txt` e `LICENSES/`. O código de cada release é o commit marcado com a
tag dele no branch `amd`.
