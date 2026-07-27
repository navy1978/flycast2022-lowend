# Low-end profiler guide

The profiler is a diagnostic build feature. It is disabled by default and
does not add profiler calls or profiler strings to the normal binary generated
by the documented Cortex-A35 build.

## Building

Always clean when changing `LOWEND_PROFILING`. Make does not encode compiler
defines into object filenames, so an incremental build could otherwise reuse
objects from the opposite mode.

Normal build:

```sh
make clean
make platform=arm64 FORCE_GLES=1 HAVE_LTCG=0 LOWEND_PROFILING=0 -j4
```

Profiling build:

```sh
make clean
make platform=arm64 FORCE_GLES=1 HAVE_LTCG=0 LOWEND_PROFILING=1 -j4
```

This defines `FLYCAST_LOWEND_PROFILING`. Reports go to standard error. Set the
report interval in frontend frames with:

```sh
FLYCAST_LOWEND_PROFILE_INTERVAL=1800 frontend core game
```

The default is 1800 and the minimum accepted value is 60. A short smoke test
can use 60; longer measurements should use a larger interval to reduce report
I/O.

## Output

Frame-stage rows use:

```text
[LOWEND_PROFILE_CSV] stage,samples,calls,avg_ms,min_ms,max_ms,p50_ms,p95_ms,p99_ms
```

Dynarec totals use:

```text
[LOWEND_DYNAREC_CSV] counter,value
[LOWEND_DYNAREC] top_interpreter_fallbacks #1=opcode:count ...
```

Texture totals and per-format costs use:

```text
[LOWEND_TEXTURE_CSV] counter,value
[LOWEND_TEXTURE_FORMAT_CSV] operation,format,count,bytes,total_ms,avg_ms
```

`gl_tex_image_2d_calls`, `gl_tex_sub_image_2d_calls` and
`texture_storage_reuses` distinguish storage definition from compatible
updates. `identical_reuploads` uses a profiler-only source hash; its cost is
reported separately as `texture_hash` and is absent from normal builds. See
`docs/TEXTURE_PROFILE.md` for the counter definitions and RG351V findings.

The histogram has 250 microsecond buckets and saturates at 100 ms. Average,
minimum and maximum retain nanosecond input precision; histogram percentiles
are quantized. `renderer_state` samples one of every 16 setup calls to avoid
timing every polygon. Its `calls` value is therefore an estimate in multiples
of 16, while its time statistics describe the sampled calls.

## Stage meanings

| Stage | Meaning |
| --- | --- |
| `frontend_frame` | Wall time spent inside one `retro_run` call. |
| `emulated_frame_wall` | Monotonic wall time between emulated VBlank boundaries. |
| `emulated_frame_cpu` | CPU time of the emulator thread between those boundaries; this is not pure SH4 time. |
| `ta_parse` | TA stream parsing and geometry preparation. |
| `translucent_sort` | The fork's translucent pre-sort used by merge modes. It is absent when those modes are disabled. |
| `index_generation` | `make_index` work; multiple samples may occur per rendered frame. |
| `renderer_process` | Whole renderer process phase. It contains TA parsing and can contain texture work. |
| `renderer_render` | Whole GLES render phase. |
| `renderer_state` | Sampled `SetGPState` cost. |
| `draw_submit` | Wall time of the main draw phase plus a count of GLES draw submissions. |
| `texture_decode` | CPU texture preparation before upload. |
| `texture_upload` | `UploadToGPU`, including the GLES upload path. |
| `aica_update` | AICA scheduling/update work. |
| `audio_mix` | Mixer work nested inside AICA updates. |
| `present` | Time inside the libretro video callback at the core/frontend boundary. RetroRun also reports its presenter/backend timing. |

Nested stages are not additive. Renderer work may run on another thread when
threaded rendering is enabled. Reports reset atomics independently, so a
sample that lands exactly during a report can move one field into the next
window. Use longer windows and repeated runs rather than treating one report
as exact accounting.

The profile build intentionally adds clocks and relaxed atomic counters.
Compare optimizations against the same profile build, and use an unprofiled
build for release FPS claims.

The accepted storage-reuse implementation is compiled by default, while the
profiler is disabled by default:

```sh
LOWEND_PROFILING=0
LOWEND_TEXTURE_SUBIMAGE=1
```

Set `LOWEND_TEXTURE_SUBIMAGE=0` only when a true pre-optimization baseline is
needed. Rejected DIV1, texture-shadow and PAL4 implementations are no longer
present in production source. Always run `make clean` before changing a build
switch.
