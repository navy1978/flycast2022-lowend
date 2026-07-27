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

The source repository for this low-end core is:

```text
https://github.com/navy1978/flycast2022-lowend
```

This fork already reports rumble through the standard libretro rumble
interface. RetroRun handles the device-specific PWM/event output, so dArkOS
does not need to patch Flycast with direct `/sys/class/pwm` shell commands.
If the launcher expects `flycast_rumble_libretro.so`, packaging may install
the same built core under that filename.

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

For the 64-bit RK3566 devices, use a clean, deterministic build:

```sh
make clean
make platform=RK3566 FORCE_GLES=1 HAVE_OPENMP=1 HAVE_LTCG=0 -j$(nproc)
```

For the validated PGO-AICA build, use the bundled, path-independent profile
wrapper instead:

```sh
make rk3566-pgo JOBS="$(nproc)"
```

This command verifies the six versioned profiles, stages them for the current
absolute checkout path, performs a clean RK3566/Cortex-A55 build and produces
`flycast_libretro.so`. It requires GCC 9.x because GCC execution profiles are
compiler-version-specific. The startup log must additionally contain:

```text
flavor: pgo-optimized
```

ArkOS/dArkOS must not run the training core and does not need a Sonic ROM.
The repository contains only compiler execution counters. Packaging may
install the resulting core as `flycast_rumble_libretro.so`, while preserving
the `reicast_*` option prefix and `Flycast` library name.

Use the same command after applying any rumble patch. In particular, do not
incrementally rebuild a previous `platform=RK3566` object tree with
`platform=goadvance`: Make does not track changed platform flags in object
filenames, so that produces a hybrid binary. The startup log must contain:

```text
Build target: RK3566-cortex-a55
```

An RG353M comparison using a 30-second warm-up and 90-second measurement found
47.287 FPS with this target versus 41.886 FPS with the generic ARM64 target in
Sonic Adventure 2 (`+12.9%`). Soul Calibur reached 58.860 FPS with the current
RetroRun catalog profile. The bundled AICA profile raises Sonic from 47.380
to 50.913 FPS (+7.46%) without a measurable Soul Calibur regression. From the
same combat save-state, Dead or Alive 2 reaches 43.685 FPS versus 36.620 FPS
for the original dArkOS stack and distribution configuration (+19.3%).

## Adding another distribution

Prefer a packaging-time identity conversion over a distribution-specific
source branch. Keep the emulator changes shared, then document these three
values for the package:

1. core option prefix;
2. libretro library name;
3. installed shared-object filename and ABI.
