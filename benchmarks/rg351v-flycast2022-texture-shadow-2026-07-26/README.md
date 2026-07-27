# RG351V exact texture source shadow experiment

Date: 2026-07-26

The candidate retained exact protected VRAM bytes and palette identity, then
used `memcmp` after invalidation to avoid identical decode/upload work. It was
compiled with `LOWEND_TEXTURE_SHADOW_SKIP=1`; DIV1 was disabled.

Profiler counters proved that the mechanism removed hundreds to thousands of
identical updates per reporting window. The first five 60-second alternating
wall-time pairs were positive in aggregate but one pair reversed strongly.

To remove unequal frame counts, a temporary uncommitted RetroRun diagnostic
option stopped after exactly 2,400 core frames with a 180-second safety
deadline. All ten JSON files contain `requested_core_frames=2400` and
`core_frames=2400`.

Fixed-frame aggregates:

| Metric | Baseline | Candidate | Change |
| --- | ---: | ---: | ---: |
| Mean throughput | 42.429 FPS | 44.305 FPS | +4.42% |
| Mean core time | 11.909 ms | 11.189 ms | -6.05% |
| Mean active p95 | 33.647 ms | 31.912 ms | -5.16% |
| Mean active p99 | 46.414 ms | 43.372 ms | -6.55% |
| Audio underruns | 34 | 35 | +1 |

Three pairs improved and two regressed. The MVC2 state does not produce an
identical dynamic path across launches, so variance still overlaps the claimed
gain.

Decision: **Revise; not accepted as an FPS improvement**. The option remains
disabled and was not combined with the later subimage experiment.
