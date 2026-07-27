# RG351V empirical wave 1

Date: 2026-07-26

All fixed-frame runs use the Flycast 2021-compatible Sonic Adventure 2 save,
five seconds of warm-up, exactly 600 core frames, locked CPU/DMC/GPU
frequencies and the full DRM reset procedure. The frontend supporting
`--benchmark-frames` is used so every candidate executes the same number of
core frames.

## Fixed-frame result

| Candidate | Result | Decision |
| --- | ---: | --- |
| Cortex-A35 compiler flags | +0.16% in the fixed run | Reject: neutral |
| transient depth/stencil discard | +0.43% mean across three paired runs | Reject: pairs crossed zero |
| A35 plus discard | +0.59% in the fixed run | Reject: no additive signal |
| SH4 `LDS FPSCR` direct decode | +0.79% mean across three paired runs | Retain default-off for review |
| SH4 timeslice 448 -> 896 | -0.10% | Reject: neutral |
| AICA ARM7 batch 2 | +0.83% in one run | Reject: batch 4 did not scale |
| AICA ARM7 batch 4 | -0.27% | Reject: slower and higher timing risk |

The three transient-discard pair deltas were +1.72%, -0.41% and -0.05% when
expressed as completion-time savings. The apparent +5-6% seen in the first
time-based runs was not repeatable.

The three direct `LDS FPSCR` FPS pair deltas were -0.26%, +1.93% and +0.71%.
The mean core time fell from 25.884 to 25.574 ms in the paired subset. This is
a small signal, not an accepted optimization: Sonic is the best observed case
for this instruction, with roughly 10,776 `406a` fallbacks per profiler
window, so a large gain in the other profiled games is unlikely.

`results.csv` contains the aggregate metrics. Individual JSON and log files
are retained in this directory. The original device archive is
`flycast2022-empirical-wave1-results.tar.gz`, SHA-256:

```text
c83ca1a40b1c3f87d24e32293d6b19666e0c49b686996e848f7376d2f1ca6e9d
```

## User-requested manual candidates

Two candidates are deliberately preserved despite not being selected by the
automatic FPS screen:

- RetroRun `audio-thread`: Sonic FPS was neutral/slightly lower, while audio
  underruns fell from 29 to 19 in the first matched listening scene. The user
  reported that it may sound better.
- palette/fog lookup storage reuse: the first Sonic run was approximately
  neutral (+0.87%) and needs another visual check for palette and fog
  correctness.

They are installed under
`/storage/retrorun-test/flycast2022-manual-followup` with a launcher that
performs the full DRM reset. See the separate manual-followup README.

No source commit or push was made.

## Final clean review build

After removing transient discard, multi-draw, SH4 timeslice and ARM7 batching
from the source, the default ARM64 review build completed successfully:

```text
SHA-256: 56c9551ebd3ac5afa513fe84967fa51e5eb3af6d825f60dc256044b4b06a6140
Build ID: 69cde3c36508543ff33ba0251118d7f9d3c4c9df
```

`final-review-smoke.json` records a successful 300-frame Sonic run using the
compatible save. The log contains no invalid-state, debugbreak, GL-error or
failure marker.
