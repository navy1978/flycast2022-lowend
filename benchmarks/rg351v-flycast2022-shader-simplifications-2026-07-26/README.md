# RG351V shader simplification screening

This directory records the rejected nearest-texture and flat-shading
experiments performed after the accepted Fast Depth result.

All runs used:

- Anbernic RG351V, RK3326/Cortex-A35 and Mali-G31;
- the same AArch64 core binary, SHA-256
  `2e30f72cbb98cae8abf0c94da0f2c42727feafb2e0b5f9f8b88ec4b15b719eae`;
- the compatible Sonic Adventure 2 automatic state, product `MK-51117`;
- 640x480, no frontend or core frameskip;
- threaded rendering enabled and synchronous rendering disabled;
- Texture Storage Reuse, Fast Depth and the low-end audio mixer enabled;
- a full DRM reset before every run;
- 600 core frames after warm-up.

The control left both new shader experiments disabled. `fast_texture` forced
nearest-neighbour sampling and bypassed Flycast's trilinear path.
`flat_shading` suppressed Gouraud interpolation. `combined` enabled both.

| Variant | Runs (seconds / 600 frames) | Mean seconds | Mean FPS | FPS change |
| --- | --- | ---: | ---: | ---: |
| Control | 22.029, 22.095, 21.794 | 21.972667 | 27.306654 | reference |
| Fast texture | 22.655 | 22.655000 | 26.484220 | -3.01% |
| Flat shading | 21.628, 22.056 | 21.842000 | 27.470012 | +0.60% |
| Combined | 21.026, 22.450 | 21.738000 | 27.601435 | +1.08% |

Every JSON result reports 600 presented frames, zero duplicated frames and
zero skipped frames. The first combined run looked promising, but its repeat
was slower than the control range. The mean signals are too small or negative
and do not justify the visible quality loss. Both options were therefore
removed from the working source rather than retained as user-facing settings.
