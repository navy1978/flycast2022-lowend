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

AmberELEC historically installs this 2022 core alongside modern Flycast. Its
`flycast2021/package.mk` performs three packaging transformations:

1. `CORE_OPTION_NAME` changes from `reicast` to `flycast2021`;
2. the libretro library name changes from `Flycast` to `Flycast 2021`;
3. `flycast_libretro.so` is installed as `flycast2021_libretro.so`.

Keep those transformations in the AmberELEC package. When switching the
package to this fork, only its source URL, commit and archive checksum need to
change. The resulting low-end option keys are:

```text
flycast2021_sh4clock
flycast2021_adjacent_state_elision
flycast2021_translucent_strip_merge
flycast2021_translucent_menu_guard_strategy
flycast2021_translucent_menu_guard_max_vertices
flycast2021_translucent_menu_guard_risk
flycast2021_translucent_menu_guard_depth_tolerance
flycast2021_translucent_menu_guard_overlap
flycast2021_translucent_menu_guard_draw_sorting
```

This preserves existing AmberELEC RetroArch and RetroRun configurations and
keeps their core-specific save-state/override naming unchanged.

## dArkOS

dArkOS configurations use the original `reicast_*` option prefix. Build this
fork without applying the AmberELEC identity transformations. The low-end keys
will therefore be:

```text
reicast_sh4clock
reicast_adjacent_state_elision
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
