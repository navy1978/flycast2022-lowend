# Configurable SH4 clock experiment

## Purpose

Current Flycast exposes an emulated SH4 clock option. A conservative backport
is included in Flycast 2022 Low-End so different values can be tested per game
without changing the accurate default.

This is a timing hack, not a renderer optimization. It may make a game easier
for the host CPU to emulate, but can also change game timing or cause game
logic problems.

## Compatibility properties

- Direct/dArkOS core option: `reicast_sh4clock`
- Current AmberELEC packaged core option: `flycast2022_sh4clock`
- Exposed values: 50, 75, 100-300 MHz in 20 MHz steps, 350 and 400 MHz
- Default: 200 MHz
- At 200 MHz the original Flycast 2021 decoder path is preserved exactly.
- No serialization structures or version numbers are changed.
- Existing save states should therefore remain structurally compatible, but
  loading old and new states still requires an on-device regression test.
- Existing low-end renderer options remain independent and disabled by
  default.

## Verified integrated build

ARM64 Cortex-A35 build:

```text
/home/navy78/flycast-candidates-20260724/flycast2021_lowend_integrated_sh4clock_a35_libretro.so
SHA-256: 7ed7aca13fabddec62d12b257b4a29d7521b1c566de76af31ab91a2f01f72e52
```

This historical binary was rebuilt with the then-current Flycast 2021
AmberELEC identity. Binary string inspection confirmed that
`flycast2021_sh4clock` and its option label are present. Current AmberELEC
packaging exposes the option as `flycast2022_sh4clock`; a direct or dArkOS
build exposes it as `reicast_sh4clock`.

The original standalone patch is retained in the development archive:

```text
/private/tmp/flycast2021-sh4clock-option.patch
SHA-256: 49f59fa9a67988ff2c674b09d9a7ee9a193d7f5e0904b60a3bf6982bb44a3dba
```

## RG351V result

Sonic Adventure 2 was measured on AmberELEC using the same save state and
frontend configuration:

| SH4 clock | Mean rendered FPS | Mean core time | Mean audio underruns |
|---|---:|---:|---:|
| 200 MHz | 37.013 | 20.586 ms/frame | 203 |
| 180 MHz | 36.818 | 20.702 ms/frame | 205 |

Single runs at 160 and 140 MHz were also slightly slower than the 200 MHz
control. Underclocking is therefore rejected as a performance default for this
workload. The option remains available because other games may react
differently.

## Further test order

Use the same save state, RetroRun settings and in-game camera for each run:

1. 200 MHz control;
2. 180 MHz;
3. 160 MHz;
4. 140 MHz only if 160 MHz remains stable;
5. optional 220 MHz control to confirm the expected opposite trend.

Record:

- emulation speed percentage;
- emulated VBlanks per second;
- rendered FPS;
- audio underruns and audible crackle;
- gameplay speed and input response;
- menu, animation and scripting regressions;
- save-state load and save behavior.

Test a non-default clock in isolation before combining it with translucent
strip merge, so timing and renderer effects remain distinguishable.

## Fork audit context

The option was selected after auditing the known Flycast 2021 fork candidates.
Other apparent low-end changes were not promoted to a build because they were
ARM32/Vita-specific, changed defaults without removing work, disabled
post-processing globally, enabled unsafe dynarec behavior, or depended on
large later refactors. The modern 64-bit register-allocation change in
particular does not cherry-pick cleanly onto Flycast 2021 and is not a safe
standalone candidate.
