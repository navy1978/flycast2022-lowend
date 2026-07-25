# Flycast 2021 experiment archive

`patches/flycast2021-experiments-2026-07-23.zip` preserves the 28 Flycast and
Flycast 2021 patches produced during the RK3326 optimization work. The archive
also contains `PATCHES-SHA256.txt`, with a SHA-256 digest for every patch.

Archive SHA-256:

```text
c1c40fa2dc29c6d9ee9c730743340609fcdf0242d53a0033b71287fcd36d4719
```

## Integrated in this fork

The implementation at commit `aaf6d44c` contains the complete result of
`flycast2021-configurable-render-options.patch`:

- `reicast_adjacent_state_elision`, disabled by default in a direct build;
- `reicast_translucent_strip_merge`, disabled by default in a direct build, with
  `menu_guarded` and `inaccurate` opt-in modes;
- configurable menu-guard heuristics for translucent strip merging;
- exact-state checks including the second texture ID;
- the corrected original translucent depth order before index generation;
- RGB565 low-end compatibility.

AmberELEC packaging changes the `reicast` prefix to `flycast2021`, so the first
two keys are exposed there as `flycast2021_adjacent_state_elision` and
`flycast2021_translucent_strip_merge`. The archived patch names retain their
historical Flycast 2021 naming.

The configurable patch therefore supersedes these earlier standalone patches:

- `flycast2021-adjacent-state-elision-experimental.patch`;
- `flycast2021-stripmerge-orderfix-experimental.patch`;
- `flycast2021-amberelec-lowend-compat.patch`;
- the earlier translucent merge/batching variants.

Commit `009effe7` documented a distribution-specific `Flycast 2021` identity
directly in the source. The current source restores the base revision's
`reicast` option prefix and `Flycast` library name; AmberELEC still applies its
Flycast 2021 identity during packaging. The renderer implementation is
unchanged by this naming correction.

## Preserved as experimental evidence

The remaining patches document profiler instrumentation and candidates tested
during development, including lazy uniforms, GL statistics, state caches,
buffer streaming, AICA/audio experiments, primitive restart and earlier
translucent batching strategies. They are archived for reproducibility and
future investigation, but are not active in the shipping source.

Several candidates measured neutral or worse performance, while some
translucent variants produced visual regressions. Their presence in the
archive does not imply that they should be applied together or enabled by
default.

The 2026-07-24 archive update adds
`flycast2021-sh4clock-option-experimental.patch`. Its 200 MHz default preserves
the original decoder path; other values deliberately change emulated SH4
timing and must remain opt-in. See `SH4_CLOCK_EXPERIMENT.md`.

## Compatibility audit

The fork is based directly on `4c293f30`. None of the accepted renderer
changes modifies `core/serialize.cpp`, the serialization version, AICA state,
Maple state or SH4 state. Direct and dArkOS builds use the base revision's
`Flycast` library name; AmberELEC packages the same source as `Flycast 2021` to
preserve its core-specific save-state and override naming.

The two low-end renderer options preserve the original path when disabled.
Save-state compatibility still requires an on-device regression test before a
release, because frontend naming and runtime state cannot be proven solely by
source comparison.

## Build verification

The historical `009effe7` AmberELEC-identity candidate was rebuilt on the
ARM64 Ubuntu build VM with the established Cortex-A35 flags:

```text
-mcpu=cortex-a35+crc+fp+simd+crypto -mabi=lp64
-mno-outline-atomics -fomit-frame-pointer -Ofast
```

The build completed successfully and produced an AArch64 shared object
containing the `flycast2021_*` option keys and the `Flycast 2021` identity.
This verifies that historical artifact; current direct builds intentionally
use `reicast_*` and `Flycast` instead:

```text
29199c2456f90b94d2541a86d8b85a138cf3e595dc971903be86db8e964cdd84
```
