# Direct `LDS.L @Rn+,FPSCR` screen

This directory contains the RG351V Sonic Adventure 2 fixed-frame A/B used to
screen a Redream-style direct dynarec translation of
`LDS.L @Rn+,FPSCR`.

Both settings used the same candidate core, save state, 640x480 configuration,
Fast Depth, low-end audio mixer, locked device frequencies, and full DRM reset.
Only the experimental direct translation changed.

## Results

| Test | Interpreter fallback | Direct dynarec | FPS change |
|---|---:|---:|---:|
| Three 600-frame runs, mean | 22.017 s | 21.198 s | +3.87% |
| One 1200-frame run | 46.622 s | 46.713 s | -0.19% |
| All 3000 frames combined | 112.673 s | 110.306 s | +2.15% |

The short runs were promising but inconsistent with the longer validation.
All runs presented every requested frame with zero skipped or duplicated
frames and loaded the same save correctly.

Decision: reject as too small and unstable; remove the implementation.
