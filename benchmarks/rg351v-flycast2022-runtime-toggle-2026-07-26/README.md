# RG351V texture-storage runtime toggle verification

Date: 2026-07-26

The integrated profiled core was run three times with a full EmulationStation
stop/start/stop DRM reset before every test:

- `reicast_texture_storage_reuse = enabled`: compatible updates used
  `glTexSubImage2D` and reported texture-storage reuse;
- `reicast_texture_storage_reuse = disabled`: every observed update used
  `glTexImage2D`, with zero subimage calls and zero storage reuses;
- option absent: behavior matched `enabled`, proving the default.

Each run exited normally. A separate non-profiled integrated-core smoke test
also loaded Marvel vs. Capcom 2 and exited without crash, assertion,
debug-break or GLES error markers.

The configs, logs and JSON summaries are stored beside this file.
`toggle-evidence.tar.gz` has SHA-256:

```text
2a6c4cd2768fffcd76956fc14894e90566c83b9480f577ecc7eb9fcc0f41c0d5
```
