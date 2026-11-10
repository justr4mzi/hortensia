> [!IMPORTANT]
> ## Report issues from inside Orchid
> Use **Help > Report an Issue on GitHub** to open a report with your build, system, GPU, driver and recent game log. After a crash, restart Orchid first and attach the full game log from the folder it opens.

## 0.9.2 — Xenoblade 2 boots, Xenoblade DE at 60 fps, Wonder flicker fix and NUP extraction

### Highlights

- **Xenoblade Chronicles 2 – Nintendo Switch 2 Edition:** boots past the startup abort (`nn::fs` 0x160A). Switch 2 processes now get a 128 GiB alias region, which the game's address allocator needs. In handheld mode the 3D scene is no longer shown zoomed in 2x.
- **Xenoblade Chronicles: Definitive Edition – Nintendo Switch 2 Edition:** runs at 60 fps in the opening instead of 30-38. A per-game setting stops the texture readback the game does every frame from waiting on the whole GPU queue.
- **Super Mario Bros. Wonder – Nintendo Switch 2 Edition:** scenery no longer flickers dark blue. GPU timestamps now reflect when the work was submitted, so the game stops switching to its cheaper lighting path every other frame.
- **Switch 2 Edition dumps, one loader for every layout:** extracted folders, NSP/XCI and NUP all serve the edition's other programs and add-ons the same way. Dumps without program 0 (Xenoblade 2) get a stand-in in every format. NUP files packed with older versions must be repacked to include the sibling programs.
- **NUP tools:** *Extract NUP* (game list context menu, `eden-cli --extract-nup`) unpacks an archive back into the folder layout, checking every file's SHA-256. *Edit NUP Metadata* changes name, icon and display version in place without repacking.
- The Xenoblade series (1, 2, X, 3), Orbitals and the Kirby Switch 2 Edition no longer trigger the unsupported-game warning.

### Known issues

- Xenoblade Chronicles X: the intro video can freeze, and character creation can hang on a black screen.
