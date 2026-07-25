# Flycast 2021 fork optimization audit

Date: 2026-07-24

Base used by Flycast 2022 Low-End:

```text
4c293f306bc16a265c2d768af5d0cea138426054
```

Naming note: the candidate binaries recorded in this audit used the
AmberELEC-style `flycast2021_*` option prefix and `Flycast 2021` library name.
The current source keeps the base revision's `reicast_*`/`Flycast` identity;
AmberELEC recreates the recorded identity during packaging.

## Method

The GitHub fork networks for the historical libretro Flycast repository and
current Flycast repository were enumerated. Candidate heads were grouped by
ancestry and inspected for ARM, AArch64, GLES, renderer, texture, audio,
dynarec and low-end changes.

Changes were rejected before building when they were:

- platform-specific to Vita, Switch, Windows or ARM32;
- build flags/default changes without removing measured work;
- dependent on later renderer/dynarec architecture;
- known to alter correctness globally;
- already represented by one of the 27 archived experiments;
- already measured neutral or slower in the RK3326 campaign.

## Historical fork heads inspected locally

The audit repository contains the following remote heads:

```text
candidates/cmitu
candidates/flycast-switch
candidates/manzolo
candidates/mastag
candidates/metallic77
candidates/morpheus
candidates/psc
candidates/rg351p
candidates/rinnegatamante-old
candidates/stormed
candidates/sumavision-metallic77
candidates/supervised
candidates/tunip3
candidates/vanfanel
candidates/vita
candidates/windows-rt
```

## Findings

### No source delta or no relevant target delta

- `metallic77` and `stormed` resolve to the Flycast 2021 base.
- `cmitu` and `manzolo` add README/libnx maintenance, not RK3326 runtime work.
- `mastag`, `supervised` and `vanfanel` are behind the selected base.
- `rg351p` adds old device rumble support, not emulator performance.
- `flycast-switch`, `tunip3`, `psc` and `windows-rt` contain broad platform or
  later-upstream histories rather than isolated RK3326 optimizations.

### Morpheus / Sumavision Xtreme

The apparent low-end work consists primarily of:

- platform compiler targets;
- lower default resolutions/options;
- disabling post-processing or texture upscaling;
- lowering a cache allocation from 16 to 15 MiB;
- enabling unsafe dynarec behavior;
- an emulated SH4 clock scalar;
- ARM32-specific depth/NEON changes.

The ARM32 changes do not apply to the AArch64 RG351V build. Disabling
post-processing globally and enabling unsafe dynarec behavior are not suitable
defaults. The small cache change has no measured reason to improve throughput.

The useful concept is the SH4 clock option. It was reimplemented
conservatively so that 200 MHz preserves the original decoder path exactly,
with all other values explicitly opt-in. See `SH4_CLOCK_EXPERIMENT.md`.

### Rinnegatamante / Vita

The fork contains many aggressive optimizations, but the relevant-looking
changes are tied to ARM32, VitaGL, Vita allocators, copyless Vita rendering or
Vita memory constraints. They are not portable evidence for Mali-G31 AArch64.

The later upstream commit that allocates 64-bit dynarec registers and avoids
some interpreter fallbacks is genuinely interesting for AArch64. A direct
cherry-pick was attempted and aborted: it conflicts across the ARM64
recompiler, backend interfaces and related refactors. It is not a safe
standalone backport and would require a dedicated multi-commit port plus
instruction-level validation.

### Current upstream ideas

- Static quad buffers affect small VMU/crosshair/frontend quads and are not a
  priority for the measured Sonic scene.
- Later texture-cache and presentation work depends on substantial renderer
  refactoring.
- Present-queue changes do not map to the old renderer architecture.
- Vita copyless rendering is platform-specific.
- Current upstream SH4 clock control is the one small concept that can be
  isolated without changing serialization.

## Candidate produced

```text
/home/navy78/flycast-candidates-20260724/flycast2021_lowend_sh4clock_a35_libretro.so
SHA-256: b6df075010de446d953515bcfbbd3a2e728f923d66fec93fcde9944c4ec2dd7b
```

The historical candidate is AArch64, identifies as `Flycast 2021`, and
contains:

```text
flycast2021_sh4clock
flycast2021_adjacent_state_elision
flycast2021_translucent_strip_merge
```

The VM source was restored and left clean after the build.

## Conclusion

The fork audit did not reveal a hidden safe renderer patch likely to deliver a
large accurate gain beyond the work already archived and measured. The SH4
clock candidate is worth testing as an optional game-compatibility/performance
hack, but it must not be reported as a faster accurate emulator.

The substantial AArch64 dynarec register-allocation work remains the only
high-upside untested source direction found in later code. It is a separate
engineering project, not a cherry-pick candidate.
