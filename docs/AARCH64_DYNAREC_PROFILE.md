# AArch64 dynarec profile

Date: 2026-07-26

The counters are compiled only with `FLYCAST_LOWEND_PROFILING`. Fixed-size
atomic arrays are used; there are no maps or per-event log messages in hot
paths.

## Instrumented locations

- `core/hw/sh4/dyna/driver.cpp`: compilation, lookup failure, block-check
  failure, cache clear, link resolution, block composition and exit class.
- `core/hw/sh4/dyna/blockmanager.cpp`: block addition and discard.
- `core/rec-ARM64/rec_arm64.cpp`: interpreter fallbacks, selected exceptions,
  and allocator preload/writeback generation.
- `core/lowend_profiler.cpp`: interval aggregation and ranking of the 65,536
  possible SH4 opcode words.

## Counter semantics

`compiled_blocks`, `guest_opcodes`, `host_opcodes`, `host_code_bytes`, exit
classes and register preload/writeback values describe code generated during
the report window. They are not execution-weighted. `failed_to_find_blocks`
counts runtime misses that enter compilation. `link_resolutions` counts
runtime block-link requests. `blocks_discarded` is the available invalidation
proxy but does not classify the cause.

`interpreter_fallbacks` and the per-opcode ranking are runtime counts. In the
non-MMU AArch64 path the profiling build routes an existing fallback through a
small wrapper to count it. The normal build retains the original direct call.

## Initial RG351V results

Representative steady 60-frontend-frame windows from the three saved scenes:

| Scene | Fallbacks | #1 | #2 | Other recurring opcodes |
| --- | ---: | --- | --- | --- |
| Sonic Adventure 2 | 23,734 | `406a` 10,776 | `3304` 7,717 | `4f03` 1,556; `4f66` 1,556; `401b` 1,340 |
| Soul Calibur | 8,279 | `3304` 3,182 | `406a` 2,890 | `4f03`, `4f07`, `4f66` 557 each |
| Marvel vs. Capcom 2 | 8,282 | `406a` 3,510 | `3304` 1,959 | `4f03` 1,123; `4f66` 1,123; `401b` 408 |

Soul Calibur also produced one transition window with 725,262 fallbacks, of
which 660,676 were `3304`. It is excluded from the steady table until a deterministic
scene proves whether the burst belongs to state loading, a transition, or
normal gameplay.

The opcode words decode as:

| Word | SH4 instruction |
| --- | --- |
| `3304` | `div1 R0,R3` |
| `406a` | `lds R0,FPSCR` |
| `4f03` | `stc.l SR,@-R15` |
| `4f07` | `ldc.l @R15+,SR` |
| `4f66` | `lds.l @R15+,FPSCR` |
| `401b` | `tas.b @R0` |

Register-form operands vary with the `n` and `m` bits; the table names the
exact words observed.

## Likely focused targets

1. Implement the unmatched `DIV1` path in SHIL/AArch64 instead of falling back
   when the existing 32-step division matcher cannot aggregate it.
2. Handle `lds Rn,FPSCR` without the interpreter while preserving block
   termination, FPU bank swapping and host rounding/denormal state.
3. Handle `lds.l @Rn+,FPSCR`, including memory faults and the same FPSCR
   synchronization rules.
4. Generate `stc.l SR,@-Rn` directly with exact SR reconstruction and memory
   exception behavior.
5. Evaluate direct generation for `tas.b @Rn`; `ldc.l @Rn+,SR` is an
   alternative where its frequency is higher.

This ordering is based on frequency, not yet on measured cost. `DIV1` is the
smallest attractive first experiment, but its Q/M/T state machine requires
interpreter-vs-dynarec validation before performance testing.

## Limitations

- Executed compiled blocks are not counted because adding a counter to every
  block entry would materially perturb execution.
- Helper calls are not grouped; the old backend does not expose a cheap common
  runtime dispatch point with stable helper identifiers.
- Exit counters classify compiled block shapes, not runtime exit frequency.
- Allocator preload/writeback counters measure generated operations, not
  execution-weighted spills and reloads.
- `exceptions` covers the instrumented AArch64 escape paths, not every
  emulator exception.
- The report window starts at core launch and is not synchronized to
  RetroRun's warm-up boundary.
- Current save-state scenes are not deterministic enough for an optimization
  claim.
- Only the final `*-drmreset-r2` runs are used here. Earlier runs with an
  incomplete KMS/DRM reset were discarded after the panel remained black.

## Experiment 1: unmatched `DIV1`

The first focused Phase 3 candidate implements a single unmatched SH4 `DIV1`
operation in SHIL and emitted it directly in the AArch64 backend. It did not
change the existing 32-step division matcher, timing, serialization, renderer
options or the accurate path. After its neutral benchmark result, the
implementation was removed from production source; the report and raw
evidence remain archived.

The implementation:

- decodes unmatched `DIV1` instead of selecting the interpreter fallback;
- passes Q, M and T explicitly through SHIL, with no hidden context side
  effects;
- preserves the `Rm == Rn` alias case;
- provides a side-effect-free canonical implementation and SSA constant
  folding;
- emits shift, add/subtract, carry/borrow and Q/T updates directly with VIXL
  AArch64 operations.

`tools/verify_div1.cpp` compares an independent transcription of the existing
interpreter against the canonical candidate. It covers all Q/M/T and alias
combinations for edge operands plus one million deterministic random vectors.
The test passed 1,005,184 vectors both on the macOS host and natively on the
AArch64 build VM.

The normal Cortex-A35 candidate is:

```text
SHA-256: 7bfd9aa62795702b1e77a2956f0d2d98c39c3902f56928dfa00d487dde8318ab
Build ID: 97901c59f647e9bcee127e75587d03f3cd1d41ff
```

The profiling candidate is:

```text
SHA-256: b5308ec532c885adfd251c35c13712b508af201d4759e2e977605f5cfd284fbf
Build ID: 5846aa21c9a438327a5595f8abac54bafeb9e0df
```

Profiled RG351V smoke tests loaded the existing automatic save states, rendered
and exited cleanly in Sonic Adventure 2, Soul Calibur and Marvel vs. Capcom 2.
`3304` was absent from every candidate fallback ranking. One first Sonic run
displayed a black panel despite reported presentations; the unchanged baseline
was then visible, and a fresh full DRM reset made the same candidate visible.
It is retained as an invalid DRM-handoff run, not as correctness or performance
evidence.

## Experiment 1 benchmark decision

Five alternating normal-build runs per variant used Sonic Adventure 2,
30 seconds of warm-up and at least 60 seconds of measurement. The configuration
was identical in all ten JSON files and had SHA-256
`31dfed80909c20fe30feb80d2c15079641c9559ff4740344f4bdeb069b52365b`.

| Metric | Baseline | `DIV1` candidate | Change |
| --- | ---: | ---: | ---: |
| Mean FPS | 19.423 | 19.389 | -0.18% |
| Median run FPS | 19.246 | 19.108 | -0.72% |
| Mean core time | 31.844 ms | 31.873 ms | +0.09% |
| Mean active-frame p95 | 55.985 ms | 55.835 ms | -0.27% |
| Mean active-frame p99 | 60.015 ms | 60.286 ms | +0.45% |
| Audio underruns, total | 144 | 200 | variable |

Per-pair FPS changes were +0.57%, -2.31%, -0.20%, -0.72% and +1.68%.
The ranges overlap and the aggregate result is neutral. Eliminating a frequent
fallback therefore does not prove that the fallback was expensive enough to
matter in this renderer-limited scene.

**Decision: reject this candidate as a performance optimization.** It is
correctness-tested and removes the intended counter, but it neither reaches
the 5% target nor demonstrates a small repeatable gain. Do not describe it as
an FPS improvement. The review tree intentionally retains the uncommitted
experiment so the implementation and evidence can be inspected before it is
kept or reverted.

Exact raw data and the machine-readable table are under
`benchmarks/rg351v-flycast2022-div1-2026-07-26/`. Temperature was not recorded
at both boundaries of every run, so no acceptance claim should be based on
these results even if the averages had been positive. The launcher did lock
CPU, DMC and GPU frequencies identically for both variants.

The next focused candidate should be selected from a cost signal, not fallback
frequency alone. `lds Rn,FPSCR` remains frequent, but its block-termination and
host floating-point state requirements make it higher risk. Detailed texture
decode/upload profiling on Marvel vs. Capcom 2 is the safer next measurement
phase before another implementation.

## Experiment 2: direct `LDS Rn,FPSCR`

A later isolated build decoded `LDS Rn,FPSCR` into `shop_mov32` followed by
the existing `shop_sync_fpscr` operation, then terminated the block because
FPSCR controls FPU banking and decoding. It exposed the experiment through
the runtime core option `reicast_sh4_fpscr`.

Three paired Sonic runs used the compatible in-game audio save, five seconds
of warm-up and exactly 600 core frames. Pair FPS changes were -0.26%, +1.93%
and +0.71%; the mean was +0.79%. Paired mean core time changed from 25.884 to
25.574 ms. The candidate loaded the save and exited cleanly, but the result is
too small and variable to enable by default.

The reviewed implementation was initially included in the normal core behind
`reicast_sh4_fpscr`, disabled by default. A corrected runtime smoke test used
the same final AArch64 binary for both settings. OFF completed 300 Sonic frames
in 13.124 seconds (27.491 ms mean core time); ON completed them in 13.204
seconds (27.631 ms). Both paths loaded the compatible save and exited without
reported GL, state-load or dynarec errors. These short runs validate runtime
selection rather than superseding the longer paired performance measurements.

SH4 timeslice doubling and ARM7 dispatcher batching were also screened.
Timeslice 896 was neutral (-0.10%). ARM7 batch 2 produced +0.83% once, but
batch 4 was -0.27% and introduced greater interrupt-timing risk; both batching
changes were removed.

Raw evidence is under
`benchmarks/rg351v-flycast2022-empirical-wave1-2026-07-26/`.
