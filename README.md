# Flycast 2022 Low-End

Flycast 2022 Low-End is an unofficial, performance-oriented libretro fork for
low-power ARM handhelds. It is based on
[`libretro/flycast` commit `4c293f3`](https://github.com/libretro/flycast/tree/4c293f306bc16a265c2d768af5d0cea138426054)
from 6 April 2022.

The fork keeps the original renderer as the default and adds explicitly
optional optimizations for devices such as the RK3326/Cortex-A35 family.
Accuracy-changing options remain disabled unless the user enables them.

For the emulator's original features, BIOS, game compatibility and general
configuration, refer to the
[original Flycast documentation](https://docs.libretro.com/library/flycast/)
and the
[upstream source at the base revision](https://github.com/libretro/flycast/tree/4c293f306bc16a265c2d768af5d0cea138426054).

## Core identity and distribution packaging

The source keeps the identity inherited from the base revision:

```text
Core option prefix: reicast
Libretro library name: Flycast
Build output: flycast_libretro.so
```

Distribution packaging may rename this identity without maintaining a separate
source branch. AmberELEC packages this fork separately from its historical
Flycast 2021 core, rewrites the option prefix to `flycast2022`, changes the
library name to `Flycast 2022 Low-End`, and installs the core as
`flycast2022_libretro.so`. dArkOS can keep the source defaults, so its existing
`reicast_*` configuration continues to work.

The configuration examples below use the source/dArkOS `reicast_*` names. On
AmberELEC, replace only the `reicast_` prefix with `flycast2022_`. See
[the distribution packaging guide](docs/DISTRIBUTION_PACKAGING.md) for the
exact mapping and build requirements.

## New core options

All option keys are generated from the compile-time core option prefix. This
keeps the implementation identical across distributions while allowing each
package to preserve its existing configuration names.

### Texture Storage Reuse

Key:

```text
reicast_texture_storage_reuse
```

Values:

- `enabled` (default): uses `glTexSubImage2D` when an existing texture
  allocation has exactly the required width, height, GLES format and type.
- `disabled`: restores the original `glTexImage2D` upload path.

Five alternating RG351V runs improved mean FPS by 7.45% in the texture-heavy
Marvel vs. Capcom 2 scene. Sonic Adventure 2 and Soul Calibur showed no
meaningful performance regression, and the final candidate passed the user's
visual and audible device tests. The first upload, allocation mismatches,
mip-chain uploads, recreated texture IDs and render-to-texture hand-offs retain
the original allocation path.

### Fast Depth Calculation

Key:

```text
reicast_fast_depth
```

Values:

- `disabled` (default): retains Flycast's per-fragment logarithmic depth;
- `enabled`: uses the original experimental linear per-vertex approximation;
- `vertex_log`: calculates logarithmic depth per vertex;
- `vertex_fast_log`: uses the lower-cost logarithmic per-vertex approximation;
- `menu_guarded`: aggressive profile that uses `vertex_fast_log` during
  moving gameplay and restores accurate per-fragment depth for menu-sized
  scenes, font-like interface scenes and stable paused scenes. It maximizes
  gameplay performance but can show rectangular projected shadows.
- `menu_guarded_shadow_safe`: adds the same menu/pause guard and restores
  accurate depth only for opaque shadow receivers whose vertex-depth range
  exceeds 4x. This avoids the rectangular projected-shadow artifacts seen in
  Sonic Adventure 2 while keeping ordinary gameplay on the fast path.

On the RG351V Sonic Adventure 2 state, the final
`menu_guarded_shadow_safe` plus
`reicast_opaque_strip_merge = enabled` profile presented every frame at 30.35
FPS versus 23.24 FPS for the previous shadow-safe profile (`+30.6%`). Shadows,
menus, audio and gameplay passed manual review. The older depth-only
`menu_guarded` experiment remains available for reproducibility but was both
slower in the final comparison and visually incorrect. The original DOA test
flashed, but the later `vertex_fast_log` profile with fog and mipmapping
disabled passed manual video, audio and input review.
Soul Calibur needs its separate translucent HUD ordering guard described below. See
[the compatibility notes](docs/LOW_END_COMPATIBILITY.md).

### Audio Mixer

Key:

```text
reicast_audio_mixer
```

Values:

- `accurate` (default): uses the original Flycast AICA path;
- `lowend`: uses the deliberately simplified low-end mixer.

The low-end path reduces AICA work by omitting envelopes, filters, LFO, pan,
DSP routing and CD audio. It is intended for games where CPU time matters more
than exact audio reproduction. The user approved it in Sonic Adventure 2 but
reported lower audio quality in Dead or Alive 2 Europe, so it should also be
selected per game.

The independent `reicast_aica_arm_cycles` option underclocks only the emulated
AICA ARM7 sound CPU. Its accurate default is `32`; experimental values are
`24`, `16`, `12` and `8`. Lower values can delay or break sound-driver timing.
They remain disabled by default and require an audible per-game test.

### Adjacent Render-State Elision

Key:

```text
reicast_adjacent_state_elision
```

Values:

- `disabled` (default): preserves the original renderer path.
- `enabled`: skips repeated GLES state setup when two adjacent polygons have
  an exact state match.

This mode is designed to preserve rendering semantics, but the measured
performance difference on the current RK3326 test scenes is small and can be
within normal run-to-run variance. It is retained as an experimental option
for broader per-game testing.

### Translucent Strip Merge

Key:

```text
reicast_translucent_strip_merge
```

Values:

- `disabled` (default): preserves the original translucent rendering order.
- `menu_guarded`: uses the same fast pre-sort, but keeps likely 2D
  menu/overlay strips as separate draw calls. Detection combines short
  screen-aligned geometry, nearly constant depth, overlay depth state,
  screen-edge placement, coverage and overlap. This is conservative and may
  recover less performance than the aggressive mode.
- `inaccurate`: aggressively merges compatible translucent strips to reduce
  GLES draw submissions, preserving the previous experimental behavior.

The guarded detector is intentionally configurable without rebuilding:

```ini
reicast_translucent_menu_guard_strategy = scored
reicast_translucent_menu_guard_max_vertices = 8
reicast_translucent_menu_guard_risk = 5
reicast_translucent_menu_guard_depth_tolerance = 0.0001
reicast_translucent_menu_guard_overlap = risky
reicast_translucent_menu_guard_draw_sorting = standard
```

`strategy = top_hud_last` is an experimental fighting-game profile. It
classifies wide translucent geometry confined to the upper screen band once,
sorts ordinary world transparency through the fast path, and submits the HUD
last in its original order. On the repeatable Soul Calibur state, two paired
1,200-frame tests were 20.3% and 26.1% faster than the correct-order control
(23.2% aggregate), with no skipped frames. The stricter 2,400-frame final pair
was 49.781 versus 42.623 seconds, or 16.79%; the later part of the scene reduces
the gain. It still requires the final manual health-bar review before being
classified as compatible, and must not yet be described as a stable 20% gain.

For a menu not detected by the default profile, first try
`max_vertices = 16`, then `strategy = flat`. `strategy = all_short` is the
broadest diagnostic mode: it is useful to confirm that retaining the suspected
draw boundaries fixes the menu, but it can give back much of the performance
gain. If geometric protection is insufficient, use
`draw_sorting = per_triangle`: this retains guarded strip preprocessing while
submitting translucent geometry through the compatibility draw path. Once the
relevant geometry is identified, reduce the scope again.

This is the main low-end performance option. Tests on an AmberELEC RG351V
showed scene-dependent gains around 12–18%, but it can break transparency,
menus or PowerVR ordering. Enable it per game only after visual testing.

See [the compatibility notes](docs/LOW_END_COMPATIBILITY.md) for the current
device and game observations.

### SH4 CPU Clock

Key:

```text
reicast_sh4clock
```

Values range from `50` to `400` MHz, with the accurate Dreamcast default at
`200` MHz.

- `200` (default): preserves the original Flycast 2021 decoder path exactly.
- Other values: experimentally underclock or overclock the emulated SH4 and
  may change game timing, compatibility or performance.

This option is retained for per-game experimentation. It is not enabled
automatically and the RG351V Sonic Adventure 2 benchmark did not show a
performance benefit from underclocking. See
[the SH4 clock notes](docs/SH4_CLOCK_EXPERIMENT.md) for measurements and
compatibility details.

The development patches, including rejected diagnostics and superseded
renderer experiments, are preserved in the
[patch archive](docs/PATCH_ARCHIVE.md).

## Performance investigation

The RG351V investigation is documented in the
[reproducible baseline](docs/PERFORMANCE_BASELINE.md),
[profiler guide](docs/PROFILING_GUIDE.md),
[AArch64 dynarec report](docs/AARCH64_DYNAREC_PROFILE.md), and
[texture profile](docs/TEXTURE_PROFILE.md), and
[next-candidate report](docs/NEXT_OPTIMIZATION_REPORT.md). Raw profiles and
alternating-run results are under `benchmarks/`.

Development-only build switches currently include:

```sh
LOWEND_PROFILING=0
LOWEND_TEXTURE_SUBIMAGE=1
```

`LOWEND_TEXTURE_SUBIMAGE=1` compiles the accepted storage-reuse path and its
runtime option by default. Set it to `0` for an emergency build-time opt-out.
Palette/fog lookup reuse is included as the runtime option
`reicast_palette_fog_storage_reuse` and is disabled by default. Direct SH4
`LDS FPSCR` decoding is included as the runtime option `reicast_sh4_fpscr`,
also disabled by default; restart the content after changing it. Opaque-strip
state grouping is exposed as `reicast_opaque_strip_merge`, disabled by
default. The profiler remains disabled in release builds. The rejected DIV1,
texture-shadow, PAL4, transient-discard, multi-draw, SH4-timeslice and
ARM7-batching implementations were removed from the production source; their
reports and raw benchmark evidence remain archived.

## Suggested low-end baseline

Start with the accurate path:

```ini
reicast_texture_storage_reuse = enabled
reicast_palette_fog_storage_reuse = disabled
reicast_sh4_fpscr = disabled
reicast_aica_arm_cycles = 32
reicast_adjacent_state_elision = disabled
reicast_opaque_strip_merge = disabled
reicast_translucent_strip_merge = disabled
reicast_sh4clock = 200
```

For a game already verified with the faster translucent path:

```ini
reicast_sh4clock = 200
reicast_adjacent_state_elision = disabled
reicast_translucent_strip_merge = menu_guarded
reicast_translucent_menu_guard_strategy = all_short
reicast_translucent_menu_guard_max_vertices = 64
reicast_translucent_menu_guard_risk = 5
reicast_translucent_menu_guard_depth_tolerance = 0.01
reicast_translucent_menu_guard_overlap = all
reicast_translucent_menu_guard_draw_sorting = per_triangle
```

If the guarded mode is visually correct but not fast enough, test
`inaccurate` per game. Do not make either experimental merge mode a global
default. A setting that works well for one Dreamcast title can damage another
title's menus.

The tested product numbers and proposed per-content profiles are:

| Product number | Title / status | Important overrides |
| --- | --- | --- |
| `MK-51117` | Sonic Adventure 2; gameplay, menus, audio and projected shadows manually approved | `fast_depth = menu_guarded_shadow_safe`, low-end mixer and opaque merge on; 30.35 versus 23.24 FPS (`+30.6%`) |
| `RDC-0140`, `RDC-0149` | Dead or Alive 2 observed CDI variants; manually approved | `fast_depth = vertex_fast_log`, fog/mipmapping off, opaque merge on, EGL D24S0 |
| `T1401D  50` | Soul Calibur; compatibility profile visually correct, faster alternative still has an intermittent HUD defect | Validated: `draw_sorting = per_triangle`, fog/mipmapping on and opaque merge off. Performance: `strategy = top_hud_last`, fog/mipmapping off, opaque merge on and EGL D24S0 |
| `MK-51035` | Crazy Taxi; no repeatable faster candidate retained | Keep the accurate control profile |

These are documentation keys, not hard-coded automatic overrides. A different
region or revision can have a different product number and must be validated
separately.

## Building the libretro core

Use the same toolchain and platform flags as the target distribution. A
generic AArch64 libretro build is:

```sh
make clean
make platform=arm64 FORCE_GLES=1 HAVE_LTCG=0 -j$(nproc)
```

For a 32-bit ARM userspace on a Cortex-A35 device:

```sh
make clean
make platform=classic_armv8_a35 FORCE_GLES=1 -j$(nproc)
```

The Makefile produces `flycast_libretro.so`. AmberELEC performs the documented
Flycast 2022 Low-End identity and filename conversion in `package.mk`; dArkOS
should install the unmodified identity under the filename expected by its
selected 64-bit or 32-bit frontend. A 64-bit core cannot be loaded by
`retroarch32` or `retrorun32`, and a 32-bit core cannot be loaded by their
64-bit counterparts.

## Project status

- Primary target: low-power OpenGL ES handhelds.
- First validated platform: AmberELEC on RK3326/Cortex-A35/Mali-G31.
- Renderer defaults: accurate/original.
- Inaccurate optimizations: opt-in and expected to need a per-game
  compatibility list.
- Save-state compatibility with other Flycast builds is not guaranteed.

Benchmark results are workload-specific. A higher rendered FPS result does not
by itself prove that audio, transparency, menus and save states remain correct.

## License and attribution

The original reicast/Flycast files retain their GPL-2.0-or-later notices and
the GPLv2 text in [LICENSE](LICENSE). This fork also contains the optional
Redream-derived low-end AICA path documented in
[NOTICE-REDREAM.md](NOTICE-REDREAM.md). A combined distribution containing
that path is distributed under GPLv3; the complete GPLv3 text is already
included at `core/deps/picotcp/LICENSE.GPLv3`.

Flycast, reicast, libretro and Redream attributions remain with their
respective authors.

Code contributed to this fork is not bound by the Individual Contributor
License Agreement of the upstream reicast repository and must not be treated
as an upstream contribution unless its author explicitly submits it there.
