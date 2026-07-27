# Next optimization investigation

Date: 2026-07-26

Phase 1 and Phase 2 instrumentation identified unmatched `DIV1` as the smallest
high-frequency AArch64 dynarec candidate. It removed the intended fallback but
was neutral on RG351V. Phase 5 then identified repeated texture storage
definition in Marvel vs. Capcom 2. Exact-source shadowing was inconclusive, but
compatible `glTexSubImage2D` reuse produced a repeatable gain, passed manual
visual/audio review and is enabled by default with a runtime opt-out. An exact
PAL4 pair cache was then verified but rejected because it reduced only the
already-small decode stage.

## Scope and execution-path map

The investigation first reviewed `README.md`, `docs/FORK_AUDIT_2026-07-24.md`,
`docs/PATCH_ARCHIVE.md`, `docs/SH4_CLOCK_EXPERIMENT.md`, and the archived
experiments referenced by the patch archive.

The instrumented paths are:

| Area | Main path |
| --- | --- |
| Frontend/frame | `core/libretro/libretro.cpp`: `retro_run` drives `dc_run`, consumes a rendered frame with `rend_single_frame`, and calls the libretro video callback. |
| SH4 dynarec | `core/nullDC.cpp`: `sh4_cpu.Run` -> `core/hw/sh4/dyna/driver.cpp`: `recSh4_Run`; missing blocks go through `bm_GetCodeByVAddr`, are compiled with `ngen_Compile`, and use the AArch64 backend in `core/rec-ARM64/rec_arm64.cpp`. |
| PowerVR/renderer | PVR start-render events call `rend_start_render`; `core/hw/pvr/Renderer_if.cpp` queues/dequeues contexts and invokes renderer `Process` and `Render`. The GLES `Process` path parses TA data with `ta_parse_vdrc`/`make_index`; `core/rend/gles/gldraw.cpp` performs state setup and draw submission. |
| Texture cache | `core/rend/TexCache.cpp` prepares and hashes texture data, then the GLES implementation in `core/rend/gles/gltex.cpp` performs `UploadToGPU`. |
| AICA/audio | `core/hw/aica/aica.cpp`: `libAICA_TimeStep` schedules samples; `core/hw/aica/sgc_if.cpp`: `AICA_Sample` performs mixer work. |

The profiling implementation is isolated in `core/lowend_profiler.{h,cpp}`.
The Makefiles add the compile-time switch; call sites were added only to the
paths listed above. Full modified-file details remain visible in the review
diff.

## What the first profiles show

Sonic Adventure 2's accurate path is renderer/draw-call dominated. A steady
window spent roughly 28.8 ms in the draw phase, about 3.7 ms in TA parsing,
and submitted approximately 66,800 draws per 60 frontend frames: around 1,113
per frame. This explains why the already-known inaccurate translucent merge
can help this title, but does not identify a new accurate batching rule.

Marvel vs. Capcom 2 exercised texture processing much more heavily. A steady
window recorded approximately 2,410 texture decodes and uploads per 60
frontend frames. Individual operations were short, but their aggregate cost
is large enough to justify the more detailed texture counters requested by
the later investigation phase.

Across all three scenes, interpreter fallback frequency is high and dominated
by a small opcode set. `DIV1` (`3304`) is especially promising because this
revision already aggregates recognized 32-step division sequences but falls
back for unmatched individual operations. This is a narrower first candidate
than importing the modern register allocator.

## Phase 3 experiment performed

The experiment implemented one unmatched `DIV1` operation:

1. an independent interpreter comparison covers all Q/M/T and operand alias
   cases for edge values and one million deterministic random inputs;
2. the canonical helper passed 1,005,184 vectors on both the host and AArch64
   VM;
3. the AArch64 backend emits the operation inline rather than calling a
   helper;
4. profiled RG351V smoke tests confirm that `3304` reaches zero in Sonic
   Adventure 2, Soul Calibur and Marvel vs. Capcom 2;
5. five alternating baseline/candidate Sonic runs use 30-second warm-up and
   60-second measurement windows;
6. the candidate mean is 19.389 FPS versus 19.423 FPS for baseline (-0.18%).

The implementation was not combined with strip merging, SH4 clock changes,
texture work or a register-allocator port. Existing automatic save states
loaded successfully, so serialization compatibility was not changed.

The result demonstrates why fallback count alone is not a cost measurement:
`DIV1` was frequent, but Sonic remains dominated by GLES draw submission.
Per-pair deltas crossed zero and the candidate's FPS range overlapped the
baseline range.

**Decision: reject as an FPS optimization.** The implementation was removed;
the report and benchmark evidence remain for review and must not be presented
as a speedup.

## Experiment 2: exact texture source shadow

### Hypothesis and evidence

MVC2 produced 2,304-4,120 texture updates per short profile window, including
hundreds of updates with identical source bytes.

### Implementation

`LOWEND_TEXTURE_SHADOW_SKIP=1` stores the exact protected VRAM range and palette
identity after upload. A later invalidation is skipped only after exact
`memcmp`; no hash collision can affect rendering.

### Results and decision

A five-pair, exactly 2,400-core-frame comparison reported +4.42% mean
throughput, -6.05% core time and -5.16% active p95, but only three pairs
improved and the distributions overlapped.

**Decision: Reject.** The implementation was removed and is not used in
subsequent candidate builds.

## Experiment 3: compatible GLES storage reuse

### Hypothesis and evidence

The same MVC2 windows called `glTexImage2D` for essentially every one-level
texture update and never generated mipmaps. Upload time was approximately
twice decode time per update.

### Implementation

`LOWEND_TEXTURE_SUBIMAGE=1` tracks allocation width, height, format and type.
It calls `glTexSubImage2D` only on an exact match; first uploads, mismatches,
new GL IDs and RTT hand-offs use or force `glTexImage2D`.

### Benchmark environment

Five alternating unprofiled MVC2 pairs used the exact archived configuration,
30-second warm-up, 60-second measurement and full DRM reset. DIV1 and texture
shadowing were disabled in both builds.

### Results

- mean FPS: 41.722 -> 44.832 (+7.45%);
- median FPS: 40.990 -> 45.790 (+11.71%);
- mean core time: 12.316 -> 10.355 ms (-15.93%);
- active p95: 34.934 -> 31.713 ms (-9.22%);
- all five pairs improved FPS and core time;
- audio underruns: 31 -> 34.

A three-pair Sonic guard was neutral within noise and had 128 versus 127 audio
underruns. Sonic and Soul Calibur smoke tests produced no crash, assert or GL
error marker.

### Risks and decision

The final candidate passed the user's visual and audible RG351V tests, with no
reported regression in the exercised menus, gameplay or audio.

**Decision: Keep and enable by default.** Release builds compile the support
with `LOWEND_TEXTURE_SUBIMAGE=1` and expose a runtime escape hatch:
`reicast_texture_storage_reuse = disabled` restores the original upload path.
The build-time flag can also be set to `0` for an emergency package-level
opt-out.

## Experiment 4: exact PAL4 pair cache

### Hypothesis and verification

MVC2's baseline profile contained 41,187 PAL4 decodes taking 685.858 ms.
`LOWEND_PAL4_PAIR_CACHE=1` precomputes the two converted palette pixels for all
256 packed source bytes and rebuilds the table only after an exact 16-entry
palette comparison.

An independent harness verified all byte values for 10,003 palettes and more
than one million random tiles on both the host and AArch64 VM. The device
profile recorded 82,960 exact cache hits and 32 palette-triggered rebuilds,
with no crash, assertion or GL error marker.

### Results and decision

Mean PAL4 decode cost changed from 0.016652 to 0.016115 ms/update (-3.22%).
Even at the observed update rate, this saves only a small fraction of one
millisecond per frontend frame; the expected end-to-end gain is below one
percent.

**Decision: Reject for insufficient impact.** A long alternating FPS run was
not warranted and the implementation was removed.

## Experiment 5: GCC profile-guided optimization

### Hypothesis and procedure

An AArch64 GCC 9.4 build with `-fprofile-generate` was prepared for a
representative Sonic, MVC2 and Soul Calibur training set. All implementation
switches were disabled and the exact archived RetroRun configuration and DRM
reset procedure were retained.

### Results and decision

The generate core linked and loaded, but consistently stopped before its first
frame in the generated SH4 dispatcher. GDB localized the fault to
`SH4_TCB+868`, during the first FPCB dispatch-table load. Removing legacy
gprof instrumentation, excluding naked and signal-sensitive translation units,
excluding the dynarec driver, and temporarily disabling native mapped memory
all reproduced the same exit status 133. No `.gcda` file was produced.

**Decision: Reject as infeasible with the current GCC 9/Flycast 2022 dynarec
architecture.** No profile-use core or performance claim was produced, and all
PGO-only source workarounds were removed. Raw evidence is archived under
`benchmarks/rg351v-flycast2022-pgo-2026-07-26/`.

## Recommended next measurement

Keep the accepted subimage rule exact. The next wave deliberately broadens the
search method: rapidly screen previously untried renderer, scheduling,
data-movement and toolchain variants on the device, then spend long A/B time
only on variants that show a material real-FPS signal. Rendering omissions,
frameskip and timing cheats do not count as gains.

## Redream-inspired SH4 fallback screen

Three additional high-frequency interpreter fallbacks were translated directly
into Flycast SHIL using the operation ordering found in Redream's SH4
translator. Each was exposed temporarily as a disabled-by-default runtime
option and tested with a single candidate binary, alternating settings, the
same Sonic save, locked frequencies and a complete DRM reset.

| SH4 instruction | Result | Decision |
|---|---:|---|
| `STC.L SR,@-Rn` | -2.04% FPS, two 600-frame runs/setting | Remove |
| `TAS.B @Rn` | -0.88% FPS, two 600-frame runs/setting | Remove |
| `LDS.L @Rn+,FPSCR` | +3.87% short mean; -0.19% long A/B | Remove |

The FPSCR memory candidate was +2.15% when all 3000 frames were pooled, but the
1200-frame validation was neutral/slightly negative. None met the threshold for
retention. All implementations and temporary core options were removed; raw
JSON and logs remain under the matching
`benchmarks/rg351v-flycast2022-sh4-*` directories.

## Validated opaque grouping result

Profiling showed that Sonic Adventure 2 submits roughly 1,100 GLES draws per
frontend frame. Enabling the existing exact-state opaque-strip grouping on top
of the shadow-safe depth profile directly targeted that bottleneck without
reducing resolution or skipping frames.

On the same RG351V save-state, with a 15-second warm-up, 45-second measurement
window and locked CPU/GPU/DMC frequencies, the previous profile presented
1,046 frames at 23.24 FPS. The final
`menu_guarded_shadow_safe + opaque_strip_merge` profile presented 1,366 frames
at 30.35 FPS (`+30.6%`), with no skipped frames. The user then approved
gameplay, audio, menus and projected shadows.

The depth-only aggressive variant was not retained as the recommended profile:
it measured 29.09 FPS and still showed rectangular shadow artifacts. The
validated result therefore comes from reducing draw submissions while keeping
the targeted shadow correction, not from accepting that regression.

## Evidence still required

- fixed, repeatable save states or scripted input for at least three workload
  classes;
- a full AmberELEC package/toolchain build for a release artifact;
- a dArkOS build and device test if dArkOS support is part of the release.

The alternating-run and instruction-vector requirements have now been met for
the rejected `DIV1` candidate. The raw core loads and runs correctly through
RetroRun on AmberELEC, but that does not prove distribution packaging, full
visual/audio accuracy or dArkOS behavior. Only runs made with the complete
KMS/DRM reset are used for the figures above.
