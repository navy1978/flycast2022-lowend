# RG351V Sonic menu and shadow guard

Date: 2026-07-27

Game: Sonic Adventure 2, product number `MK-51117`

All automatic comparisons used the same RetroRun gameplay state, a five-second
warm-up, 600 core frames, locked RG351V CPU/DMC/GPU frequencies and the full
EmulationStation DRM reset before every run. Raw logs and benchmark JSON files
are under `raw/`.

## Visual isolation

| Fast Depth exception | Projected-shadow rectangles | Decision |
| --- | --- | --- |
| Accurate translucent list | Present | Excluded |
| Accurate punch-through list | Present | Excluded |
| Accurate opaque list | Gone, but slower | Confirms the defect is in opaque depth |
| Accurate opaque shadow receivers | Gone, but slower | Correct but too broad |
| Accurate modifier volumes only | Present | Excluded |
| Accurate shadow receivers with vertex-depth ratio above 4x | Gone; user approved speed and image | Selected |

The selected rule is exposed as
`reicast_fast_depth = menu_guarded_shadow_safe`. It retains the existing
low-complexity/font/pause guard and uses accurate fragment depth only when an
opaque PowerVR shadow receiver has `max(abs(z)) > 4 * min(abs(z))`.

## Fixed-frame results

| Run | Mode | Duration | Core avg | Video avg | Duplicated | Skipped |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| control 1 | pure `menu_guarded` | 25.727 s | 34.692 ms | 8.170 ms | 1 | 0 |
| control 2 | pure `menu_guarded` | 25.773 s | 34.778 ms | 8.162 ms | 1 | 0 |
| candidate 1 | diagnostic 4x rule | 28.448 s | 38.853 ms | 8.545 ms | 1 | 0 |
| candidate 2 | diagnostic 4x rule | 26.028 s | 34.894 ms | 8.470 ms | 1 | 0 |
| candidate 3 | diagnostic 4x rule | 25.651 s | 34.195 ms | 8.540 ms | 1 | 0 |
| final smoke | stable `menu_guarded_shadow_safe` name | 27.630 s | 37.470 ms | 8.548 ms | 1 | 0 |

The interleaved control 2/candidate 3 pair slightly favored the candidate
(25.773 versus 25.651 seconds). Using the two stable candidate runs 2 and 3,
the candidate total was 51.679 seconds versus 51.500 seconds for both
controls: a 0.35% aggregate cost. Candidate 1 and the final stable-name smoke
show that runtime variance is material.

Making every opaque shadow receiver accurate completed the valid 600-frame run
in 30.382 seconds. The 4x rule therefore recovers essentially all fast-path
performance in the stable pair while preserving the visual correction.

After removing the diagnostic option names and rebuilding the final public
core, both public modes were retested from the same state:

| Frames | `menu_guarded` | `menu_guarded_shadow_safe` | Aggressive gain |
| ---: | ---: | ---: | ---: |
| 300, pair 1 | 11.631 s | 12.220 s | 5.06% |
| 300, pair 2 | 10.233 s | 11.955 s | 16.83% |
| 600, reversed order | 27.033 s | 28.070 s | 3.84% |

Every run presented every requested frame and skipped zero. The shadow-safe
cost is therefore scene-sensitive: the earlier stable 600-frame aggregates
were nearly neutral, while the final public build showed a clear advantage for
the aggressive mode, especially in the shorter shadow-heavy window. This is
why both values are retained instead of treating them as equivalent.

The final ARM64 core in `artifacts/sonic-menu-guard/` has SHA-256:

```text
6cf4a036bc351b02efbb8e66e31f2e6b5c5c9dd51a49b3f7059c8ab91987c888
```

No source commit or push was made.
