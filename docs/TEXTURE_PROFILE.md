# Texture decode and upload profile

Date: 2026-07-26

The Phase 5 texture profiler is compiled only with
`FLYCAST_LOWEND_PROFILING`. Normal builds contain neither the counters nor the
diagnostic source hash.

## Instrumented paths

| Area | Location | Measurements |
| --- | --- | --- |
| Cache lookup | `core/rend/gles/gltex.cpp` | lookups, hits, misses and creates |
| VRAM/palette invalidation | `core/rend/TexCache.cpp` | dirty texture and palette events |
| Decode | `core/rend/TexCache.cpp` | source/decoded bytes, layout, pixel format and time |
| Diagnostic identity check | `core/rend/TexCache.cpp` | identical reuploads and separately timed hash cost |
| GLES upload | `core/rend/gles/gltex.cpp` | bytes, format, time and GL upload calls |

Reports use:

```text
[LOWEND_TEXTURE_CSV] counter,value
[LOWEND_TEXTURE_FORMAT_CSV] operation,format,count,bytes,total_ms,avg_ms
```

The per-format decode names describe the Dreamcast source format. Upload names
describe the converted GLES format. Counters reset at each profiler report, so
events at a report boundary can appear in the adjacent window.

## Build and benchmark configuration

```sh
make clean
make platform=arm64 FORCE_GLES=1 HAVE_LTCG=0 \
  LOWEND_PROFILING=1 LOWEND_TEXTURE_SUBIMAGE=0 AS=cc -j4
```

The RG351V runs used the exact configuration archived at
`benchmarks/rg351v-flycast2022-profiler-2026-07-26/`:

- configuration SHA-256:
  `31dfed80909c20fe30feb80d2c15079641c9559ff4740344f4bdeb069b52365b`;
- 640x480, dynarec, SH4 200 MHz, threaded core rendering;
- frameskip disabled, mipmapping enabled and texture upscale disabled;
- custom texture loading/dumping disabled;
- complete EmulationStation stop/start/stop DRM reset before every run.

## Results by workload

### Marvel vs. Capcom 2

This is the useful texture-heavy workload. Stable 60-frontend-frame windows
recorded 2,304 to 4,120 texture updates. Nearly all steady-state updates were
single-level PAL4 twiddled textures with approximately 512 source bytes and
1,024 converted/uploaded bytes per update.

Across the archived baseline profile:

- 41,187 PAL4 decodes consumed 685.858 ms, or 0.016652 ms per decode;
- 41,226 uploads consumed 1,409.769 ms, or 0.034196 ms per upload;
- steady windows used `glTexImage2D` for every upload;
- no steady window called `glGenerateMipmap`;
- identical source reuploads varied substantially with the dynamic scene.

The upload is therefore both more expensive per operation and called thousands
of times in a short window. Reusing existing GLES storage is the strongest
small candidate supported by these counters.

### Sonic Adventure 2

The stable profile is texture-cache-hit dominated: approximately 62,000 hits
per report and usually zero texture updates after startup. It is a renderer and
draw-submission workload, not a useful texture optimization benchmark.

### Soul Calibur

Stable windows also usually contain no texture updates. A transition window
contained YUV and paletted updates, but no repeated identical reupload signal.
It is retained as a compatibility smoke test.

## Experiment: exact source shadow

The archived `LOWEND_TEXTURE_SHADOW_SKIP=1` experiment retained the exact
protected VRAM bytes and the
relevant palette hash after a real update. After an invalidation it uses
`memcmp`, not a hash, before skipping decode/upload and re-locking the range.
The full protected range includes mip data and a VQ codebook when present.

Profiler runs proved the mechanism: 273 to 1,910 identical updates were skipped
per window and the corresponding decode and GL calls disappeared.

The five-pair fixed-core-frame experiment was not repeatable enough to accept:

| Metric | Baseline | Candidate | Change |
| --- | ---: | ---: | ---: |
| Throughput mean | 42.429 FPS | 44.305 FPS | +4.42% |
| Core time mean | 11.909 ms | 11.189 ms | -6.05% |
| Active p95 mean | 33.647 ms | 31.912 ms | -5.16% |
| Active p99 mean | 46.414 ms | 43.372 ms | -6.55% |

Only three of five pairs improved. MVC2 still followed different dynamic paths
after loading the same state, and the distributions overlapped. Decision:
**Reject**. Its implementation was removed and is not combined with later
tests.

## Experiment: reuse GLES texture storage

`LOWEND_TEXTURE_SUBIMAGE=1` uses `glTexSubImage2D` only for a non-mip-chain
upload whose existing texture has the exact same width, height, GLES format
and GLES type. The first upload and every mismatch still use
`glTexImage2D`. Recreated texture IDs and RTT hand-offs explicitly invalidate
the tracked allocation.

The profile candidate reused storage for 98-100% of steady MVC2 uploads. One
window used 3,718 subimage calls for 3,791 updates; no GL error marker was
reported. Its aggregate observed upload time was 0.029041 ms/update versus
0.034196 ms/update in the dynamic baseline profile. This timing comparison is
diagnostic only; the unprofiled alternating runs provide the FPS evidence.

Five unprofiled 60-second MVC2 pairs produced:

| Metric | Baseline | Candidate | Change |
| --- | ---: | ---: | ---: |
| Mean FPS | 41.722 | 44.832 | +7.45% |
| Median FPS | 40.990 | 45.790 | +11.71% |
| Core time mean | 12.316 ms | 10.355 ms | -15.93% |
| Core p95 mean | 23.296 ms | 20.266 ms | -13.01% |
| Active p95 mean | 34.934 ms | 31.713 ms | -9.22% |
| Active p99 mean | 44.821 ms | 43.403 ms | -3.16% |
| Audio underruns | 31 | 34 | +3 total |

All five pairs improved FPS and core time. Per-pair FPS changes were +16.21%,
+3.10%, +0.73%, +7.97% and +10.26%. A three-pair Sonic regression guard was
neutral within noise: 20.581 versus 20.349 FPS (-1.13%), 28.875 versus
29.433 ms core (+1.93%), and 128 versus 127 total audio underruns. Soul Calibur
and Sonic normal-build smoke tests loaded and exited without crash, assert or
GL error markers.

The final candidate subsequently passed the user's visual and audible RG351V
tests. Decision: **Keep and enable by default**. Release builds compile the
support with `LOWEND_TEXTURE_SUBIMAGE=1` and expose
`reicast_texture_storage_reuse = enabled`; setting the runtime option to
`disabled` restores the original `glTexImage2D` upload path. A build-time
emergency opt-out remains available with `LOWEND_TEXTURE_SUBIMAGE=0`.

## Experiment: PAL4 palette-pair cache

The archived `LOWEND_PAL4_PAIR_CACHE=1` experiment replaced the two palette
lookups performed for each
packed PAL4 source byte with an exact 256-entry pair table. The table is
rebuilt only when an exact comparison detects a change in the relevant
16-entry palette. No hash is used for the correctness decision.

The independent host and AArch64 harness checked every possible packed source
byte for 10,003 palettes and compared more than one million random converted
tiles with the original implementation. All output vectors matched.

The MVC2 profile recorded 82,960 pair-cache hits and 32 exact rebuilds. PAL4
decode time changed from 0.016652 to 0.016115 ms/update, a 3.22% reduction
inside a stage that accounts for only about 0.7 ms per frontend frame in this
scene. The implied whole-frame benefit is well below one percent, so a long
FPS comparison was not justified.

Decision: **Reject for insufficient impact**. Its implementation was removed
and is not combined with the retained storage-reuse candidate.

## Remaining correctness work

The automated runs cover boot, content/state load, rendering, audio production
and clean benchmark exit. The accepted candidate also passed the user's manual
visual and audible RG351V tests. The direct dArkOS-style runtime key and the
AmberELEC-style renamed key have dedicated plumbing checks; a complete
distribution image build remains a release-stage check. Future reports should
still identify any long-session, custom-texture or unusual RTT regression.
