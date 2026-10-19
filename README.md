# Johnny Castaway HD-2D — Linux, C99 and Vulkan

An experimental remaster of **Johnny Castaway / Screen Antics (1992)**: the
original scripts, visual gags and sprites play against a modern, GPU-animated
backdrop. Johnny keeps his retro design, cap, shorts and beard, with warmer
colours and lighting that ties him to the island.

**Current features:** 79 original scenes and 2,392 sprites, classic and modern
rendering, original sound effects, animated sea and shoreline, a day/night sky
with stars, the original palm tree and campfire lighting. Tested on Linux with
a Radeon 780M using RADV.

The presentation is **2D / 2.5D**, combining an illustrated backdrop with shaders.
It is not a fully modelled 3D scene.

## Demo video

https://github.com/user-attachments/assets/d9330753-e718-41ad-b86a-7958ed6a1a21

[Download the video](johnny-castaway-demo.mp4)

Three minutes of actual captures: the classic presentation, reading, coconuts,
fishing, raft building, sunset, a campfire and night on the island.
**3 minutes · H.264 MP4 · 1280 × 720 · 60 fps · original sound effects**,
The embedded preview is approximately 8.3 MB. The downloadable repository copy
is approximately 11.8 MB and uses a higher video bitrate.
French on-screen text explains the project, from REA
and the C99 port to the modern Vulkan effects. The recording includes the latest
shadow fixes.

The project includes **all 79 complete scenes from the original game**.
This video shows a selection of eight sequences, rather than the full catalogue.

## Contents

- [Demo video](#demo-video)
- [Quick start](#quick-start)
- [Installation and original game data](#installation-and-original-game-data)
- [Features](#features)
- [Commands and previews](#commands-and-previews)
- [All command-line options](#all-command-line-options)
- [Progress and configuration](#progress-and-configuration)
- [Building](#building)
- [Architecture](#architecture)
- [Tests and diagnostics](#tests-and-diagnostics)
- [Performance and size](#performance-and-size)
- [Known limitations](#known-limitations)
- [Validation and video](#validation-and-video)
- [Port origins and licences](#port-origins-and-licences)

## Quick start

Clone the repository, add the three original game files described below, then
build and start the preview:

```sh
git clone --depth 1 https://github.com/Nide96/johnny-hd2d.git
cd johnny-hd2d
# Place RESOURCE.MAP, RESOURCE.001 and SCRANTIC.SCR in assets/original/
make
./preview-living.sh --fullscreen
```

Alternatively, [download the current source as a ZIP](https://github.com/Nide96/johnny-hd2d/archive/refs/heads/main.zip),
extract it and open a terminal in the extracted directory before running `make`.
The ZIP and shallow clone avoid downloading the old, larger assets in Git history.

The living preview runs the normal scenes with a two-minute visual day. To start
under the stars instead:

```sh
./preview-stars.sh --fullscreen
```

For a normal session using local time and persistent progress:

```sh
./johnny-hd2d --modern --fullscreen
```

**Escape** closes the application. **M** toggles sound. The normal window is
resizable; `--fullscreen` selects fullscreen at startup.

All relative commands below assume your terminal is in the project directory.
The launch scripts locate the executable relative to their own location, so
they can also be started from another directory.

## Installation and original game data

### Runtime requirements

- Linux with working Vulkan support and the appropriate GPU driver.
- X11; on Hyprland/Wayland, **XWayland** must be available for the window.
- X11, Xi, Xcursor, Vulkan and ALSA runtime libraries, plus the usual C libraries.
- ALSA audio output; PipeWire can provide the ALSA route, as on the tested system.

Sokol headers are included in `vendor/`. Rust, SDL and Wine are not required to
run the program. Python is only used by development tools.

### Original game files

Place these three files from your own copy of the game in:

```text
assets/original/
├── RESOURCE.MAP
├── RESOURCE.001
└── SCRANTIC.SCR
```

File names are case-sensitive. The original game data **is not included in the
distribution**. `SCRANTIC.SCR` is read as a source of tables and sound data;
the Windows executable is not run.

You can use another data directory:

```sh
./johnny-hd2d --data /path/to/original/files --modern --fullscreen
```

`--data` changes the original game data directory, not the modern backdrop path.
The default backdrop is located relative to the executable at
`assets/modern/island-registered-v2.jcr`. Keep this directory structure when
moving the program, or use `--modern-plate` to supply an explicit path.

## Features

### Original logic and retro fidelity

- C99 port of the logic core: compressed resources, ADS/TTM scripts, movement,
  scene selection, calendar and progression through 11 story chapters.
- **79 scenes**, **117 BMP sprite sheets** and **2,392 original sprites**.
- An indexed **640 × 480** logical canvas, preserving original clipping and
  multipart sprite assembly.
- Two inherited timers: **55 ms** composition steps and a **16 ms** script timer.
- GPU presentation independent of composition timing, with subpixel movement
  interpolation between compatible states. Interpolation does not invent
  intermediate animation poses or change the timing of the gags.
- Injectable RNG seed and calendar for reproducible playback.

### Two presentations

| Mode | Presentation |
| --- | --- |
| `--classic` | Original indexed presentation in 4:3; the default mode. |
| `--modern` | Widescreen 16:9 backdrop, harmonised original sprites, animated lighting and environment. |

The initial window is 1280 × 960 in classic mode and 1280 × 720 in modern mode.
The viewport preserves the selected aspect ratio; other display shapes may
show borders. Rendering follows the output size and supports HiDPI.

### Johnny and the other sprites

Johnny is rendered **exclusively from the original indexed R8 atlas**, both in
normal scenes and in the walking preview. He no longer switches to a painted
replacement halfway through an animation.

The modern GPU material provides:

- A harmonised palette: warm skin, slightly ivory whites and deep brown outlines.
- Linear-light fusion of detected checkerboard dithering, including small 2 × 2 patterns.
- A directional 3 × 3 neighbourhood filter to soften diagonal stair steps.
- Reduced isolated white speckles in Johnny's dark areas.
- A restrained treatment for Johnny: no bevel shading and greatly reduced rim lighting.
- Preserved white shorts and cap, with readable eyes.
- Continuous ambient lighting, subtle relief and material-dependent rim lighting.
- Output-resolution alpha coverage for primitive lines, including the fishing line.

Johnny's skin and detail treatment targets his own resources. Props and other
characters retain their existing material; composite sprites preserve their
authored assembly and shared palette. The filter improves presentation at large
sizes without inventing new drawing detail.

### Palm tree and animation contact points

The palm tree **always uses the original sprites**, including in modern mode.
Its trunk, crown, animated parts and detached leaves therefore share the
geometry expected by the scripts. The reading and coconut gags no longer switch
between a painted crown and a retro animated fragment.

A light breeze moves the fronds while keeping attachment points fixed. Wind is
disabled from the start of scenes involving sensitive trunk, coconut or crown
interactions, preserving contact points and the original comic motion.

The old Johnny PNG/JCR sheets and painted palm replacement were removed:
**44 files, saving 168.25 MiB**. Obsolete metadata and their processing tools
were also removed. The legacy `--johnny-hd` and `--palm-original` options do not
restore any removed textures.

### Sea, shoreline and sand

- Crossing swells and small wavelets, compressed toward the horizon and filtered
  according to output resolution. Subtle UV distortion is restricted to water.
- Lit surface relief and small, broken highlights: most of the motion comes
  from changing light, without new textures or geometric simulation.
- Sun and moon reflections following their visual positions.
- Foam advancing and receding along the beach, with a wet sand edge.
- A shoreline mask built at startup from the supplied backdrop, using a signed
  distance for transitions. Interior sand and the rock are excluded from sea distortion.
- Subtle cloud shadows across the island.

The backdrop remains an illustration. Shaders animate its presentation rather
than simulate a complete physical ocean or terrain.

### Sky, day/night cycle and fire

- Procedural sky with continuous day, dusk and night transitions.
- Two moving cloud layers at different speeds.
- Sun, moon and analytic halos.
- Stationary stars with slow twinkling, cloud attenuation and a gradual fade at dawn.
- Warm light from the actual campfires in scenes 16–19, affecting sand, sprites
  and the palm tree.
- Shared fire flicker, subtle embers and a dedicated flame material visible at night.

The visual clock is independent of the VM. `--time-lapse` speeds up the visual
day; `--freeze-scene` lets the environment move around a frozen animation pose.
Animation phases wrap at 4,096 seconds, with a seamless transition in the tested case.
Halos are calculated in existing passes; this version does not use multipass HDR bloom.

### Shadows

Standing sprites have projected, sheared shadows. Lying and sitting poses, along
with the raft, use contact shadows close to their silhouettes. The sand mask
limits spill into the water.

Original grey ground shadows identified in **551 sprites across 48 families**
are removed only from the GPU copies to avoid doubling the modern shadows.
Coverage includes coconut and palm-shaking poses (`MJCOCO.BMP`), plus reviewed
fire, bottle, jogging, raft, seagull and other poses. Masks also apply to clipped
fragments, even when no new projected shadow is emitted.

Clothing and useful grey details are preserved. Unreviewed areas are handled
conservatively. The three separate coconut shadow sprites (`COCONUTS.BMP`,
frames 4–6) use the translucent modern ground-shadow material. Grey surface
pixels on the coconut itself are preserved.

### Audio

- **23 original sound effects**, stored as mono 8-bit PCM at 11,025 Hz.
- Sokol Audio / ALSA output, with linear resampling to the accepted device rate
  and duplication into stereo channels.
- Sound events observed at every VM step, including catch-up steps.
- Last sound takes priority: a new sound interrupts the previous one.
- Runtime volume from 0 to 100, default 70; mute option and **M** toggle.
- No audio device opened in headless mode. Audio initialisation failure is
  reported without stopping the animation.

No music or continuous ocean ambience has been added. A scene can remain
silent for a long time; `--skip-steps` does not replay sounds from skipped steps.
Validation videos may be silent.

## Commands and previews

### Included launchers

| Launcher | Behaviour |
| --- | --- |
| `preview-living.sh` | Normal scenes; starts at 17:00, with a 120-second visual day. |
| `preview-stars.sh` | Normal scenes; starts at midnight with stars. |
| `preview-modern.sh` | Normal scenes with the calendar fixed at noon. |
| `preview-hybrid.sh` | Fishing scene 20 at step 65, before its first useful sound effect. |
| `preview-sleep.sh` | Rest/sleep scene 51, chapter 6, at step 150. |
| `preview-reading-palm.sh` | Reading/coconut gag, scene 35 at step 125. |
| `preview-reading-hd.sh [scene]` | Reading; scene 34 by default, or 35 for the gag. |
| `preview-fishing-hd.sh [scene]` | Selected fishing scene; scene 20 by default. |
| `preview-scenes-hd.sh [scene]` | Scene 51 by default; also supports scenes such as 52 and 53. |
| `preview-walk-hd.sh` | Isolated walking loop, without story progression. |

Launcher names containing `hd` are historical: the character still uses R8
sprites. All these launchers use a fixed calendar and preserve the real save.
For scripts accepting a scene number, put it **before** other options:

```sh
./preview-reading-hd.sh 35 --fullscreen
./preview-scenes-hd.sh 52 --fullscreen
```

### Direct examples

```sh
# Original presentation
./johnny-hd2d --classic --fullscreen

# Reproducible reading scene
./johnny-hd2d --modern --scene 34 --seed 1 --clock 2026-10-18T12:00 --fullscreen

# Campfire at night, with the VM frozen and the environment still moving
./johnny-hd2d --modern --scene 16 --skip-steps 400 --seed 4242 --clock 2026-10-18T00:00 --freeze-scene --fullscreen

# Disable interpolation to compare discrete movement
./preview-hybrid.sh --fullscreen --no-interpolation

# Compare with the earlier illustrated sky and backdrop presentation
./preview-modern.sh --fullscreen --no-decor-fx

# List the scene catalogue
./johnny-hd2d --list-scenes
```

`--scene` forces a test scene; its duration and ending still follow its scripts.
`--walk-demo` is an isolated preview and does not play the other scenes.

## All command-line options

The executable reference is `./johnny-hd2d --help`.

| Option | Behaviour and constraints |
| --- | --- |
| `--modern` | Enables modern presentation. |
| `--classic` | Enables classic presentation; the default. |
| `--fullscreen` | Opens directly in fullscreen. |
| `--data DIR` | Directory containing the three original game files. |
| `--modern-plate FILE` | Uses a JCR1 backdrop; its alignment must remain compatible. |
| `--palm-original` | Legacy launcher compatibility; the original palm is already always used. |
| `--johnny-hd` | Legacy compatibility, requiring modern mode; Johnny still uses R8 sprites. |
| `--walk-demo` | Isolated walking preview; implies modern mode and noon unless a clock is supplied. |
| `--no-interpolation` | Shows discrete positions between composition steps. |
| `--no-decor-fx` | Disables the newer environment effects and keeps the earlier illustrated backdrop presentation; this is not classic mode. |
| `--time-lapse N` | Visual day lasting N seconds, from 24 to 3,600; leaves the VM calendar unchanged. |
| `--freeze-scene` | Freezes the VM while the application stays open and the environment clock continues. |
| `--decor-time N` | Fixes visual time at N seconds, from 0 to 1,000,000, for captures. |
| `--clock YYYY-MM-DDTHH:MM` | Injects a fixed local date/time and disables normal save persistence. |
| `--seed N` | Non-negative numeric RNG seed; defaults to the current time. |
| `--story-day N` | Test chapter, from 1 to 11. |
| `--scene N` | Forces a scene from the catalogue, from 1 to 79. |
| `--skip-steps N` | Advances 0 to 6,000 composition steps before display; requires `--scene`. |
| `--list-scenes` | Prints the catalogue and exits without opening a window. |
| `--volume N` | Volume from 0 to 100, default 70; does not change the INI. |
| `--mute` | Starts muted; press M to restore sound. |
| `--audio-test` | Plays three diagnostic beeps without a window or original game data. |
| `--headless N` | Runs 0 to 1,000,000 steps without a window or audio and prints a logical command-stream hash. |
| `--verify` | With `--headless`, reconstructs captured commands on the CPU and compares every pixel with the logical canvas. |
| `--snapshot FILE.ppm` | With `--headless`, exports the final 640 × 480 logical frame; this is not a modern-renderer screenshot. |
| `--smoke` | Closes after 240 submitted windowed frames and prints counters. |
| `--help` | Prints help and exits. |

`--walk-demo` cannot be combined with `--scene` or `--list-scenes`.
`--audio-test` cannot be combined with `--headless` or `--list-scenes`.
Purely visual options do not produce Vulkan output in headless mode;
`--freeze-scene` does not block explicitly requested headless steps.

## Progress and configuration

A normal session reads and saves `SCRANTIC.INI` **beside the executable**.
The file is written on normal shutdown, including handled SIGTERM or SIGINT,
using a temporary file and an atomic rename. Invalid configuration triggers a
warning and default values without overwriting the existing file.

The recognised section is `[ScreenSaver.ScreenAntics]`. Inherited keys are
`background`, `clouds`, `waves`, `sounds`, `numdays`, `introduction`,
`currentyear`, `currentmonth`, `currentday` and `starttime`. They control the
original logical configuration; they are not individual controls for every
new shader effect.

Normal sessions respect `sounds=0`. `--volume`, `--mute` and M do not rewrite
that INI setting.

Headless runs, catalogue listing, smoke tests and preview modes (`--walk-demo`,
a fixed clock, forced scene or chapter, accelerated visual cycle, frozen scene
or fixed visual time) neither load nor save real progress. `--seed` alone does
not disable persistence. The executable directory must be writable to save a session.

## Building

### Dependencies

A C99 compiler with LTO support, GNU Make, and X11/Xi/Xcursor, Vulkan and ALSA
headers and libraries. The Makefile links `X11`, `Xi`, `Xcursor`, `vulkan`,
`asound`, `dl`, `m` and POSIX threads. On Arch/CachyOS, ALSA is provided by
`alsa-lib`. The Vulkan driver and loader must be installed separately.

```sh
make
make size
```

Generated SPIR-V shaders are included in `generated/`: **sokol-shdc is not
required for a normal build**. After editing GLSL sources:

```sh
make shaders SHDC=/path/to/sokol-shdc
make
```

The code builds with `-std=c99 -Wall -Wextra -Wpedantic -Werror`. Sokol is selected
with `SOKOL_VULKAN`. Bundled headers are pinned to commit
`efeb77603e24daea7d1dd6445b9358bb5d82533b`; see `vendor/REVISION`.

Optional compression using UPX, supplied separately:

```sh
make compact UPX_BIN=/path/to/upx
./johnny-hd2d-compact --modern --fullscreen
```

`make clean` removes both executables; it does not delete the game data.

### Backdrop format

JCR1 consists of the `JCR1` signature, little-endian uint32 width and height,
then uncompressed RGBA8 pixels. The engine loads this format directly, with no
embedded JPEG or PNG decoder. Pillow is only required for conversion:

```sh
python3 tools/pack_plate.py source.png backdrop.jcr
./johnny-hd2d --modern --modern-plate backdrop.jcr
```

A replacement backdrop requires checking anchors, trunk occlusion and the sand
mask. Swapping in an arbitrary image does not align it with the original scenes.

## Architecture

```text
johnny-hd2d/
├── main.c                     CLI, window, clocks, loop and shutdown
├── vm_core.c / scene.h         VM/render bridge, double state and interpolation
├── engine.c / config.c         Story, calendar, RNG and INI progress
├── resource.c                 Original data loading and decompression
├── ads.c / ttm.c               Script interpretation
├── island.c / walk.c           Island logic and movement
├── gfx.c / render_frame.c      Reference canvas and command capture
├── renderer_vulkan.c           R8 atlas, Vulkan passes, materials and shadows
├── environment.c               Atmosphere, shoreline mask, wind and fire
├── audio.c / audio_mixer.c     ALSA output, mixing and resampling
├── layout.h / palm_layout.h    Backdrop and palm alignment
├── hybrid.h / sprite_material.h  Sprite classification and materials
├── shaders/                   GLSL sources
├── generated/                 Generated SPIR-V headers and geometry masks
├── vendor/                    Sokol headers and licences
├── assets/original/           Game data, present only in the local copy
├── assets/modern/             Single active JCR backdrop and provenance
├── tests/ / tools/            C tests and development tools
├── VALIDATION.md              Current validation notes
└── work/                      Temporary test output, excluded from the archive
```

The renderer consumes presentation states. Effects neither change the logical
canvas nor consume the story RNG. Scene IDs are also used to lock wind during
sensitive interactions. GDI compositions that cannot be decomposed fall back
to the harmonised original canvas to preserve their result.

## Tests and diagnostics

### C tests and memory checks

Tests that use original game data require those files to be present.

```sh
make test
make sanitize TESTDIR=work/sanitize
```

The suite covers resources, scripts, engine behaviour, command capture,
calendar, configuration, movement, reading, fishing, shadows, audio and the
environment. Replays of **79 scenes / 30,537 frames** compare the reference
canvas pixels exactly; this does not mean every modern scene has been visually
reviewed. `make sanitize` enables ASan/UBSan and disables leak detection
(`detect_leaks=0`).

Example of a GPU-free check and logical frame export:

```sh
./johnny-hd2d --headless 1000 --seed 4242 --clock 2026-10-18T17:00 --verify --snapshot /tmp/johnny-logical.ppm
```

The hash depends on all starting parameters. Compare runs with the same data,
seed, calendar, chapter and scene. `--verify` checks renderer commands on the CPU;
it does not read back the pixels actually produced by the GPU.

### Vulkan validation and GPU timing

With the Vulkan validation layer installed:

```sh
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./johnny-hd2d --modern --mute --smoke
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./johnny-hd2d --classic --mute --smoke
```

Included manual tools:

```sh
python3 tools/test_material_gpu.py
python3 tools/test_environment_gpu.py
python3 tools/bench_environment.py
make inventory
```

Visual checks require an active Hyprland session, `hyprctl`, `grim`, Pillow,
and NumPy for the environment checks. They open their own windows and capture
the window area; keep those windows visible during testing. Results go into
`work/`. The benchmark builds a temporary variant instrumented with GPU
timestamps; it does not measure the application's complete presentation rate.
`make inventory` rebuilds the inventory from the original scenes.

Obsolete RGBA sheet processing tools and metadata have been removed.
`make inventory` writes its local inventory to `assets/scenes/`, which is
excluded from the distribution and ignored by Git.

### Troubleshooting

| Symptom | What to check |
| --- | --- |
| `No such file or directory` when starting a launcher | Return to the project directory or use its absolute path. Keep `./preview-living.sh` and its arguments on the same line, or escape the line break. |
| Data error at startup | Check all three files, their case-sensitive names and `--data`. |
| No window under Wayland | Check XWayland, the graphical session and Vulkan driver; the current window backend uses X11. |
| Modern backdrop not found | Keep `assets/modern/` beside the executable or pass `--modern-plate`. |
| Only walking around the palm tree | Remove `--walk-demo` or the walking launcher; use `preview-living.sh` for normal scenes. |
| No audio | Check M, `--mute`, `--volume`, `sounds=0` and ALSA messages; try `./johnny-hd2d --audio-test`. |
| Three diagnostic beeps work, but a scene is silent | Some scenes contain few sounds. Try `./preview-hybrid.sh`, which starts before the fishing sound effect. |
| No stars | Try `preview-stars.sh`; stars fade during daylight, under clouds and in composite fallbacks. |
| Progress is not saved | Check that no preview mode is active and the executable directory is writable. |
| An opaque grey shadow remains | Reviewed sprite masks and separate coconut shadows are handled in modern mode. Rare composite fallbacks and unreviewed areas may still retain original shadows. |

## Performance and size

### Memory and frame timing

Vulkan staging is bounded at **7 MiB**: 1 MiB copy buffer, 2 MiB transient buffers
for each of two frames in flight, and a 2 MiB atlas relay. This is not the total
memory used by the program or driver.

The shoreline mask is **640 × 360 RGBA8**, or 921,600 bytes (**0.88 MiB on the
GPU**). Building it requires approximately **3.30 MiB of temporary CPU memory**,
freed after upload. No Johnny or painted-palm RGBA sheets are loaded; the JCR
backdrop is the artwork loaded at modern startup.

Measurements recorded on **19 October 2026**, with the wavelet shader on a
Radeon 780M at 1920 × 1080, averaged **1.70 ms for the graphics pass in daylight,
1.72 ms at night and 1.76 ms with fire**, over 209 samples after warm-up.
These measurements preceded the latest shadow fixes and exclude CPU work,
uploads and presentation latency. They are not an end-to-end benchmark of the
current package. VSync is enabled; throughput depends on the display,
compositor and the rest of the loop. **Sustained 144 fps is neither guaranteed
nor demonstrated by these timings.**

### Minimal distribution

The repository keeps the code, active tests and tools, shaders, licences, the
JCR backdrop in use and one demo video. Old screenshots, GIFs, videos, backdrop
variants, duplicate PNGs, removed-sheet metadata and historical logs are no
longer distributed. The compressed executable can be built with `make compact`;
a duplicate binary is no longer included.

A separate local runtime package contains only the standard executable, JCR
backdrop, notices, a short guide and the two day/night launchers. No source code,
tests, build tools or demo media are needed at runtime. The three original game
files must still be supplied separately.

Measurements and package scopes: [SIZE.md](SIZE.md). One MiB is 1,048,576 bytes.
System libraries are not included in these sizes. Old large assets remain in
Git history; use the current branch ZIP or `git clone --depth 1` for a smaller download.

## Known limitations

- Rare GDI compositions that cannot be decomposed retain the harmonised original
  canvas, without individual shadows or the newer environment effects. Assembly
  fidelity is preserved, but a presentation difference may remain visible.
- Masks and anchors are tuned to the supplied, registered backdrop. Rock outlines
  and some shadow regions are handled conservatively.
- De-dithering is pattern-based. It is not an exhaustive artistic retouch of every
  sprite and does not remove every possible checkerboard pattern.
- Wind is deliberately disabled in several sensitive scenes, even where subtle
  motion might otherwise be desirable.
- Shaders do not create new animation poses. Internal sprite animation is not
  made as continuous as interpolated movement.
- No complete 3D geometry, multipass HDR bloom, added music or individual UI
  controls for every graphical effect.
- The window is opaque. Transparent wallpaper mode, layer-shell integration,
  autostart and session locking are not implemented. The program currently runs
  as an application, not as a desktop-integrated screensaver service.
- Fullscreen is selected at startup; no runtime toggle shortcut is provided.
- Validation captures document the tested cases, not an exhaustive visual review
  of all 79 scenes on every driver.

## Validation and video

[VALIDATION.md](VALIDATION.md) records checks and their limits. Temporary logs
are written to `work/` and are not part of the distribution.
The [demo video](johnny-castaway-demo.mp4) contains actual captures of the
classic, modern, sunset and night/campfire presentations. It includes original
sound effects and French on-screen explanations, with no added music.

## Port origins and licences

The core is primarily based on
[open-scrantic](https://github.com/glauberlima/open-scrantic/tree/2db75008931eb744f79d0c3fb12e0d89950466ff),
revision `2db75008931eb744f79d0c3fb12e0d89950466ff`, adapted to C99.
Its MIT licence and attributions are preserved in `LICENSE` and
`licenses/open-scrantic-MIT.txt`.

[jc_reborn](https://github.com/jno6809/jc_reborn/tree/524a5803e4fa65f840379c781f40ce39a927032e)
served as a second reference; none of its GPL source files were incorporated.
Dependencies have their own notices in `vendor/` and `licenses/`. The provenance
of the modern artwork is documented in `assets/modern/`.

Original game data remains separate from the port's source code. It and
`SCRANTIC.INI` are excluded from the distribution archive. `SHA256SUMS` covers
the selected distribution files and can be checked from the package root:

```sh
sha256sum -c SHA256SUMS
```
