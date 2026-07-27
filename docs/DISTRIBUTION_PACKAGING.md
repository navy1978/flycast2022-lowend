# Distribution packaging

This fork keeps the libretro identity from its `libretro/flycast` base revision
in source code:

```text
Core option prefix: reicast
Libretro library name: Flycast
Makefile output: flycast_libretro.so
```

The option prefix and library name are independent of the shared object's
filename. Renaming only the `.so` does not change the core option keys reported
to a frontend.

## AmberELEC

AmberELEC installs this fork as a new core alongside modern Flycast and the
historical Flycast 2021 package. Its `flycast2022/package.mk` performs three
packaging transformations:

1. `CORE_OPTION_NAME` changes from `reicast` to `flycast2022`;
2. the libretro library name changes from `Flycast` to `Flycast 2022 Low-End`;
3. `flycast_libretro.so` is installed as `flycast2022_libretro.so`.

The resulting low-end option keys are:

```text
flycast2022_texture_storage_reuse
flycast2022_palette_fog_storage_reuse
flycast2022_fast_depth
flycast2022_sh4_fpscr
flycast2022_audio_mixer
flycast2022_aica_arm_cycles
flycast2022_sh4clock
flycast2022_adjacent_state_elision
flycast2022_opaque_strip_merge
flycast2022_translucent_strip_merge
flycast2022_translucent_menu_guard_strategy
flycast2022_translucent_menu_guard_max_vertices
flycast2022_translucent_menu_guard_risk
flycast2022_translucent_menu_guard_depth_tolerance
flycast2022_translucent_menu_guard_overlap
flycast2022_translucent_menu_guard_draw_sorting
```

The separate identity prevents its settings, overrides and save states from
colliding with AmberELEC's existing Flycast and Flycast 2021 cores.
AmberELEC's RetroRun launcher writes
`flycast2022_texture_storage_reuse = enabled` by default and accepts an
explicit disabled setting as the package-level escape hatch. Palette/fog
storage reuse is written as disabled unless explicitly enabled.
Direct SH4 FPSCR decoding is likewise written as disabled unless explicitly
enabled. `fast_depth = menu_guarded` and
`fast_depth = menu_guarded_shadow_safe` require the corresponding values to
be preserved by distribution option validation. Restart the content after
changing Fast Depth or direct SH4 FPSCR decoding.

## dArkOS

dArkOS configurations use the original `reicast_*` option prefix. Build this
fork without applying the AmberELEC identity transformations. The low-end keys
will therefore be:

```text
reicast_texture_storage_reuse
reicast_palette_fog_storage_reuse
reicast_fast_depth
reicast_sh4_fpscr
reicast_audio_mixer
reicast_aica_arm_cycles
reicast_sh4clock
reicast_adjacent_state_elision
reicast_opaque_strip_merge
reicast_translucent_strip_merge
reicast_translucent_menu_guard_strategy
reicast_translucent_menu_guard_max_vertices
reicast_translucent_menu_guard_risk
reicast_translucent_menu_guard_depth_tolerance
reicast_translucent_menu_guard_overlap
reicast_translucent_menu_guard_draw_sorting
```

Keep the libretro library name as `Flycast`. The distribution may copy or
rename `flycast_libretro.so` to the core filename selected by EmulationStation;
that packaging filename does not alter the option prefix.

RetroRun audio threading is a frontend setting, not a Flycast core option:

```text
retrorun_force_audio_multithread = false
```

Keep it false by default. A per-game launcher override may set it to `true`
after manual audio validation.

Build the core for the ABI of the frontend that loads it:

- AArch64 for `retroarch` or `retrorun`;
- 32-bit ARM for `retroarch32` or `retrorun32`.

Do not reuse an AArch64 binary in a 32-bit core slot or the inverse.

## Adding another distribution

Prefer a packaging-time identity conversion over a distribution-specific
source branch. Keep the emulator changes shared, then document these three
values for the package:

1. core option prefix;
2. libretro library name;
3. installed shared-object filename and ABI.
