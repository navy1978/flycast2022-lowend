# RG351V PAL4 pair-cache profile

Date: 2026-07-26

This directory contains the device profile used to evaluate
`LOWEND_PAL4_PAIR_CACHE=1`. All other implementation switches were disabled.
The run used the archived exact RetroRun configuration and the complete
EmulationStation stop/start/stop DRM reset.

## Correctness

The independent `tools/pal4_pair_cache_test.cpp` harness checked all 256 packed
PAL4 source values for 10,003 palettes and more than one million random tiles.
It passed on the macOS host and the AArch64 build VM.

The device profile recorded:

- 82,960 exact pair-cache hits;
- 32 exact palette-triggered table rebuilds;
- no crash, assertion or GL error marker.

## Performance result

| Metric | Baseline | Candidate | Change |
| --- | ---: | ---: | ---: |
| PAL4 decodes profiled | 41,187 | 82,992 | workload differs |
| Mean PAL4 decode | 0.016652 ms | 0.016115 ms | -3.22% |

The candidate profile was deliberately longer to cover palette changes. The
per-update saving affects a stage worth only about 0.7 ms per frontend frame
in the baseline MVC2 scene, so its expected whole-frame effect is well below
one percent.

Decision: **Reject for insufficient impact**. No long alternating FPS run was
performed.
