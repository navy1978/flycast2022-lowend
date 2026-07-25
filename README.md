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

## Suggested low-end baseline

Start with the accurate path:

```ini
reicast_adjacent_state_elision = disabled
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

The Makefile produces `flycast_libretro.so`. AmberELEC performs its established
Flycast 2021 identity and filename conversion in `package.mk`; dArkOS should
install the unmodified identity under the filename expected by its selected
64-bit or 32-bit frontend. A 64-bit core cannot be loaded by `retroarch32` or
`retrorun32`, and a 32-bit core cannot be loaded by their 64-bit counterparts.

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

The project preserves the original history and is distributed under the
GPL-2.0 license in [LICENSE](LICENSE). Flycast, reicast and libretro
attributions remain with their respective authors.

Code contributed to this fork is not bound by the Individual Contributor
License Agreement of the upstream reicast repository and must not be treated
as an upstream contribution unless its author explicitly submits it there.
