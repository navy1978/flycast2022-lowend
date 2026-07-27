# Direct `TAS.B @Rn` screen

This directory contains the RG351V Sonic Adventure 2 fixed-frame A/B used to
screen a Redream-style direct dynarec translation of `TAS.B @Rn`.

Both settings used the same candidate core, save state, 640x480 configuration,
Fast Depth, low-end audio mixer, locked device frequencies, and full DRM reset.
Only the experimental direct translation changed.

| Setting | 600-frame runs | Mean | Effective FPS |
|---|---:|---:|---:|
| Interpreter fallback | 22.004 s, 21.849 s | 21.927 s | 27.364 |
| Direct dynarec | 22.107 s, 22.136 s | 22.122 s | 27.123 |

The direct path was **0.88% slower**. All runs presented all 600 frames with
zero skipped or duplicated frames and loaded the same save correctly.

Decision: reject and remove the implementation.
