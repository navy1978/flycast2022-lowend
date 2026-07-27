# RG351V unmatched DIV1 experiment

Date: 2026-07-26

This directory contains the raw Phase 3 candidate evidence. The candidate
generates one unmatched SH4 `DIV1` operation in the AArch64 dynarec instead of
using the interpreter fallback.

## Reproducibility

- Device: Anbernic RG351V, RK3326/Cortex-A35
- OS: AmberELEC `pr-1028-dev-20250320_0944-c75e6e9`
- Frontend: RetroRun 3.1.2, GO2/DRM and GO2 audio
- Content: Sonic Adventure 2, existing automatic save state
- Order: baseline/candidate, repeated five times
- Warm-up: 30 seconds
- Measurement: 60 seconds minimum
- Frame skip: disabled
- Frequencies during each measurement: CPU 1296 MHz, DMC 786 MHz, GPU
  520 MHz

The exact launcher and `retrorun.cfg` are in the sibling directory
`../rg351v-flycast2022-profiler-2026-07-26/`. Their SHA-256 values are:

```text
launcher: 37141833af772be776cd825dbca9edcece42c50b1208811e46e4cbefd1a29a52
config:   31dfed80909c20fe30feb80d2c15079641c9559ff4740344f4bdeb069b52365b
```

The launcher performs the full DRM reset before every run: stop
EmulationStation, start it, wait six seconds, stop it, then wait three seconds.
It locks frequencies for the run and restores them on exit. All ten long-run
JSON files contain one identical `settings` object.

The normal baseline is SHA-256
`ea7f1158d28a39a1eafefa001c8b1e8fd36862009c641336812446d52180a72e`
with Build ID `2dfa8991efdaa09f38778711b684aa64d4fc2525`.
The normal candidate is SHA-256
`7bfd9aa62795702b1e77a2956f0d2d98c39c3902f56928dfa00d487dde8318ab`
with Build ID `97901c59f647e9bcee127e75587d03f3cd1d41ff`.

## Result

| Metric | Baseline | Candidate |
| --- | ---: | ---: |
| Mean FPS | 19.423 | 19.389 |
| Median run FPS | 19.246 | 19.108 |
| Mean core time | 31.844 ms | 31.873 ms |
| Mean active-frame p95 | 55.985 ms | 55.835 ms |
| Mean active-frame p99 | 60.015 ms | 60.286 ms |
| Total audio underruns | 144 | 200 |

The mean FPS change is -0.18%. Per-pair changes cross zero and the ranges
overlap. The candidate is rejected as a performance optimization.

The `*-profile-smoke` files are shorter correctness/counter checks for Sonic
Adventure 2, Soul Calibur and Marvel vs. Capcom 2. `3304` is absent in the
candidate fallback rankings. The first Sonic smoke run encountered an
intermittent black DRM handoff; the same candidate was visible after a fresh
full reset, so only the retry is useful visual evidence.

Temperature was not captured at both endpoints of every long run. That is a
methodology limitation and another reason these data cannot support a positive
acceptance claim. Game images and save states are not included.
