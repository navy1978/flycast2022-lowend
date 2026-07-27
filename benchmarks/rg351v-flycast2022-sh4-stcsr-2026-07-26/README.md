# Direct `STC.L SR,@-Rn` screen

This directory contains the RG351V Sonic Adventure 2 fixed-frame A/B used to
screen a Redream-style direct dynarec translation of `STC.L SR,@-Rn`.

Both settings used the same candidate core, save state, 640x480 configuration,
Fast Depth, low-end audio mixer, locked device frequencies, and full DRM reset.
Only the experimental direct translation changed.

| Setting | 600-frame runs | Mean | Effective FPS |
|---|---:|---:|---:|
| Interpreter fallback | 21.865 s, 21.981 s | 21.923 s | 27.369 |
| Direct dynarec | 22.243 s, 22.515 s | 22.379 s | 26.811 |

The direct path was **2.04% slower**. All runs presented all 600 frames with
zero skipped or duplicated frames and loaded the same save correctly.

Decision: reject and remove the implementation.
